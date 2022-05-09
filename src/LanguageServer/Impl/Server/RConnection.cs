// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.IO;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.Services;
using Microsoft.R.Components.InteractiveWorkflow;
using Microsoft.R.Editor.Functions;
using Microsoft.R.Host.Client;
using Microsoft.R.Host.Client.Host;
using Microsoft.R.LanguageServer.InteractiveWorkflow;
using Microsoft.R.LanguageServer.Settings;
using Microsoft.R.Platform.Interpreters;
using Microsoft.R.Platform.Windows.Interpreters;

namespace Microsoft.R.LanguageServer.Server {
    /// <summary>
    /// Manages connection to RTVS
    /// </summary>
    internal sealed class RConnection : IDisposable {
        private readonly IServiceContainer _services;
        private readonly CancellationToken _cancellationToken;
        private IRInteractiveWorkflow _workflow;
        private IPackageIndex _packageIndex;
        private IHostUIService _ui;

        public RConnection(IServiceContainer services, CancellationToken cancellationToken) {
            _services = services;
            _cancellationToken = cancellationToken;

            var settings = _services.GetService<ISettingsManager>();
            settings.SettingsChanged += OnSettingsChanged;
        }

        private void OnSettingsChanged(object s, EventArgs e) {
            var settings = _services.GetService<ISettingsManager>();
            settings.SettingsChanged -= OnSettingsChanged;
            ConnectAsync(_cancellationToken).DoNotWait();
        }

        private async Task ConnectAsync(CancellationToken ct) {
            var provider = _services.GetService<IRInteractiveWorkflowProvider>();
            _workflow = provider.GetOrCreate();
            _ui = _services.GetService<IHostUIService>();
            _ui.SetLogLevel(MessageType.Info);

            var e = GetREngine();
            if (e == null) {
                return;
            }

            var log = _services.Log();
            var info = BrokerConnectionInfo.Create("(local)", e.InstallPath, e.Architecture, e.Version, string.Empty);
            var start = DateTime.Now;

            _ui.LogMessageAsync($"Starting R Process with {e.InstallPath}...", MessageType.Info).DoNotWait();

            try {
                if (await _workflow.RSessions.TrySwitchBrokerAsync("(local)", info, ct)) {
                    try {
                        await _workflow.RSession.StartHostAsync(new RHostStartupInfo(), new RSessionCallback(), _services.UI(), Debugger.IsAttached ? 1000000 : 20000, ct);
                    } catch (Exception ex) {
                        _ui.ShowMessageAsync($"Unable to start Microsoft.R.Host process. Exception: {ex.Message}", MessageType.Error).DoNotWait();
                        return;
                    }

                    // Start package building
                    _ui.LogMessageAsync($"complete in {FormatElapsed(DateTime.Now - start)}", MessageType.Info).DoNotWait();
                    start = DateTime.Now;
                    _ui.LogMessageAsync("Building IntelliSense index...", MessageType.Info).DoNotWait();

                    _packageIndex = _services.GetService<IPackageIndex>();
                    _packageIndex.BuildIndexAsync(ct).ContinueWith(t => {
                        _ui.LogMessageAsync($"complete in {FormatElapsed(DateTime.Now - start)}", MessageType.Info).DoNotWait();
                    }, ct, TaskContinuationOptions.None, TaskScheduler.Default).DoNotWait();
                } else {
                    _ui.ShowMessageAsync($"Unable to connect to broker.", MessageType.Error).DoNotWait();
                }
            } catch (Exception ex) {
                _ui.ShowMessageAsync($"Unable to connect to broker. Exception: {ex.Message}", MessageType.Error).DoNotWait();
            }
        }

        public void Dispose() => _workflow?.Dispose();

        private IRInterpreterInfo GetREngine() {
            var rs = _services.GetService<IREngineSettings>();
            if (!string.IsNullOrEmpty(rs.InterpreterPath)) {
                _ui.LogMessageAsync($"Using interpreter at '{rs.InterpreterPath}'", MessageType.Info).DoNotWait();
                return new RInterpreterInfo("R", rs.InterpreterPath, _services.GetService<IFileSystem>());
            }

            var ris = _services.GetService<IRInstallationService>();
            var engines = ris
                .GetCompatibleEngines()
                .OrderByDescending(x => x.Version)
                .ToList();

            if (engines.Count == 0) {
                const string message = "Unable to find R interpreter. Please install R from https://cran.r-project.org";
                _ui.ShowMessageAsync(message, MessageType.Error).DoNotWait();
                return null;
            }

            _ui.LogMessageAsync("Available R interpreters:", MessageType.Info).DoNotWait();
            for (var i = 0; i < engines.Count; i++) {
                _ui.LogMessageAsync($"\t[{i}] {engines[i].Name}", MessageType.Info).DoNotWait();
            }
            _ui.LogMessageAsync("You can specify the desired interpreter index via `r.interpreter` setting", MessageType.Info).DoNotWait();
            _ui.LogMessageAsync("or provide path to R using `r.interpreterPath` setting.", MessageType.Info).DoNotWait();

            if (rs.InterpreterIndex >= engines.Count) {
                _ui.ShowMessageAsync($"Selected interpreter [{rs.InterpreterIndex}] does not exist. Using latest instead.", MessageType.Warning).DoNotWait();
                rs.InterpreterIndex = -1;
            }

            if (rs.InterpreterIndex < 0) {
                // Default
                var latest = ris.GetLatest();
                if (latest != null) {
                    rs.InterpreterIndex = engines.FindIndex(e => e.Architecture == latest.Architecture);
                }
                if (rs.InterpreterIndex < 0) {
                    _ui.ShowMessageAsync($"No interpreter is available for architecture {ris.Architecture} does not exist. Trying first available R.", MessageType.Warning).DoNotWait();
                    rs.InterpreterIndex = 0;
                }
            }

            if (rs.InterpreterIndex > engines.Count) {
                _ui.ShowMessageAsync($"Selected interpreter [{rs.InterpreterIndex}] does not exist. Using lastest instead", MessageType.Warning).DoNotWait();
            } else {
                _ui.LogMessageAsync($"Selected interpreter: [{rs.InterpreterIndex}] {engines[rs.InterpreterIndex].Name}.\n", MessageType.Info).DoNotWait();
            }

            var e = engines[rs.InterpreterIndex];
            _ui.LogMessageAsync($"CPU architecture: {ris.Architecture}", MessageType.Info).DoNotWait();
            _ui.LogMessageAsync($"Selected R architecture: {e.Architecture}", MessageType.Info).DoNotWait();

            return engines[rs.InterpreterIndex];
        }

        private static string FormatElapsed(TimeSpan ts) => ts.ToString("mm':'ss':'fff");
    }
}
