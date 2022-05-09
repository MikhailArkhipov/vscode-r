// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Microsoft.Common.Core.Diagnostics;

namespace Microsoft.R.Host.Client.Host {
    [DebuggerDisplay("{Uri}, InterpreterId={InterpreterId}")]
    public struct BrokerConnectionInfo {
        public string Name { get; }
        public Uri Uri { get; }
        public string RCommandLineArguments { get; }
        public string InterpreterPath { get; }
        public string InterpreterArchitecture { get; }
        public Version RVersion { get; }

        public static BrokerConnectionInfo Create(string name, string interpreterPath, string interpreterArchitecture, Version rVersion, string rCommandLineArguments) {
            rCommandLineArguments = rCommandLineArguments ?? string.Empty;

            if (!Uri.TryCreate(interpreterPath, UriKind.Absolute, out Uri uri)) {
                return new BrokerConnectionInfo();
            }

            return new BrokerConnectionInfo(name, uri, interpreterPath, interpreterArchitecture, rVersion, rCommandLineArguments);
        }

        private BrokerConnectionInfo(string name, Uri uri, string interpreterPath, string interpreterArchitecture, Version rVersion, string rCommandLineArguments) {
            Check.ArgumentStringNullOrEmpty(nameof(interpreterPath), interpreterPath);
            Check.ArgumentStringNullOrEmpty(nameof(interpreterArchitecture), interpreterArchitecture);

            Name = name;
            Uri = uri;
            InterpreterPath = interpreterPath;
            InterpreterArchitecture = interpreterArchitecture;
            RCommandLineArguments = rCommandLineArguments?.Trim() ?? string.Empty;
            RVersion = rVersion;
        }
    }
}