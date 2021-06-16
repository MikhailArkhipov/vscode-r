// Copyright (c) Mikhail Arkhipov. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.R.Common.Core.OS;

namespace Microsoft.R.Platform.Interpreters {
    public abstract class RInstallationService : IRInstallationService {
        protected ISupportedRVersionRange SupportedVersions {get;}
        protected RInstallationService() {
            var a = Environment.GetEnvironmentVariable("LS_HOST_PROCESS_ARCHITECTURE");
            Architecture = string.IsNullOrWhiteSpace(a) ? ArchitectureString.X64 : a;
            SupportedVersions = new SupportedRVersionRange(3, 2, 4, 9);
        }

        public abstract IRInterpreterInfo CreateInfo(string name, string path);
        public abstract IEnumerable<IRInterpreterInfo> GetCompatibleEngines();

        /// <summary>
        /// Retries latest installed R for the current CPU architecture.
        /// </summary>
        public IRInterpreterInfo GetLatest()
            => GetCompatibleEngines().OrderByDescending(x => x.Version).FirstOrDefault(x => x.Architecture == Architecture);
        public string Architecture { get; }
    }
}
