// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.R.Platform.OS;

namespace Microsoft.Common.Core.OS.Mac {
    public sealed class MacProcessServices : ProcessServices {
        protected override void KillProcess(int pid) 
            => Process.GetProcessById(pid)?.Kill();
    }
}
