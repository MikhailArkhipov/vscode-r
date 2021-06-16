// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using Microsoft.Common.Core;
using Microsoft.Common.Core.IO;
using Microsoft.R.Common.Core.OS;
using Microsoft.R.Platform.IO;

namespace Microsoft.R.Platform.Interpreters.Mac {
    public sealed class RMacInstallation : RInstallationService {
        private const string RootPath = "/Library/Frameworks/R.framework/Versions/";
        private readonly IFileSystem _fileSystem;

        public RMacInstallation() :
            this(new UnixFileSystem()) { }

        public RMacInstallation(IFileSystem fileSystem) {
            _fileSystem = fileSystem;
        }

        public override IRInterpreterInfo CreateInfo(string name, string path) {
            var version = VersionFromPath(path, out var versionString, out var architecture);
            return version != null ? new RMacInterpreterInfo("R " + versionString, versionString, architecture, version, _fileSystem) : null;
        }

        public override IEnumerable<IRInterpreterInfo> GetCompatibleEngines() {
            var rFrameworkPath = Path.Combine("/Library/Frameworks/R.framework/Versions");

            foreach (var path in _fileSystem.GetDirectories(rFrameworkPath)) {
                var version = VersionFromPath(path, out var versionString, out var architecture);
                if (version != null && SupportedVersions.IsCompatibleVersion(version)) {
                    yield return new RMacInterpreterInfo("R " + versionString, versionString, architecture, version, _fileSystem);
                }
            }
        }

        private static Version VersionFromPath(string path, out string versionString, out string architecture) {
            versionString = null;
            architecture = ArchitectureString.X64;

            if (!path.StartsWithOrdinal(RootPath)) {
                return null;
            }
            var resPath = path.Substring(RootPath.Length, path.Length - RootPath.Length);
            var index = resPath.IndexOf("/Resources");
            versionString = index > 0 ? resPath.Substring(0, index) : resPath;

            // On Mac M1 R ARM64 version string looks like '4.1-arm64'. Truncate appropriately.
            var vs = versionString;
            var dashIndex = vs.IndexOf('-');
            if (dashIndex > 0) {
                architecture = vs.Substring(dashIndex + 1);
                vs = vs.Substring(0, dashIndex);
            }
            return Version.TryParse(vs, out var version) ? version : null;
        }
    }
}