// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.WebSockets;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Disposables;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.Services;
using Microsoft.R.Host.Client.BrokerServices;
using Microsoft.R.Host.Client.Transports;
using Microsoft.R.Host.Protocol;
using static System.FormattableString;

namespace Microsoft.R.Host.Client.Host {
    public abstract class BrokerClient : IBrokerClient {
        private static readonly TimeSpan HeartbeatTimeout =
#if DEBUG
            // In debug mode, increase the timeout significantly, so that when the host is paused in debugger,
            // the client won't immediately timeout and disconnect.
            TimeSpan.FromMinutes(10);
#else
            TimeSpan.FromSeconds(5);
#endif
        private readonly string _interpreterPath;
        private readonly string _interpreterArchitecture;
        private readonly string _rCommandLineArguments;
        private readonly ICredentialsDecorator _credentials;

        protected DisposableBag DisposableBag { get; } = DisposableBag.Create<BrokerClient>();
        protected IActionLog Log { get; }
        protected HttpClientHandler HttpClientHandler { get; private set; }
        protected HttpClient HttpClient { get; private set; }

        public BrokerConnectionInfo ConnectionInfo { get; }
        public string Name { get; }

        protected BrokerClient(string name, BrokerConnectionInfo connectionInfo, ICredentialsDecorator credentials, IServiceContainer services) {
            Name = name;
            Log = services.Log();

            _rCommandLineArguments = connectionInfo.RCommandLineArguments;
            _interpreterPath = connectionInfo.InterpreterPath;
            _interpreterArchitecture = connectionInfo.InterpreterArchitecture;
            _credentials = credentials;
            ConnectionInfo = connectionInfo;
        }

        protected void CreateHttpClient(Uri baseAddress) {
            HttpClientHandler = new HttpClientHandler {
                PreAuthenticate = true,
                Credentials = _credentials
            };

            try {
                HttpClient = new HttpClient(HttpClientHandler) {
                    BaseAddress = baseAddress,
                    Timeout = TimeSpan.FromSeconds(30),
                };
            } catch(ArgumentException) {
                var message = Resources.Error_InvalidUrl.FormatInvariant(baseAddress);
                throw new RHostDisconnectedException(message);
            }

            HttpClient.DefaultRequestHeaders.Accept.Clear();
            HttpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
        }

        public void Dispose() => DisposableBag.TryDispose();

        public virtual async Task<RHost> ConnectAsync(HostConnectionInfo connectionInfo, CancellationToken cancellationToken = default) {
            DisposableBag.ThrowIfDisposed();
            await TaskUtilities.SwitchToBackgroundThread();

            var uniqueSessionName = $"{connectionInfo.Name}_{Guid.NewGuid()}";
            try {
                // _services.UI().LogMessage($"Creating broker session {uniqueSessionName}");
                await CreateBrokerSessionAsync(uniqueSessionName, connectionInfo.UseRHostCommandLineArguments, connectionInfo.IsInteractive, cancellationToken);
                // _services.UI().LogMessage($"Connecting to broker session {uniqueSessionName}");
                var webSocket = await ConnectToBrokerAsync(uniqueSessionName, cancellationToken);
                return CreateRHost(uniqueSessionName, connectionInfo.Callbacks, webSocket);
            } catch (HttpRequestException ex) {
                throw await HandleHttpRequestExceptionAsync(ex);
            } catch (WebSocketException wsex) {
                throw new RHostDisconnectedException(Resources.Error_HostNotResponding.FormatInvariant(Name, wsex.Message), wsex);
            }
        }

        public Task TerminateSessionAsync(string name, CancellationToken cancellationToken = default) {
            var sessionsService = new SessionsWebService(HttpClient, _credentials, Log);
            return sessionsService.DeleteAsync(name, cancellationToken);
        }

        protected virtual Task<Exception> HandleHttpRequestExceptionAsync(HttpRequestException exception)
            => Task.FromResult<Exception>(new RHostDisconnectedException(Resources.Error_HostNotResponding.FormatInvariant(Name, exception.Message), exception));

        private async Task CreateBrokerSessionAsync(string name, bool useRCommandLineArguments, bool isInteractive, CancellationToken cancellationToken) {
            var rCommandLineArguments = useRCommandLineArguments && _rCommandLineArguments != null ? _rCommandLineArguments : null;
            var sessions = new SessionsWebService(HttpClient, _credentials, Log);
            using (Log.Measure(LogVerbosity.Normal, Invariant($"Create broker session \"{name}\""))) {
                try {
                    await sessions.PutAsync(name, new SessionCreateRequest {
                        InterpreterPath = _interpreterPath,
                        InterpreterArchitecture = _interpreterArchitecture,
                        CommandLineArguments = rCommandLineArguments,
                        IsInteractive = isInteractive,
                    }, cancellationToken);
                } catch (BrokerApiErrorException apiex) {
                    throw new RHostDisconnectedException(MessageFromBrokerApiException(apiex), apiex);
                }
            }
        }

        private async Task<WebSocket> ConnectToBrokerAsync(string name, CancellationToken cancellationToken) {
            using (Log.Measure(LogVerbosity.Normal, Invariant($"Connect to broker session \"{name}\""))) {
                var pipeUri = new UriBuilder(HttpClient.BaseAddress) {
                    // Host = "localhost.fiddler",
                    Scheme = "ws",
                    Path = $"sessions/{name}/pipe"
                }.Uri;

                var wsClient = new WebSocketClient(pipeUri, new List<string> { "Microsoft.R.Host" }, HeartbeatTimeout, HttpClientHandler.Credentials);
                while (true) {
                    using (await _credentials.LockCredentialsAsync(cancellationToken)) {
                        try {
                            return await wsClient.ConnectAsync(cancellationToken);
                        } catch (UnauthorizedAccessException) {
                            _credentials.InvalidateCredentials();
                        } catch (Exception ex) when (ex is InvalidOperationException || ex is WebSocketException) {
                            throw new RHostDisconnectedException(Resources.HttpErrorCreatingSession.FormatInvariant(Name, ex.Message), ex);
                        }
                    }
                }
            }
        }

        private RHost CreateRHost(string name, IRCallbacks callbacks, WebSocket socket) {
            var transport = new WebSocketMessageTransport(socket);
            return new RHost(name, callbacks, transport, Log);
        }

        private string MessageFromBrokerApiException(BrokerApiErrorException ex) {
            switch (ex.ApiError) {
                case BrokerApiError.NoRInterpreters:
                    return Resources.Error_NoRInterpreters;
                case BrokerApiError.InterpreterNotFound:
                    return Resources.Error_InterpreterNotFound.FormatInvariant(_interpreterPath);
                case BrokerApiError.UnableToStartRHost:
                    if (!string.IsNullOrEmpty(ex.Message)) {
                        return Resources.Error_UnableToStartHostException.FormatInvariant($"{ex.Message}\n{ex.StackTrace}");
                    }
                    return Resources.Error_UnknownError;
                case BrokerApiError.PipeAlreadyConnected:
                    return Resources.Error_PipeAlreadyConnected;
                case BrokerApiError.Win32Error:
                    if (!string.IsNullOrEmpty(ex.Message)) {
                        return Resources.Error_BrokerWin32Error.FormatInvariant(ex.Message);
                    }
                    return Resources.Error_BrokerUnknownWin32Error;
            }

            Debug.Fail("No localized resources for broker API error" + ex.ApiError.ToString());
            return ex.ApiError.ToString();
        }
    }
}
