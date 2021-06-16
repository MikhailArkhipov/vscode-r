// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using Microsoft.Common.Core;
using Microsoft.Common.Core.IO;
using Microsoft.R.Platform.IO;
using Microsoft.R.Platform.OS.Linux;

namespace Microsoft.R.Platform.Interpreters.Linux {
    public sealed class RLinuxInstallation : RInstallationService {
        private readonly IFileSystem _fileSystem;

        public RLinuxInstallation() :
            this(new UnixFileSystem()) {
        }

        public RLinuxInstallation(IFileSystem fileSystem)  {
            _fileSystem = fileSystem;
        }

        public override IRInterpreterInfo CreateInfo(string name, string path) {
            var packagesInfo = InstalledPackageInfo.GetPackages(_fileSystem);
            var libRsoPath = Path.Combine(path, "lib/libR.so").Replace('\\', '/');

            // In linux there is no direct way to get version from binary. So, try and find a package that 
            // has this file in the package files list. 
            InstalledPackageInfo package = null;
            foreach (var pkg in packagesInfo) {
                var files = pkg.GetPackageFiles(_fileSystem);
                if (files.Contains(libRsoPath)) {
                    package = pkg;
                    break;
                }
            }

            if(package != null) {
                return new RLinuxInterpreterInfo(name, package, package.Version, package.GetVersion(), _fileSystem);
            }
            return null;
        }

        public override IEnumerable<IRInterpreterInfo> GetCompatibleEngines() {
            var packagesInfo = InstalledPackageInfo.GetPackages(_fileSystem);
            var interpreters = new List<IRInterpreterInfo>();

            interpreters.AddRange(GetInstalledMRO(packagesInfo));
            interpreters.AddRange(GetInstalledCranR(packagesInfo));
            return interpreters;
        }

        private IEnumerable<IRInterpreterInfo> GetInstalledMRO(IEnumerable<InstalledPackageInfo> packagesInfo) {
            var selectedPackages = packagesInfo.Where(p => p.PackageName.StartsWithIgnoreCase("microsoft-r-open-mro") && SupportedVersions.IsCompatibleVersion(p.GetVersion()));
            foreach (var package in selectedPackages) {
                yield return RLinuxInterpreterInfo.CreateFromPackage(package, "Microsoft R Open", _fileSystem);
            }
        }

        private IEnumerable<IRInterpreterInfo> GetInstalledCranR(IEnumerable<InstalledPackageInfo> packagesInfo) {
            var selectedPackages = packagesInfo.Where(p => p.PackageName.EqualsIgnoreCase("r-base-core") && SupportedVersions.IsCompatibleVersion(p.GetVersion()));
            foreach (var package in selectedPackages) {
                yield return RLinuxInterpreterInfo.CreateFromPackage(package, "CRAN R", _fileSystem);
            }
        }
    }
}
