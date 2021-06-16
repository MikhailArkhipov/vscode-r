// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Common.Core.IO;
using Microsoft.Common.Core.OS;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Microsoft.R.Host.Broker.RHost;
using Microsoft.R.Host.Broker.Services;
using Microsoft.R.Host.Broker.Start;
using Microsoft.R.Platform.Windows.IO;
using Microsoft.R.Platform.Windows.OS;
using Microsoft.R.Platform.Windows.Registry;

namespace Microsoft.R.Host.Broker.Windows {
    public sealed class WindowsStartup : Startup {
        public WindowsStartup(ILoggerFactory loggerFactory, IConfigurationRoot configuration) : base(loggerFactory, configuration) {}

        public override void ConfigureServices(IServiceCollection services) {
            base.ConfigureServices(services);

            services
                .AddSingleton<IFileSystem>(new WindowsFileSystem())
                .AddSingleton<IProcessServices>(new WindowsProcessServices())
                .AddSingleton<IRegistry>(new RegistryImpl())
                .AddSingleton<IRHostProcessService, RHostProcessService>();
        }
    }
}
