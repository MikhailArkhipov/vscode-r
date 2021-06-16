// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using Microsoft.Common.Core.OS.Linux;
using Microsoft.Common.Core.OS.Mac;
using Microsoft.Common.Core.Services;
using Microsoft.R.Platform.Interpreters;
using Microsoft.R.Platform.Interpreters.Linux;
using Microsoft.R.Platform.Interpreters.Mac;
using Microsoft.R.Platform.IO;
using Microsoft.R.Platform.Windows;
using Microsoft.R.Platform.Windows.Interpreters;
using Microsoft.R.Platform.Windows.IO;
using Microsoft.R.Platform.Windows.OS;
using Microsoft.R.Platform.Windows.Registry;

namespace Microsoft.R.Platform {
    /// <summary>
    /// Invoked via reflection to populate service container
    /// with platform-specific services such as R discovery,
    /// file system, process management.
    /// </summary>
    public static class PlatformServiceProvider {
        public static void AddPlatformSpecificServices(IServiceManager services) {
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
                services
                    .AddService(new WindowsFileSystem())
                    .AddService(new WindowsProcessServices())
                    .AddService(new RegistryImpl())
                    .AddService<IRInstallationService, WindowsRInstallation>()
                    .AddService(new WindowsPlatformServices());
            } else {
                var fs = new UnixFileSystem();
                services
                    .AddService(new UnixFileSystem());

                if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX)) {
                    services
                        .AddService(new MacProcessServices())
                        .AddService(new MacPlatformServices())
                        .AddService(new RMacInstallation(fs));
                } else {
                    services
                        .AddService(new LinuxProcessServices())
                        .AddService(new LinuxPlatformServices())
                        .AddService(new RLinuxInstallation(fs));
                }
            }
        }
    }
}
