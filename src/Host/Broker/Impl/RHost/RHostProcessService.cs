// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Claims;
using Microsoft.Common.Core.IO;
using Microsoft.Common.Core.OS;
using Microsoft.R.Host.Broker.Interpreters;
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

        public IProcess StartHost(Interpreter interpreter, string commandLine) {
            var exeLocator = BrokerExecutableLocator.Create(_fs);
            var hostBinPath = exeLocator.GetHostExecutablePath();
            if(!_fs.FileExists(hostBinPath) && RuntimeInformation.IsOSPlatform(OSPlatform.Linux)) {
                hostBinPath = PathConstants.RunHostBinPath;
            }

            var process = Utility.RunAsCurrentUser(_ps, hostBinPath, commandLine, GetRHomePath(interpreter), GetLoadLibraryPath(interpreter));
            process.WaitForExit(250);
            if (process.HasExited && process.ExitCode != 0) {
                throw new Win32Exception(process.ExitCode);
            }
            return process;
        }

        protected virtual string GetRHomePath(Interpreter interpreter) => interpreter.RInterpreterInfo.InstallPath;

        protected virtual string GetRHostBinaryPath() {
            var locator = BrokerExecutableLocator.Create(_fs);
            return locator.GetHostExecutablePath();
        }

        protected virtual string GetLoadLibraryPath(Interpreter interpreter) {
            var value = Environment.GetEnvironmentVariable("LD_LIBRARY_PATH");
            return !string.IsNullOrEmpty(value) ? value : Path.Combine(interpreter.RInterpreterInfo.InstallPath, "lib");
        }
    }
}