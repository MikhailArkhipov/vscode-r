﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Disposables;
using Microsoft.Common.Core.Services;
using Microsoft.Common.Core.Tasks;
using Microsoft.Common.Core.Threading;
using Microsoft.R.Common.Core.Output;
using Microsoft.R.Host.Client.Host;
using Microsoft.R.Platform.Interpreters;

namespace Microsoft.R.Host.Client.Session {
    public sealed class RSessionProvider : IRSessionProvider {
        private readonly ConcurrentDictionary<string, RSession> _sessions = new ConcurrentDictionary<string, RSession>();
        private readonly DisposeToken _disposeToken = DisposeToken.Create<RSessionProvider>();
        private readonly AsyncReaderWriterLock _connectArwl = new AsyncReaderWriterLock();

        private readonly BrokerClientProxy _brokerProxy;
        private readonly IServiceContainer _services;
        private readonly IConsole _console;
        private readonly ITaskService _taskService;

        private int _isConnected;
        private int _sessionCounter;

        public bool HasBroker => _brokerProxy.HasBroker;

        public bool IsConnected {
            get => _isConnected != 0;
            private set {
                var isConnected = value ? 1 : 0;
                if (Interlocked.Exchange(ref _isConnected, isConnected) != isConnected) {
                    var args = new BrokerStateChangedEventArgs(value);
                    Task.Run(() => BrokerStateChanged?.Invoke(this, args)).DoNotWait();
                }
            }
        }

        public IBrokerClient Broker => _brokerProxy;

        public event EventHandler BeforeDisposed;
        public event EventHandler BrokerChanging;
        public event EventHandler BrokerChangeFailed;
        public event EventHandler BrokerChanged;
        public event EventHandler<BrokerStateChangedEventArgs> BrokerStateChanged;

        public RSessionProvider(IServiceContainer services) {
            _console = services.GetService<IConsole>() ?? new NullConsole();
            _brokerProxy = new BrokerClientProxy();
            _services = services;
            // Cache task service since we need it during disposal.
            // This service may be disposed AFTER the service container service is marked as disposed.
            _taskService = _services.Tasks();
        }

        public IRSession GetOrCreate(string sessionId) {
            _disposeToken.ThrowIfDisposed();
            return _sessions.GetOrAdd(sessionId, CreateRSession);
        }

        public IEnumerable<IRSession> GetSessions() => _sessions.Values;

        public void Dispose() {
            if (!_disposeToken.TryMarkDisposed()) {
                return;
            }

            try {
                BeforeDisposed?.Invoke(this, EventArgs.Empty);
            } catch (Exception ex) when (!ex.IsCriticalException()) { }

            var sessions = GetSessions().ToList();
            var stopHostTasks = sessions.Select(session => session.StopHostAsync(false));
            try {
                _taskService.Wait(() => Task.WhenAll(stopHostTasks));
            } catch (Exception ex) when (!ex.IsCriticalException()) { }

            foreach (var session in sessions) {
                session.Dispose();
            }

            Broker.Dispose();
        }

        private RSession CreateRSession(string sessionId) 
            => new RSession(Interlocked.Increment(ref _sessionCounter), sessionId, Broker, _connectArwl.CreateExclusiveReaderLock(), () => DisposeSession(sessionId));

        private void DisposeSession(string sessionId) => _sessions.TryRemove(sessionId, out _);

        private void OnBrokerChanged() => Task.Run(() 
            => BrokerChanged?.Invoke(this, new EventArgs())).DoNotWait();

        public async Task RemoveBrokerAsync(CancellationToken cancellationToken = default) {
            using (_disposeToken.Link(ref cancellationToken)) {
                await TaskUtilities.SwitchToBackgroundThread();

                var lockToken = await _connectArwl.WriterLockAsync(cancellationToken);
                try {
                    BrokerChanging?.Invoke(this, EventArgs.Empty);
                    await StopSessionsAsync(_sessions.Values, false, cancellationToken);
                    var oldBroker = _brokerProxy.Set(new NullBrokerClient());
                    oldBroker?.Dispose();
                } finally {
                    lockToken.Dispose();
                }

                OnBrokerChanged();
            }
        }

