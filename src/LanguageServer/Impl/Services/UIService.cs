// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using StreamJsonRpc;

namespace Microsoft.R.LanguageServer.Services {
    public sealed class UIService : IUIService {
        private readonly JsonRpc _rpc;
        private MessageType _logLevel = MessageType.Error;

        public UIService(JsonRpc rpc) {
            _rpc = rpc;
        }
        public Task ShowMessage(string message, MessageType messageType) {
            var parameters = new ShowMessageRequestParams {
                type = messageType,
                message = message
            };
            return _rpc.NotifyWithParameterObjectAsync("window/showMessage", parameters);
        }

        public Task<MessageActionItem?> ShowMessage(string message, MessageActionItem[] actions, MessageType messageType) {
            var parameters = new ShowMessageRequestParams {
                type = messageType,
                message = message,
                actions = actions
            };
            return _rpc.InvokeWithParameterObjectAsync<MessageActionItem?>("window/showMessageRequest", parameters);
        }

        [Serializable]
        class LogMessageParams {
            public MessageType type;
            public string message;
        }

        public Task LogMessage(string message, MessageType messageType) {
            if(messageType > _logLevel) {
                return Task.CompletedTask;
            }
            var parameters = new LogMessageParams {
                type = messageType,
                message = message
            };
            return _rpc.NotifyWithParameterObjectAsync("window/logMessage", parameters);
        }

        public Task SetStatusBarMessage(string message) 
            => _rpc.NotifyWithParameterObjectAsync("window/setStatusBarMessage", message);

        public void TraceMessage(string message) => LogMessage(message.ToString(), MessageType.Info);
        public void TraceMessage(IFormattable message) => TraceMessage(message.ToString());

        public void SetLogLevel(MessageType logLevel) => _logLevel = logLevel;
    }
}
