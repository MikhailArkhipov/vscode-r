// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Threading;

namespace Microsoft.Common.Core.Threading {
    public static class MainThreadExtensions {
        public static MainThreadAwaitable SwitchToAsync(this IMainThread mainThread, CancellationToken cancellationToken = default)
            => new MainThreadAwaitable(mainThread, cancellationToken);

        [Conditional("TRACE")]
        public static void Assert(this IMainThread mainThread, [CallerMemberName] string memberName = "", [CallerFilePath] string sourceFilePath = "", [CallerLineNumber] int sourceLineNumber = 0) {
            if (mainThread.ThreadId != Thread.CurrentThread.ManagedThreadId) {
                Debug.Fail(FormattableString.Invariant($"{memberName} at {sourceFilePath}:{sourceLineNumber} was incorrectly called from a background thread."));
            }
        }
    }
}