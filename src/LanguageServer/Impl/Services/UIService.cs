// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using StreamJsonRpc;

namespace Microsoft.R.LanguageServer.Services {
    public sealed class UIService : IUIService {
        private readonly JsonRpc _rpc;
        private MessageType _logLevel = MessageType.Error;

        public UIService(JsonRpc rpc) {
            _rpc = rpc;
        }
        public Task ShowMessageAsync(string message, MessageType messageType) {
            var parameters = new ShowMessageRequestParams {
                type = messageType,
                message = message
            };
            return _rpc.NotifyWithParameterObjectAsync("window/showMessage", parameters);
        }

        public Task<MessageActionItem> ShowMessageAsync(string message, MessageActionItem[] actions, MessageType messageType) {
            var parameters = new ShowMessageRequestParams {
                type = messageType,
                message = message,
                actions = actions
            };
            return _rpc.InvokeWithParameterObjectAsync<MessageActionItem>("window/showMessageRequest", parameters);
        }

        [Serializable]
        class LogMessageParams {
            public MessageType type;
            public string message;
        }

        public Task LogMessageAsync(string message, MessageType messageType) {
            if(messageType > _logLevel) {
                return Task.CompletedTask;
            }
            var parameters = new LogMessageParams {
                type = messageType,
                message = message
            };
            return _rpc.NotifyWithParameterObjectAsync("window/logMessage", parameters);
        }

        public Task SetStatusBarMessageAsync(string message) 
            => _rpc.NotifyWithParameterObjectAsync("window/setStatusBarMessage", message);

        public void TraceMessage(string message) => LogMessageAsync(message.ToString(), MessageType.Info).DoNotWait();
        public void TraceMessage(IFormattable message) => TraceMessage(message.ToString());

        public void SetLogLevel(MessageType logLevel) => _logLevel = logLevel;
    }
}
