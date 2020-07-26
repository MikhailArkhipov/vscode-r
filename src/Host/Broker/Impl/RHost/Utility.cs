// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Microsoft.Common.Core.OS;

namespace Microsoft.R.Host.Broker.RHost {
    public class Utility {
        public static IProcess RunAsCurrentUser(IProcessServices ps, string hostBinPath, string arguments, string rHomePath, string loadLibPath) {
            var psi = new ProcessStartInfo {
                FileName = hostBinPath,
                Arguments = arguments,
                RedirectStandardError = true,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                WorkingDirectory = Environment.GetEnvironmentVariable("PWD")
            };

            // All other should be same as the broker environment. Only these are set based on interpreters. 
            // R_HOME is explicitly set on the R-Host.
            psi.Environment.Add("R_HOME", rHomePath);
            psi.Environment.Add("LD_LIBRARY_PATH", loadLibPath);

            return ps.Start(psi);
        }
    }
}
