﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Threading.Tasks;

namespace Microsoft.R.Host.Client.Session {
    internal sealed class RSessionInteraction : IRSessionInteraction {
        private readonly TaskCompletionSource<string> _requestTcs;
        private readonly TaskCompletionSource<object> _responseTcs;

        public string Prompt { get; }
        public int MaxLength { get; }
        public IReadOnlyList<IRContext> Contexts { get; }

        public RSessionInteraction(
            TaskCompletionSource<string> requestTcs,
            TaskCompletionSource<object> responseTcs,
            string prompt,
            int maxLength,
            IReadOnlyList<IRContext> contexts) {
            _requestTcs = requestTcs;
            _responseTcs = responseTcs;
            Prompt = prompt;
            MaxLength = maxLength;
            Contexts = contexts;
        }

        public Task RespondAsync(string messageText) {
            _requestTcs.TrySetResult(messageText);
            return _responseTcs.Task;
        }

        public void Dispose() {
            _requestTcs.TrySetCanceled();
        }
    }
}