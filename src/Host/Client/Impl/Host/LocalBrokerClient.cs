﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Json;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.OS;
using Microsoft.Common.Core.Services;
using Microsoft.Common.Core.Threading;
using Microsoft.R.Platform.Host;
using Newtonsoft.Json;

namespace Microsoft.R.Host.Client.Host {
    public sealed class LocalBrokerClient : BrokerClient {
        private const string InterpreterId = "local";

        private static readonly bool ShowConsole;
        private static readonly LocalCredentialsDecorator _credentials = new LocalCredentialsDecorator();

        private readonly string _rHome;
        private readonly BinaryAsyncLock _connectLock = new BinaryAsyncLock();
        private readonly IServiceContainer _services;

        private IProcess _brokerProcess;

        static LocalBrokerClient() {
            // Allow "true" and non-zero integer to enable, otherwise disable.
            var rtvsShowConsole = Environment.GetEnvironmentVariable("RTVS_SHOW_CONSOLE");
            if (!bool.TryParse(rtvsShowConsole, out ShowConsole)) {
                if (int.TryParse(rtvsShowConsole, out var n) && n != 0) {
                    ShowConsole = true;
                }
            }
        }

        public LocalBrokerClient(string name, BrokerConnectionInfo connectionInfo, IServiceContainer services, IConsole console, IRSessionProvider sessionProvider)
            : base(name, connectionInfo, _credentials, console, services, sessionProvider) {
            _rHome = connectionInfo.Uri.LocalPath;
            _services = services;
        }

        public override async Task<RHost> ConnectAsync(HostConnectionInfo connectionInfo, CancellationToken cancellationToken = default) {
            await EnsureBrokerStartedAsync(cancellationToken);
            return await base.ConnectAsync(connectionInfo, cancellationToken);
        }

        private async Task EnsureBrokerStartedAsync(CancellationToken cancellationToken) {
            DisposableBag.ThrowIfDisposed();
            await TaskUtilities.SwitchToBackgroundThread();

            var lockToken = await _connectLock.WaitAsync(cancellationToken);
            try {
                if (!lockToken.IsSet) {
                    await ConnectToBrokerWorker(cancellationToken);
                }
                lockToken.Set();
            } finally {
                lockToken.Reset();
            }
        }

        private async Task ConnectToBrokerWorker(CancellationToken cancellationToken) {
            Trace.Assert(_brokerProcess == null);
            var fs = _services.FileSystem();
            var locator = BrokerExecutableLocator.Create(fs);

            var rhostExe = locator.GetHostExecutablePath();
            if (!fs.FileExists(rhostExe)) {
                throw new RHostBinaryMissingException();
            }

            var rhostBrokerExe = locator.GetBrokerExecutablePath();
            if (!fs.FileExists(rhostBrokerExe)) {
                throw new RHostBrokerBinaryMissingException();
            }

            IProcess process = null;
            try {
                var pipeName = Guid.NewGuid().ToString();
                var cts = new CancellationTokenSource(Debugger.IsAttached ? 500000 : 100000);

                using (var processConnectCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, cts.Token))
                using (var serverUriPipe = new NamedPipeServerStream(pipeName, PipeDirection.In, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous)) {
                    var psi = GetProcessStartInfo(rhostBrokerExe, pipeName);
                    if (!ShowConsole) {
                        psi.CreateNoWindow = true;
                    }

                    process = StartBroker(psi);
                    process.Exited += delegate
                    {
                        cts.Cancel();
                        _brokerProcess = null;
                        _connectLock.EnqueueReset();
                    };

                    var uri = await GetBrokerUri(serverUriPipe, processConnectCts.Token, cts.Token);
                    CreateHttpClient(uri);
                }

                if (DisposableBag.TryAdd(DisposeBrokerProcess)) {
                    _brokerProcess = process;
                }
            } finally {
                if (_brokerProcess == null) {
                    try {
                        process?.Kill();
                    } catch (Exception) {
                    } finally {
                        process?.Dispose();
                    }
                }
            }
        }

