﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core.UI;
using Microsoft.R.Host.Client;

namespace Microsoft.R.LanguageServer.InteractiveWorkflow {
    internal sealed class RSessionCallback : IRSessionCallback {
        internal byte[] PlotResult { get; set; }
        internal PlotDeviceProperties PlotDeviceProperties { get; set; }
            = new PlotDeviceProperties(800, 600, 96); // Typical RMD Document has CSS width of 900

        public string CranUrlFromName(string name) => "https://cran.rstudio.com";

        public Task<string> FetchFileAsync(string remoteFileName, ulong remoteBlobId, string localPath, CancellationToken cancellationToken)
            => Task.FromResult(string.Empty);

        public Task<LocatorResult> Locator(Guid deviceId, CancellationToken ct)
            => Task.FromResult(new LocatorResult());

        public Task Plot(PlotMessage plot, CancellationToken ct) {
            PlotResult = plot.Data;
            return Task.CompletedTask;
        }

        public Task<PlotDeviceProperties> PlotDeviceCreate(Guid deviceId, CancellationToken ct)
            => Task.FromResult(PlotDeviceProperties);

        public Task PlotDeviceDestroy(Guid deviceId, CancellationToken ct) => Task.CompletedTask;
        public Task<string> ReadUserInput(string prompt, int maximumLength, CancellationToken ct) => Task.FromResult(string.Empty);
        public Task ShowErrorMessage(string message, CancellationToken cancellationToken = default) => Task.CompletedTask;
        public Task ShowHelpAsync(string url, CancellationToken cancellationToken = default) => Task.CompletedTask;

        public Task<MessageButtons> ShowMessageAsync(string message, MessageButtons buttons, CancellationToken cancellationToken = default)
            => Task.FromResult(MessageButtons.OK);

        public Task ViewFile(string fileName, string tabName, bool deleteFile, CancellationToken cancellationToken = default) => Task.CompletedTask;
        public Task<string> EditFileAsync(string expression, string fileName, CancellationToken cancellationToken = default) => Task.FromResult(string.Empty);
        public Task ViewLibraryAsync(CancellationToken cancellationToken = default) => Task.CompletedTask;
        public Task ViewObjectAsync(string expression, string title, CancellationToken cancellationToken = default) => Task.CompletedTask;

        public string GetLocalizedString(string id) => null;
        public Task BeforePackagesInstalledAsync(CancellationToken ct) => Task.CompletedTask;
        public Task AfterPackagesInstalledAsync(CancellationToken ct) => Task.CompletedTask;
    }
}
