// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;

namespace Microsoft.R.Platform.Windows {
    internal static class NativeMethods {
        public const uint FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100;
        public const uint FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        public const uint FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000;

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        public static extern uint FormatMessage(uint dwFlags, IntPtr lpSource, int dwMessageId,
             uint dwLanguageId, ref IntPtr lpBuffer, uint nSize, IntPtr pArguments);

        [DllImport("ntdll.dll")]
        public static extern int RtlNtStatusToDosError(int Status);

        public static readonly Guid KNOWNFOLDERID_LocalAppData = new Guid("F1B32785-6FBA-4FCF-9D55-7B8E7F157091");

        [DllImport("Shell32.dll")]
        public static extern int SHGetKnownFolderPath([MarshalAs(UnmanagedType.LPStruct)] Guid folderId, uint dwFlags, IntPtr token, out IntPtr path);
    }
}