        private async Task<Uri> GetBrokerUri(NamedPipeServerStream serverUriPipe, CancellationToken processConnectToken, CancellationToken connectionToken) {
            await serverUriPipe.WaitForConnectionAsync(processConnectToken);

            var serverUriData = new MemoryStream();
            try {
                // Pipes are special in that a zero-length read is not an indicator of end-of-stream.
                // Stream.CopyTo uses a zero-length read as indicator of end-of-stream, so it cannot 
                // be used here. Instead, copy the data manually, using PipeStream.IsConnected to detect
                // when the other side has finished writing and closed the pipe.
                var buffer = new byte[0x1000];
                var totalRead = 0;
                do {
                    var count = await serverUriPipe.ReadAsync(buffer, 0, buffer.Length, connectionToken);
                    serverUriData.Write(buffer, 0, count);
                    totalRead += count;
                    // serverUriPipe.IsConnected is not always reliable. Read tetminator instead.
                    if (count == 0 && totalRead > 0 && buffer[totalRead - 1] == (byte)'^') {
                        break;
                    }
                } while (serverUriPipe.IsConnected);
            } catch (OperationCanceledException) {
                throw new RHostDisconnectedException("Timed out while waiting for broker process to report its endpoint URI");
            }

            var serverUriStr = Encoding.UTF8.GetString(serverUriData.ToArray());
            if(serverUriStr.EndsWithOrdinal("^")) {
                serverUriStr = serverUriStr.Substring(0, serverUriStr.Length - 1);
            }

            Uri[] serverUri;
            try {
                serverUri = Json.DeserializeObject<Uri[]>(serverUriStr);
            } catch (JsonSerializationException ex) {
                throw new RHostDisconnectedException($"Invalid JSON for endpoint URIs received from broker ({ex.Message}): {serverUriStr}");
            }
            if (serverUri?.Length != 1) {
                throw new RHostDisconnectedException($"Unexpected number of endpoint URIs received from broker: {serverUriStr}");
            }
            return serverUri[0];
        }

        private IProcess StartBroker(ProcessStartInfo psi) {
            var process = _services.Process().Start(psi);
            process.WaitForExit(250);
            if (process.HasExited && process.ExitCode < 0) {
                var message = new Win32Exception(process.ExitCode).Message;
                throw new RHostDisconnectedException(Resources.Error_UnableToStartBrokerException.FormatInvariant(message, process.ExitCode));
            }
            return process;
        }

        private void DisposeBrokerProcess() {
            try {
                _brokerProcess?.Kill();
            } catch (Exception) {
            }

            _brokerProcess?.Dispose();
        }

        private ProcessStartInfo GetProcessStartInfo(string rhostBrokerExecutable, string pipeName) {
            var baseArguments =
                $" --logging:logFolder \"{Log.Folder.TrimTrailingSlash()}\"" +
                $" --logging:logHostOutput {Log.LogVerbosity >= LogVerbosity.Normal}" +
                $" --logging:logPackets {Log.LogVerbosity == LogVerbosity.Traffic}" +
                $" --urls http://127.0.0.1:0" + // :0 means first available ephemeral port
                $" --startup:name \"{Name}\"" +
                $" --startup:writeServerUrlsToPipe {pipeName}" +
                $" --lifetime:parentProcessId {Process.GetCurrentProcess().Id}" +
                $" --security:secret \"{_credentials.Password}\"" +
                $" --R:interpreters:{InterpreterId}:name \"{Name}\"" +
                $" --R:interpreters:{InterpreterId}:basePath \"{_rHome.TrimTrailingSlash()}\"";

            var psi = new ProcessStartInfo {
                FileName = "dotnet",
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                Arguments = $"{rhostBrokerExecutable} {baseArguments}"
            };
            return psi;
        }
    }
}
