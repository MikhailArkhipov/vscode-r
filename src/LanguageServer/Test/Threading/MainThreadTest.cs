// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using FluentAssertions;
using Microsoft.R.LanguageServer.Threading;
using Microsoft.UnitTests.Core.XUnit;

namespace Microsoft.R.LanguageServer.Test.Text {
    [Category.VsCode.Threading]
    public sealed class MainThreadTest: IDisposable {
        private readonly MainThread _mt = new MainThread();

        public void Dispose() {
            _mt.Dispose();
        }
    }
}