        public async Task<bool> TrySwitchBrokerAsync(string name, BrokerConnectionInfo connectionInfo = default, CancellationToken cancellationToken = default) {
            using (_disposeToken.Link(ref cancellationToken)) {
                await TaskUtilities.SwitchToBackgroundThread();

                var brokerClient = CreateBrokerClient(name, connectionInfo);
                if (brokerClient == null) {
                    return false;
                }

                // Broker switching shouldn't be concurrent
                IAsyncReaderWriterLockToken lockToken;
                try {
                    lockToken = await _connectArwl.WriterLockAsync(cancellationToken);
                } catch (OperationCanceledException) {
                    brokerClient.Dispose();
                    return false;
                }

                if (brokerClient.ConnectionInfo.Equals(_brokerProxy.ConnectionInfo)) {
                    brokerClient.Dispose();

                    try {
                        // Switching to the broker that is currently running and connected is always successful
                        if (IsConnected) {
                            return true;
                        }

                        await ReconnectAsync(cancellationToken);
                    } catch (Exception) {
                        return false;
                    } finally {
                        lockToken.Dispose();
                    }

                    IsConnected = true;
                    return true;
                }

                // First switch broker proxy so that all new sessions are created for the new broker
                var oldBroker = _brokerProxy.Set(brokerClient);
                try {
                    BrokerChanging?.Invoke(this, EventArgs.Empty);
                    await SwitchBrokerAsync(cancellationToken);
                    oldBroker.Dispose();
                } catch (Exception ex) {
                    _brokerProxy.Set(oldBroker);
                    if (_disposeToken.IsDisposed) {
                        oldBroker.Dispose();
                    }
                    brokerClient.Dispose();
                    BrokerChangeFailed?.Invoke(this, EventArgs.Empty);
                    if (ex is OperationCanceledException || ex is ComponentBinaryMissingException) {
                        // RHostDisconnectedException is derived from OperationCanceledException
                        return false;
                    }
                    throw;
                } finally {
                    lockToken.Dispose();
                }

                OnBrokerChanged();
                return true;
            }
        }

        private async Task SwitchBrokerAsync(CancellationToken cancellationToken) {
            var transactions = new List<IRSessionSwitchBrokerTransaction>();
            var sessionsToStop = new List<RSession>();

            foreach (var session in _sessions.Values) {
                var transaction = session.StartSwitchingBroker();
                if (transaction != null) {
                    transactions.Add(transaction);
                } else {
                    sessionsToStop.Add(session);
                }
            }

            try {
                if (transactions.Any()) {
                    await SwitchSessionsAsync(transactions, sessionsToStop, cancellationToken);
                } else {
                    await StopSessionsAsync(sessionsToStop, true, cancellationToken);
                }
            } catch (OperationCanceledException ex) when (!(ex is RHostDisconnectedException)) {
                throw;
            } catch (Exception ex) {
                _console.WriteErrorLine(Resources.RSessionProvider_ConnectionFailed.FormatInvariant(ex.Message));
                throw;
            }
        }

        private async Task SwitchSessionsAsync(IReadOnlyCollection<IRSessionSwitchBrokerTransaction> transactions, List<RSession> sessionsToStop, CancellationToken cancellationToken) {
            // All sessions should participate in switch. If any of it didn't start, cancel the rest.
            try {
                await WhenAllCancelOnFailure(transactions, ConnectToNewBrokerAsync, cancellationToken);
                await Task.WhenAll(CompleteSwitchingBrokerAsync(transactions, cancellationToken), StopSessionsAsync(sessionsToStop, true, cancellationToken));
            } finally {
                foreach (var transaction in transactions) {
                    transaction.Dispose();
                }
            }
        }

