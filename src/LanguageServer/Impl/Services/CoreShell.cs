// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.Common.Core.Diagnostics;
using Microsoft.Common.Core.Services;
using Microsoft.Common.Core.Shell;

namespace Microsoft.R.LanguageServer.Services {
    internal sealed class CoreShell : ICoreShell, IDisposable {
        public static CoreShell Current { get; private set; }
        public IServiceManager ServiceManager { get; } = new ServiceContainer();
        public IServiceContainer Services => ServiceManager;

        public static IDisposable Create() {
            Check.InvalidOperation(() => Current == null);
            Current = new CoreShell();
            return Current;
        }

        private CoreShell() {
            ServiceManager.AddService(this);
        }

        public void Dispose() {
            ServiceManager?.RemoveService(this);
            ServiceManager?.Dispose();
        }
    }
}
