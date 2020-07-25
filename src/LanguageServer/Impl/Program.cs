// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// #define WAIT_FOR_DEBUGGER

using System;
using System.IO;
using System.Threading;
using Microsoft.R.LanguageServer.Services;
using Microsoft.R.LanguageServer.Threading;
using Newtonsoft.Json;
using StreamJsonRpc;

namespace Microsoft.R.LanguageServer.Server {
    internal static class Program {
        public static void Main(string[] args) {
#if WAIT_FOR_DEBUGGER
            while (!System.Diagnostics.Debugger.IsAttached) {
                System.Threading.Thread.Sleep(1000);
            }
#endif
            var messageFormatter = new JsonMessageFormatter();
            // StreamJsonRpc v1.4 serializer defaults
            messageFormatter.JsonSerializer.NullValueHandling = NullValueHandling.Ignore;
            messageFormatter.JsonSerializer.ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor;
            messageFormatter.JsonSerializer.Converters.Add(new UriConverter());

            using (CoreShell.Create()) {
                var services = CoreShell.Current.ServiceManager;

                using (var cin = Console.OpenStandardInput())
                using (var bcin = new BufferedStream(cin))
                using (var cout = Console.OpenStandardOutput())
                using (var server = new LanguageServer(services))
                using (var rpc = new JsonRpc(cout, cin, server)) {
                    rpc.SynchronizationContext = new SingleThreadSynchronizationContext();

                    services
                        .AddService(new UIService(rpc))
                        .AddService(new Client(rpc))
                        .AddService(messageFormatter.JsonSerializer);

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

        sealed class UriConverter : JsonConverter {
            public override bool CanConvert(Type objectType) => objectType == typeof(Uri);

            public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer) {
                if (reader.TokenType == JsonToken.String) {
                    var str = (string)reader.Value;
                    return new Uri(str.Replace("%3A", ":"));
                }

                if (reader.TokenType == JsonToken.Null) {
                    return null;
                }

                throw new InvalidOperationException($"UriConverter: unsupported token type {reader.TokenType}");
            }

            public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer) {
                if (null == value) {
                    writer.WriteNull();
                    return;
                }

                if (value is Uri) {
                    var uri = (Uri)value;
                    var scheme = uri.Scheme;
                    var str = uri.ToString();
                    str = uri.Scheme + "://" + str.Substring(scheme.Length + 3).Replace(":", "%3A").Replace('\\', '/');
                    writer.WriteValue(str);
                    return;
                }

                throw new InvalidOperationException($"UriConverter: unsupported value type {value.GetType()}");
            }
        }
    }
}