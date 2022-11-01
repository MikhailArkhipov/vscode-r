﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using System.Threading.Tasks.Dataflow;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Disposables;
using Microsoft.Common.Core.Threading;

namespace Microsoft.R.LanguageServer.Threading {
    /// <summary>
    /// Implements concept of a 'main' or 'UI' thread. In the language server 
    /// </summary>
    /// <remarks>
    /// for VS Code there is no UI thread and requests from the client can
    /// come at any thread. This causes problems with code that expects to 
    /// be called on the same thread that produced user activity.
    /// </remarks>
    internal sealed class MainThread : IMainThreadPriority, IDisposable {
        private readonly CancellationTokenSource _ctsExit = new CancellationTokenSource();
        private readonly ManualResetEventSlim _workItemsAvailable = new ManualResetEventSlim(false);
        private readonly object _lock = new object();
        private readonly BufferBlock<Action> _idleTimeQueue = new BufferBlock<Action>();
        private readonly BufferBlock<Action> _normalPriorityQueue = new BufferBlock<Action>();
        private readonly BufferBlock<Action> _backgroundPriorityQueue = new BufferBlock<Action>();
        private readonly DisposableBag _disposableBag = DisposableBag.Create<MainThread>();
        private volatile Action _idleOnceAction;

        public MainThread() {
            _disposableBag.Add(Stop);
            Thread.Start(this);
        }

        #region IMainThread
        public int ThreadId => Thread.ManagedThreadId;

        public void Post(Action action)
            => Execute(action, ThreadPostPriority.Normal);

        public IMainThreadAwaiter CreateMainThreadAwaiter(CancellationToken cancellationToken) 
            => new MainThreadAwaiter(this, cancellationToken);
        #endregion

        #region IMainThreadPriority
        public void Post(Action action, ThreadPostPriority priority)
            => Execute(action, priority);

        public Task<T> SendAsync<T>(Func<T> action, ThreadPostPriority priority, IHostUIService ui, CancellationToken ct = default) {
            var tcs = new TaskCompletionSource<T>();
            Execute(() => {
                try {
                    tcs.TrySetResult(action());
                } catch(OperationCanceledException) {
                    tcs.TrySetCanceled();
                } catch(Exception ex) {
                    ui.LogMessageAsync($"Exception {ex.Message} at {ex.StackTrace}", MessageType.Error).DoNotWait();
                    tcs.TrySetException(ex);
                }
            }, priority);

            return tcs.Task;
        }

        public Task<T> SendAsync<T>(Func<Task<T>> action, ThreadPostPriority priority, IHostUIService ui, CancellationToken ct = default) {
            var tcs = new TaskCompletionSource<T>();
#pragma warning disable VSTHRD101 // Avoid unsupported async delegates
            Execute(async () => {
                try {
                    tcs.TrySetResult(await action());
                } catch (OperationCanceledException) {
                    tcs.TrySetCanceled();
                } catch (Exception ex) {
                    ui.LogMessageAsync($"Exception {ex.Message} at {ex.StackTrace}", MessageType.Error).DoNotWait();
                    tcs.TrySetException(ex);
                }
            }, priority);
#pragma warning restore VSTHRD101 // Avoid unsupported async delegates

            return tcs.Task;
        }

        public void CancelIdle() => _idleOnceAction = null;
        #endregion

        #region IDisposable
        public void Dispose() => _disposableBag.TryDispose();

        private void Stop() {
            _ctsExit.Cancel();
            _normalPriorityQueue.Post(() => { });
            Thread.Join(1000);
        }
        #endregion

        private void Execute(Action action, ThreadPostPriority priority) {
            _disposableBag.ThrowIfDisposed();

            BufferBlock<Action> queue = null;
            if (priority == ThreadPostPriority.IdleOnce) {
                _idleOnceAction = action;
            } else {
                if (ThreadId == Thread.CurrentThread.ManagedThreadId) {
                    action();
                } else {
                    switch (priority) {
                        case ThreadPostPriority.Idle:
                            queue = _idleTimeQueue;
                            break;
                        case ThreadPostPriority.Background:
                            queue = _backgroundPriorityQueue;
                            break;
                        case ThreadPostPriority.Normal:
                            queue = _normalPriorityQueue;
                            break;
                    }
                }
            }

            lock (_lock) {
                queue?.Post(action);
                _workItemsAvailable.Set();
            }
        }

        internal Thread Thread { get; } = new Thread(o => ((MainThread)o).WorkerThread());

        private void WorkerThread() {
            while (!_ctsExit.IsCancellationRequested) {
                Action action;

                try {
                    _workItemsAvailable.Wait(_ctsExit.Token);
                } catch(OperationCanceledException) {
                    break;
                }

                if(_ctsExit.IsCancellationRequested) {
                    break;
                }

                while (!_ctsExit.IsCancellationRequested && 
                       _normalPriorityQueue.TryReceive(out action)) {
                    ProcessAction(action);
                }

                while (!_ctsExit.IsCancellationRequested &&
                       _normalPriorityQueue.Count == 0 &&
                       _backgroundPriorityQueue.TryReceive(out action)) {
                    ProcessAction(action);
                }

                while (!_ctsExit.IsCancellationRequested &&
                       _normalPriorityQueue.Count == 0 &&
                       _backgroundPriorityQueue.Count == 0 && 
                       _idleTimeQueue.TryReceive(out action)) {
                    ProcessAction(action);
                }

                while (!_ctsExit.IsCancellationRequested &&
                       _normalPriorityQueue.Count == 0 &&
                       _backgroundPriorityQueue.Count == 0) {
                    action = _idleOnceAction;
                    _idleOnceAction = null;
                    if (action == null) {
                        break;
                    }
                    ProcessAction(action);
                }

                lock(_lock) {
                    if(_normalPriorityQueue.Count == 0 && _backgroundPriorityQueue.Count == 0 && 
                       _idleTimeQueue.Count == 0 && _idleOnceAction == null) {
                        _workItemsAvailable.Reset();
                    }
                }
            }
        }

        private void ProcessAction(Action action) {
            if (!_ctsExit.IsCancellationRequested) {
                action();
            }
        }

        class MainThreadAwaiter : IMainThreadAwaiter {
            private readonly IMainThread _mainThread;
            private readonly CancellationToken _cancellationToken;

            public MainThreadAwaiter(IMainThread mainThread, CancellationToken cancellationToken) {
                _mainThread = mainThread;
                _cancellationToken = cancellationToken;
            }

            public bool IsCompleted => Thread.CurrentThread.ManagedThreadId == _mainThread.ThreadId;

            public void GetResult() {
                if (Thread.CurrentThread.ManagedThreadId != _mainThread.ThreadId) {
                    _cancellationToken.ThrowIfCancellationRequested();
                }
            }

            public void OnCompleted(Action continuation) => _mainThread.Post(() => continuation());
        }
    }
}
