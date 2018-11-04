// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.IO;
using System.Threading;
using Microsoft.Common.Core.Imaging;
using Microsoft.Common.Core.Logging;
using Microsoft.Common.Core.Services;
using Microsoft.Common.Core.Shell;
using Microsoft.Common.Core.Tasks;
using Microsoft.R.Components.InteractiveWorkflow;
using Microsoft.R.Editor;
using Microsoft.R.LanguageServer.Commands;
using Microsoft.R.LanguageServer.Documents;
using Microsoft.R.LanguageServer.InteractiveWorkflow;
using Microsoft.R.LanguageServer.Server;
using Microsoft.R.LanguageServer.Services.Editor;
using Microsoft.R.LanguageServer.Text;
using Microsoft.R.LanguageServer.Threading;
using Microsoft.R.Platform;

namespace Microsoft.R.LanguageServer.Services {
    internal sealed class ServiceContainer : ServiceManager {
        public ServiceContainer() {
            var mt = new MainThread();
            SynchronizationContext.SetSynchronizationContext(mt.SynchronizationContext);

            AddService<IActionLog>(s => new Logger("VSCode-R", Path.GetTempPath(), s))
                .AddService(mt)
                .AddService(new ContentTypeServiceLocator())
                .AddService<ISettingsStorage, SettingsStorage>()
                .AddService<ITaskService, TaskService>()
                .AddService<IImageService, ImageService>()
                .AddService(new Application())
                .AddService<IRInteractiveWorkflowProvider, RInteractiveWorkflowProvider>()
                .AddService(new IdleTimeService(this))
                .AddService(new DocumentCollection(this))
                .AddService(new ViewSignatureBroker())
                .AddService(new EditorSupport())
                .AddService(new REvalSession(this))
                .AddService(new SettingsManager(this))
                .AddService(new Controller(this))
                .AddEditorServices();

            PlatformServiceProvider.AddPlatformSpecificServices(this);
        }
    }
}
