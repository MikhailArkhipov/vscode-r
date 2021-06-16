﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.R.Host.Protocol {
    public struct SessionCreateRequest {
        public string InterpreterPath { get; set; }
        public string InterpreterArchitecture { get; set; }
        public string CommandLineArguments { get; set; }
        public bool IsInteractive { get; set; }
    }
}
