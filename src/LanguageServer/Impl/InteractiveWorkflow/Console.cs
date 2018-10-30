// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core.Services;
using Microsoft.R.Host.Client;

namespace Microsoft.R.LanguageServer.InteractiveWorkflow {
    internal sealed class Console : IConsole {
        private readonly IUIService _ui;

        public Console(IServiceContainer services) {
            _ui = services.GetService<IUIService>();
        }

        public void WriteError(string text) => _ui.LogMessage(text, MessageType.Error);
        public void WriteErrorLine(string text) => _ui.LogMessage(text, MessageType.Error);
        public void Write(string text) => _ui.LogMessage(text, MessageType.Info);
        public void WriteLine(string text) => _ui.LogMessage(text, MessageType.Info);
        public Task<bool> PromptYesNoAsync(string text, CancellationToken cancellationToken) => Task.FromResult(true);
    }
}
