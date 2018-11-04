// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using Microsoft.Common.Core;
using Microsoft.Common.Core.IO;

namespace Microsoft.R.Platform.Host {
    public sealed class BrokerExecutableLocator {
        public const string BrokerName = "Microsoft.R.Host.Broker.dll";
        public const string HostName = "Microsoft.R.Host";

        private readonly IFileSystem _fs;
        private readonly OSPlatform _platform;

        public static BrokerExecutableLocator Create(IFileSystem fs) {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
                return new BrokerExecutableLocator(fs, OSPlatform.Windows);
            }
            if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX)) {
                return new BrokerExecutableLocator(fs, OSPlatform.OSX);
            }
            return new BrokerExecutableLocator(fs, OSPlatform.Linux);
        }

        public BrokerExecutableLocator(IFileSystem fs, OSPlatform platform) {
            _fs = fs;
            _platform = platform;
            BaseDirectory = Path.GetDirectoryName(typeof(BrokerExecutableLocator).GetTypeInfo().Assembly.GetAssemblyPath());
        }

        public string BaseDirectory { get; }

        public string GetBrokerExecutablePath() {
            var path = Path.Combine(BaseDirectory, BrokerName);
            return _fs.FileExists(path) ? path : null;
        }

        public string GetHostExecutablePath() {
            var path = Path.Combine(BaseDirectory, GetHostMultiplatformSubPath());
            return _fs.FileExists(path) ? path : null;
        }

        private string GetHostMultiplatformSubPath() {
            string folder;
            var ext = string.Empty;
            if(_platform == OSPlatform.Windows) {
                folder = "Windows";
                ext = ".exe";
            } else if(_platform == OSPlatform.OSX) {
                folder = "Mac";
            } else {  
                folder = "Linux";
            }
            return Path.Combine("Host", folder, HostName + ext);
        }
    }
}
