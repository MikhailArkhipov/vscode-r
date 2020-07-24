﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.Common.Core.Threading {
    /// <summary>
    /// Async version of the ManualResetEvent
    /// </summary>
    public class AsyncManualResetEvent {
        private TaskCompletionSource<bool> _tcs;
        public bool IsSet => _tcs.Task.IsCompleted;

        public void Set() => _tcs.TrySetResult(true);

        public Task WaitAsync(CancellationToken cancellationToken = default) {
            if (!cancellationToken.CanBeCanceled) {
                return _tcs.Task;
            }

            if (cancellationToken.IsCancellationRequested) {
                return Task.FromCanceled(cancellationToken);
            }

            var tcs = new TaskCompletionSource<bool>();
            cancellationToken.Register(CancelTcs, tcs);
            _tcs.Task.ContinueWith(WaitContinuation, tcs, TaskContinuationOptions.ExecuteSynchronously);
            return tcs.Task;
        }

        private void WaitContinuation(Task<bool> task, object state) {
            var tcs = (TaskCompletionSource<bool>) state;
            switch (task.Status) {
                case TaskStatus.Faulted:
                    tcs.TrySetException(task.Exception);
                    break;
                case TaskStatus.Canceled:
                    tcs.TrySetCanceled();
                    break;
                case TaskStatus.RanToCompletion:
                    tcs.TrySetResult(task.Result);
                    break;
            }
        }

        public AsyncManualResetEvent(bool isSet = false) {
            _tcs = new TaskCompletionSource<bool>();
            if (isSet) {
                _tcs.SetResult(true);
            }
        }

        public void Reset() {
            while (true) {
                var tcs = _tcs;
                if (!tcs.Task.IsCompleted) {
                    return;
                }

                if (Interlocked.CompareExchange(ref _tcs, new TaskCompletionSource<bool>(), tcs) == tcs) {
                    return;
                }
            }
        }

        private static void CancelTcs(object obj) {
            var tcs = (TaskCompletionSource<bool>) obj;
            tcs.TrySetCanceled();
        }
    }
}