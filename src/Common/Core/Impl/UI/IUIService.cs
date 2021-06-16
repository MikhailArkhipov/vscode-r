// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.Common.Core.UI {
    /// <summary>
    /// Basic UI support.
    /// </summary>
    public interface IUIService {
        /// <summary>
        /// Displays error message in a host-specific UI
        /// </summary>
        void ShowErrorMessage(string message);

        /// <summary>
        /// Logs message in a host-specific output window.
        /// </summary>
        void LogMessage(string message, MessageType messageType = MessageType.Information);
    }
}
