// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.Common.Core;
using StreamJsonRpc;

namespace Microsoft.R.LanguageServer.Services {
    internal sealed class Client: IClient {
        private readonly JsonRpc _rpc;

        public Client(JsonRpc rpc) {
            _rpc = rpc;
        }

        public void PublishDiagnostics(Uri uri, IEnumerable<Diagnostic> diags) {
            var parameters = new PublishDiagnosticsParams {
                uri = uri,
                diagnostics = diags.ToArray()
            };
            _rpc.NotifyWithParameterObjectAsync("textDocument/publishDiagnostics", parameters).DoNotWait();
        }
    }
}
