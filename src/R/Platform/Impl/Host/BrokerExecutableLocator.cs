// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using Microsoft.Common.Core;
using Microsoft.Common.Core.IO;
using Microsoft.R.Common.Core.OS;

namespace Microsoft.R.Platform.Host {
    public sealed class BrokerExecutableLocator {
        public const string BrokerName = "Microsoft.R.Host.Broker.dll";
        public const string HostName = "Microsoft.R.Host";
        private readonly IFileSystem _fs;

        public BrokerExecutableLocator(IFileSystem fs) {
            _fs = fs;
            BaseDirectory = Path.GetDirectoryName(typeof(BrokerExecutableLocator).GetTypeInfo().Assembly.GetAssemblyPath());
        }

        public string BaseDirectory { get; }

        public string GetBrokerExecutablePath() {
            var path = Path.Combine(BaseDirectory, BrokerName);
            return _fs.FileExists(path) ? path : null;
        }

        public string GetHostExecutablePath(string architecture) {
            var path = Path.Combine(BaseDirectory, GetHostMultiplatformSubPath(architecture));
            return _fs.FileExists(path) ? path : null;
        }

        private string GetHostMultiplatformSubPath(string architecture) {

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
                return Path.Combine("Host", "Windows", HostName + ".exe");
            }
            
            if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX)) {
                var folder = string.Equals(architecture, ArchitectureString.Arm64, StringComparison.OrdinalIgnoreCase)
                    ? Path.Combine("Mac", ArchitectureString.Arm64)
                    : Path.Combine("Mac", ArchitectureString.X64);
                return Path.Combine("Host", folder, HostName);
            }

            return Path.Combine("Host", "Linux", HostName);
        }
    }
}
