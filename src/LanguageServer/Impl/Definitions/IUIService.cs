// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Threading.Tasks;
using Microsoft.Common.Core.UI;

namespace Microsoft.R.LanguageServer {
    /// <summary>
    /// Service that represents the application user interface.
    /// </summary>
    public interface IHostUIService: IUIService {
        /// <summary>
        /// Displays message in a host-specific UI
        /// </summary>
        Task ShowMessageAsync(string message, MessageType messageType);

        /// <summary>
        /// Displays message with specified buttons in a host-specific UI
        /// </summary>
        Task<MessageActionItem> ShowMessageAsync(string message, MessageActionItem[] actions, MessageType messageType);

        /// <summary>
        /// Writes message to the host application output log
        /// </summary>
        /// <param name="message"></param>
        /// <param name="messageType"></param>
        Task LogMessageAsync(string message, MessageType messageType);

        /// <summary>
        /// Writes message to the host application status bar
        /// </summary>
        /// <param name="message"></param>
        Task SetStatusBarMessageAsync(string message);

        /// <summary>
        /// Sets log level for output in the host application.
        /// </summary>
        /// <param name="logLevel"></param>
        void SetLogLevel(MessageType logLevel);
    }
}
