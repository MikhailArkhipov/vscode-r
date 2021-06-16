// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.Common.Core.IO;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.OS;
using Microsoft.Common.Core.Services;
using Microsoft.Common.Core.Tasks;
using Microsoft.Common.Core.Threading;
using Microsoft.Common.Core.UI;

namespace Microsoft.Common.Core.Shell {
    public static class ShellExtensions {
        public static T GetService<T>(this ICoreShell shell, Type type = null) where T: class  => shell.Services.GetService<T>(type);
        public static IActionLog Log(this ICoreShell shell) => shell.Services.Log();
        public static IFileSystem FileSystem(this ICoreShell shell) => shell.Services.FileSystem();
        public static IProcessServices Process(this ICoreShell shell) => shell.Services.Process();
        public static ITaskService Tasks(this ICoreShell shell) => shell.Services.Tasks();
        public static IUIService UI(this ICoreShell shell) => shell.Services.UI();
        public static IMainThread MainThread(this ICoreShell shell) => shell.Services.MainThread();
       
    }
}
