﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.Threading;

namespace Microsoft.R.LanguageServer.Threading {
    internal static class MainThreadExtensions {
        /// <summary>
        /// Executed cancellable action on UI thread.
        /// </summary>
        /// <param name="mainThread"></param>
        /// <param name="action"></param>
        /// <param name="cancellationToken"></param>
        public static async Task SendAsync(this IMainThread mainThread, Action action, IUIService ui, CancellationToken cancellationToken = default) {
            await mainThread.SwitchToAsync(cancellationToken);
            try {
                action();
            } catch (OperationCanceledException) {
                throw;
            } catch (Exception ex) {
                ui.LogMessageAsync($"Exception {ex.Message} at {ex.StackTrace}", MessageType.Error).DoNotWait();
                throw;
            }
        }

        /// <summary>
        /// Executed cancellable action on UI thread and return result.
        /// </summary>
        /// <param name="mainThread"></param>
        /// <param name="action"></param>
        /// <param name="cancellationToken"></param>
        public static async Task<T> SendAsync<T>(this IMainThread mainThread, Func<T> action, IUIService ui, CancellationToken cancellationToken = default) {
            await mainThread.SwitchToAsync(cancellationToken);
            try {
                return action();
            } catch (OperationCanceledException) {
                throw;
            } catch (Exception ex) {
                ui.LogMessageAsync($"Exception {ex.Message} at {ex.StackTrace}", MessageType.Error).DoNotWait();
                throw;
            }
        }
    }
}