        private Task StopSessionsAsync(IEnumerable<RSession> sessions, bool waitForShutdown, CancellationToken cancellationToken) 
            => WhenAllCancelOnFailure(sessions, (s, ct) => s.StopHostAsync(waitForShutdown, ct), cancellationToken);

        private async Task ReconnectAsync(CancellationToken cancellationToken) {
            var sessions = _sessions.Values.ToList();
            try {
                if (sessions.Any()) {
                    await WhenAllCancelOnFailure(sessions, (s, ct) => s.ReconnectAsync(ct), cancellationToken);
                }
            } catch (OperationCanceledException ex) when (!(ex is RHostDisconnectedException)) {
                throw;
            } catch (Exception ex) {
                _console.WriteError(Resources.RSessionProvider_ConnectionFailed.FormatInvariant(ex.Message) + Environment.NewLine);
                throw;
            }
        }

        private static Task ConnectToNewBrokerAsync(IRSessionSwitchBrokerTransaction transaction, CancellationToken cancellationToken)
            => transaction.ConnectToNewBrokerAsync(cancellationToken);

        private async Task CompleteSwitchingBrokerAsync(IEnumerable<IRSessionSwitchBrokerTransaction> transactions, CancellationToken cancellationToken) {
            try {
                await WhenAllCancelOnFailure(transactions, CompleteSwitchingBrokerAsync, cancellationToken);
            } catch (OperationCanceledException ex) when (!(ex is RHostDisconnectedException)) {
            } catch (Exception ex) {
                _console.WriteError(Resources.RSessionProvider_ConnectionFailed.FormatInvariant(ex.Message) + Environment.NewLine);
                throw;
            }
        }

        private static Task CompleteSwitchingBrokerAsync(IRSessionSwitchBrokerTransaction transaction, CancellationToken cancellationToken)
            => transaction.CompleteSwitchingBrokerAsync(cancellationToken);

        private static Task WhenAllCancelOnFailure(IEnumerable<IRSessionSwitchBrokerTransaction> transactions, Func<IRSessionSwitchBrokerTransaction, CancellationToken, Task> taskFactory, CancellationToken cancellationToken) {
            return TaskUtilities.WhenAllCancelOnFailure(transactions, async (t, ct) => {
                try {
                    await taskFactory(t, ct);
                } catch (ObjectDisposedException) when (t.IsSessionDisposed) {
                    ct.ThrowIfCancellationRequested();
                } catch (OperationCanceledException) when (t.IsSessionDisposed) {
                    ct.ThrowIfCancellationRequested();
                }
            }, cancellationToken);
        }

        private static Task WhenAllCancelOnFailure(IEnumerable<RSession> sessions, Func<RSession, CancellationToken, Task> taskFactory, CancellationToken cancellationToken) {
            return TaskUtilities.WhenAllCancelOnFailure(sessions, async (s, ct) => {
                try {
                    await taskFactory(s, ct);
                } catch (ObjectDisposedException) when (s.IsDisposed) {
                    ct.ThrowIfCancellationRequested();
                } catch (OperationCanceledException) when (s.IsDisposed) {
                    ct.ThrowIfCancellationRequested();
                }
            }, cancellationToken);
        }

        private IBrokerClient CreateBrokerClient(string name, BrokerConnectionInfo connectionInfo) {
            if (!connectionInfo.IsValid) {
                var installSvc = _services.GetService<IRInstallationService>();
                var path = installSvc.GetCompatibleEngines().FirstOrDefault()?.InstallPath;
                connectionInfo = BrokerConnectionInfo.Create(connectionInfo.Name, path, null);
            }

            if (!connectionInfo.IsValid) {
                return null;
            }

            return new LocalBrokerClient(name, connectionInfo, _services, _console, this);
        }
    }
}