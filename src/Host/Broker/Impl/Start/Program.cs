// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// #define WAIT_FOR_DEBUGGER

using System;
using System.Runtime.InteropServices;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.R.Host.Broker.Windows;

namespace Microsoft.R.Host.Broker.Start {
    public class Program {
        public static IWebHost WebHost { get; protected set; }
        public static void Main(string[] args) {
#if WAIT_FOR_DEBUGGER
            while (!System.Diagnostics.Debugger.IsAttached) {
                System.Threading.Thread.Sleep(1000);
            }
#endif
            var cm = new Configurator(args);

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows)) {
                WebHost = cm.ConfigureWebHost().UseStartup<WindowsStartup>().Build();
            } else {
                WebHost = cm.ConfigureWebHost().UseStartup<UnixStartup>().Build();
            }

            try {
                WebHost.Run();
            } catch (Exception ex) {
                ex.HandleWebHostStartExceptions(WebHost.Services.GetService<IServiceProvider>(), true);
            }
        }
    }
}
