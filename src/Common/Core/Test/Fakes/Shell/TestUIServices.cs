// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Common.Core.UI;

namespace Microsoft.Common.Core.Test.Fakes.Shell {
    public sealed class TestUIServices: IUIService {
        public void ShowErrorMessage(string message)=> LastShownErrorMessage = message;
        public void LogMessage(string message, MessageType messageType = MessageType.Information) { }

        public string LastShownMessage { get; private set; }
        public string LastShownErrorMessage { get; private set; }
    }
}
