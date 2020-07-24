// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Microsoft.Common.Core;

namespace Microsoft.R.Host.Client.Host {
    [DebuggerDisplay("{Uri}, InterpreterId={InterpreterId}")]
    public struct BrokerConnectionInfo {
        public string Name { get; }
        public Uri Uri { get; }
        public bool IsValid { get; }
        public string ParametersId { get; }
        public string RCommandLineArguments { get; }
        public string InterpreterId { get; }

        public static BrokerConnectionInfo Create(string name, string path, string rCommandLineArguments) {
            rCommandLineArguments = rCommandLineArguments ?? string.Empty;

            if (!Uri.TryCreate(path, UriKind.Absolute, out Uri uri)) {
                return new BrokerConnectionInfo();
            }

            return new BrokerConnectionInfo(name, uri, rCommandLineArguments, string.Empty);
        }

        private BrokerConnectionInfo(string name, Uri uri, string rCommandLineArguments, string interpreterId) {
            Name = name;
            IsValid = true;
            Uri = uri;
            RCommandLineArguments = rCommandLineArguments?.Trim() ?? string.Empty;
            InterpreterId = interpreterId;
            ParametersId = string.Empty;
        }

        public override bool Equals(object obj) => obj is BrokerConnectionInfo && Equals((BrokerConnectionInfo)obj);

        public bool Equals(BrokerConnectionInfo other) => other.ParametersId.EqualsOrdinal(ParametersId) && Equals(other.Uri, Uri);

        public override int GetHashCode() {
            unchecked {
                return ((ParametersId?.GetHashCode() ?? 0)*397) ^ (Uri != null ? Uri.GetHashCode() : 0);
            }
        }

        public static bool operator ==(BrokerConnectionInfo a, BrokerConnectionInfo b) => a.Equals(b);

        public static bool operator !=(BrokerConnectionInfo a, BrokerConnectionInfo b) => !a.Equals(b);
    }
}