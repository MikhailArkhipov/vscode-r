﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.Common.Core.Tasks {
    public sealed class EventTaskSource<T> : EventTaskSource<T, EventHandler, EventArgs> {
        public EventTaskSource(Action<T, EventHandler> subscribe, Action<T, EventHandler> unsubscribe)
            : base(subscribe, unsubscribe, a => (o, e) => a(o, e)) {
        }
    }

    public class EventTaskSource<T, TEventArgs> : EventTaskSource<T, EventHandler<TEventArgs>, TEventArgs> {
        public EventTaskSource(Action<T, EventHandler<TEventArgs>> subscribe, Action<T, EventHandler<TEventArgs>> unsubscribe)
            : base(subscribe, unsubscribe, a => (o, e) => a(o, e)) {
        }
    }

    public class EventTaskSource<T, TEventHandler, TEventArgs> {
        private readonly Action<T, TEventHandler> _subscribe;
        private readonly Action<T, TEventHandler> _unsubscribe;
        private readonly Func<Action<object, TEventArgs>, TEventHandler> _handlerConverter;

        public EventTaskSource(Action<T, TEventHandler> subscribe, Action<T, TEventHandler> unsubscribe, Func<Action<object, TEventArgs>, TEventHandler> handlerConverter) {
            _subscribe = subscribe;
            _unsubscribe = unsubscribe;
            _handlerConverter = handlerConverter;
        }

        public Task<TEventArgs> Create(T instance, CancellationToken cancellationToken = default) => Create(instance, null, cancellationToken);

        public Task<TEventArgs> Create(T instance, Action<TEventArgs> callback, CancellationToken cancellationToken = default) {
            var tcs = new TaskCompletionSource<TEventArgs>();
            var reference = new HandlerReference(instance, tcs, _unsubscribe, _handlerConverter, callback);
            if (cancellationToken != CancellationToken.None) {
                cancellationToken.Register(reference.Cancel);
            }
            _subscribe(instance, reference.Handler);
            return tcs.Task;
        }

        private class HandlerReference {
            private T _instance;
            private TaskCompletionSource<TEventArgs> _tcs;
            private Action<T, TEventHandler> _unsubscribe;
            private readonly Action<TEventArgs> _callback;
            public TEventHandler Handler { get; }

            public HandlerReference(T instance, TaskCompletionSource<TEventArgs> tcs, Action<T, TEventHandler> unsubscribe, Func<Action<object, TEventArgs>, TEventHandler> handlerConverter, Action<TEventArgs> callback) {
                _instance = instance;
                _tcs = tcs;
                _unsubscribe = unsubscribe;
                _callback = callback;
                Handler = handlerConverter(TypedHandler);
            }

            public void Cancel() {
                var tcs = Unsubscribe();
                tcs?.SetCanceled();
            }

            private void TypedHandler(object sender, TEventArgs e) {
                var tcs = Unsubscribe();
                _callback?.Invoke(e);
                tcs?.SetResult(e);
            }

            private TaskCompletionSource<TEventArgs> Unsubscribe() {
                var tcs = Interlocked.Exchange(ref _tcs, null);
                if (tcs == null) {
                    return null;
                }

                var instance = _instance;
                var unsubscribe = _unsubscribe;
                _instance = default;
                _unsubscribe = null;
                unsubscribe(instance, Handler);
                return tcs;
            }
        }
    }
}
