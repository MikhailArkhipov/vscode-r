// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// #define WAIT_FOR_DEBUGGER

using System;
using System.IO;
using System.Threading;
using Microsoft.R.LanguageServer.Services;
using StreamJsonRpc;

namespace Microsoft.R.LanguageServer.Server {
    internal static class Program {
        public static void Main(string[] args) {
#if WAIT_FOR_DEBUGGER
            while (!System.Diagnostics.Debugger.IsAttached) {
                System.Threading.Thread.Sleep(1000);
            }
#endif
            using (CoreShell.Create()) {
                var services = CoreShell.Current.ServiceManager;

                using (var cin = Console.OpenStandardInput())
                using (var bcin = new BufferedStream(cin))
                using (var cout = Console.OpenStandardOutput())
                using (var server = new LanguageServer(services))
                using (var rpc = new JsonRpc(cout, cin, server)) {

                    services.AddService(new UIService(rpc));
                    services.AddService(new Client(rpc));

                    var cts = new CancellationTokenSource();
                    using (new RConnection(services, cts.Token)) {
                        var token = server.Start();
                        rpc.StartListening();
                        // Wait for the "stop" request.
                        token.WaitHandle.WaitOne();
                        cts.Cancel();
                    }
                }
            }
        }
    }
}