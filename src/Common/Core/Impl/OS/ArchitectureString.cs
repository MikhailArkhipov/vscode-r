// Copyright (c) Mikhail Arkhipov. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.R.Common.Core.OS {
    public static class ArchitectureString {
        /// <summary>
        /// Intel 64-bit (x86-64).
        /// </summary>
        public const string X64 = "x64";
        
        /// <summary>
        /// ARM 64-bit. Such as Apple M1.
        /// </summary>
        public const string Arm64 = "arm64";
    }
}
