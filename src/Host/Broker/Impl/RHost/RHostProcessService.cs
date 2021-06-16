// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using Microsoft.Common.Core.IO;
using Microsoft.Common.Core.OS;
using Microsoft.R.Host.Broker.Services;
using Microsoft.R.Platform.Host;

namespace Microsoft.R.Host.Broker.RHost {
    internal class RHostProcessService : IRHostProcessService {
        private readonly IFileSystem _fs;
        private readonly IProcessServices _ps;

        public RHostProcessService(IFileSystem fs, IProcessServices ps) {
            _fs = fs;
            _ps = ps;
        }

        public IProcess StartHost(string interpreterPath, string interpreterArchitecture, string commandLine) {
            var exeLocator = new BrokerExecutableLocator(_fs);

            var hostBinPath = exeLocator.GetHostExecutablePath(interpreterArchitecture);
            if(hostBinPath == null) {
                throw new Win32Exception($"Microsoft.R.Host is missing. Interpreter: {interpreterPath}. Architecture: {interpreterArchitecture}");
            }

            var psi = new ProcessStartInfo {
                FileName = hostBinPath,
                Arguments = commandLine,
                RedirectStandardError = true,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                WorkingDirectory = Environment.GetEnvironmentVariable("PWD")
            };

            psi.Environment.Add("R_HOME", interpreterPath);
            psi.Environment.Add("LD_LIBRARY_PATH", Path.Combine(interpreterPath, "lib"));

            var process = _ps.Start(psi);

            process.WaitForExit(250);
            if (process.HasExited && process.ExitCode != 0) {
                throw new Win32Exception(process.ExitCode);
            }
            return process;
        }
    }
}