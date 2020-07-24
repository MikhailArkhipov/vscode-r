﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core.Tasks;

namespace Microsoft.R.LanguageServer.Threading {
    internal sealed class TaskService: ITaskService {
        public bool Wait(Func<Task> method, int ms = Timeout.Infinite, CancellationToken cancellationToken = default) {
            var delayTask = Task.Delay(ms, cancellationToken);
            var resultTask = Run(method, delayTask);
            return resultTask != delayTask;
        }

        public bool Wait<T>(Func<Task<T>> method, out T result, int ms = Timeout.Infinite, CancellationToken cancellationToken = default) {
            var delayTask = Task.Delay(ms, cancellationToken);
            var resultTask = Run(method, delayTask);
#pragma warning disable VSTHRD002 // Avoid problematic synchronous waits
            result = resultTask is Task<T> task ? task.GetAwaiter().GetResult() : default;
#pragma warning restore VSTHRD002 // Avoid problematic synchronous waits
            return resultTask != delayTask;
        }

        public Task Run(Func<Task> method, Task delayTask) => Task.WhenAny(method(), delayTask);
    }
}
