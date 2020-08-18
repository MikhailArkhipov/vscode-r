﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Microsoft.R.Platform.OS;

namespace Microsoft.R.Platform.Windows.OS {
    public sealed class WindowsProcessServices : ProcessServices {
        protected override void KillProcess(int pid) {
            try {
                (Process.GetProcessById(pid)).Kill();
            } catch (ArgumentException) { }
        }
    }
}
