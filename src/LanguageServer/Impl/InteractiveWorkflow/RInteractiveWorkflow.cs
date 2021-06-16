// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Common.Core.Disposables;
using Microsoft.Common.Core.Services;
using Microsoft.R.Components.InteractiveWorkflow;
using Microsoft.R.Components.PackageManager;
using Microsoft.R.Host.Client;
using Microsoft.R.Host.Client.Session;

namespace Microsoft.R.LanguageServer.InteractiveWorkflow {
    internal sealed class RInteractiveWorkflow: IRInteractiveWorkflow {
        private readonly DisposableBag _disposableBag;

        public IServiceContainer Services { get; }
        public IRSessionProvider RSessions => Services.GetService<IRSessionProvider>();
        public IRSession RSession { get; }
        public IRPackageManager Packages { get; }
        public IRInteractiveWorkflowOperations Operations { get; }

        public RInteractiveWorkflow(IServiceContainer services) {
            Services = services.Extend()
                .AddService<IRInteractiveWorkflow>(this)
                .AddService<IRSessionProvider, RSessionProvider>();

            RSession = RSessions.GetOrCreate(SessionNames.InteractiveWindow);

            _disposableBag = DisposableBag.Create<RInteractiveWorkflow>()
                .Add(RSessions);
        }

        public void Dispose() => _disposableBag.TryDispose();
    }
}
