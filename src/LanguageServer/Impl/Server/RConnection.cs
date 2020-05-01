// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.Services;
using Microsoft.R.Components.InteractiveWorkflow;
using Microsoft.R.Editor.Functions;
using Microsoft.R.Host.Client;
using Microsoft.R.Host.Client.Host;
using Microsoft.R.LanguageServer.InteractiveWorkflow;
using Microsoft.R.LanguageServer.Settings;
using Microsoft.R.Platform.Interpreters;

namespace Microsoft.R.LanguageServer.Server {
    /// <summary>
    /// Manages connection to RTVS
    /// </summary>
    internal sealed class RConnection : IDisposable {
        private readonly IServiceContainer _services;
        private readonly CancellationToken _cancellationToken;
        private IRInteractiveWorkflow _workflow;
        private IPackageIndex _packageIndex;
        private IUIService _ui;

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
            _ui = _services.GetService<IUIService>();
            _ui.SetLogLevel(MessageType.Info);

            var e = GetREngine();
            if(e == null) {
                return;
            }

            var log = _services.Log();
            var info = BrokerConnectionInfo.Create(_services.Security(), "VSCR", e.InstallPath, string.Empty, false);

            var start = DateTime.Now;
            var message = $"Starting R Process with {e.InstallPath}...";
            _ui.LogMessageAsync(message, MessageType.Info).DoNotWait();

            log.Write(LogVerbosity.Normal, MessageCategory.General, $"Switching local broker to {e.InstallPath}");
            if (await _workflow.RSessions.TrySwitchBrokerAsync("VSCR", info, ct)) {
                try {
                    await _workflow.RSession.StartHostAsync(new RHostStartupInfo(), new RSessionCallback(), Debugger.IsAttached ? 100000 : 20000, ct);
                } catch (Exception ex) {
                    _ui.ShowMessageAsync($"Unable to start R process. Exception: {ex.Message}", MessageType.Error).DoNotWait();
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
                _ui.ShowMessageAsync("Unable to start R process", MessageType.Error).DoNotWait();
            }
        }

        public void Dispose() => _workflow?.Dispose();

        private IRInterpreterInfo GetREngine() {
            var ris = _services.GetService<IRInstallationService>();
            var engines = ris
                .GetCompatibleEngines(new SupportedRVersionRange(3, 2, 4, 9))
                .OrderByDescending(x => x.Version)
                .ToList();

            if (engines.Count == 0) {
                const string message = "Unable to find R intepreter. Please install R from https://cran.r-project.org";
                _ui.ShowMessageAsync(message, MessageType.Error).DoNotWait();
                return null;
            }

            _ui.LogMessageAsync("Available R interpreters:", MessageType.Info).DoNotWait();
            for (var i = 0; i < engines.Count; i++) {
                _ui.LogMessageAsync($"\t[{i}] {engines[i].Name}", MessageType.Info).DoNotWait();
            }
            _ui.LogMessageAsync("You can specify the desired interpreter index in the R settings", MessageType.Info).DoNotWait();

            var rs = _services.GetService<IREngineSettings>();
            if (rs.InterpreterIndex < 0 || rs.InterpreterIndex > engines.Count) {
                _ui.ShowMessageAsync($"Selected interpreter [{rs.InterpreterIndex}] does not exist. Using [0] instead", MessageType.Warning).DoNotWait();
                rs.InterpreterIndex = 0;
            } else {
                _ui.LogMessageAsync($"Selected interpreter: [{rs.InterpreterIndex}] {engines[rs.InterpreterIndex].Name}.\n", MessageType.Info).DoNotWait();
            }

            return engines[rs.InterpreterIndex];
        }

        private static string FormatElapsed(TimeSpan ts) => ts.ToString("mm':'ss':'fff");
    }
}
