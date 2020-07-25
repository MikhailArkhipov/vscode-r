// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using Microsoft.R.Host.Client.Host;

namespace Microsoft.R.Components.ConnectionManager {
    public class Connection : ConnectionInfo, IConnection {
        public static Connection Create(IConnectionInfo connectionInfo) {
            var brokerConnectionInfo = BrokerConnectionInfo.Create(connectionInfo.Name, connectionInfo.Path, connectionInfo.RCommandLineArguments);
            return new Connection(brokerConnectionInfo, connectionInfo);
        }

        public static Connection Create(string name, string path, string rCommandLineArguments, bool isUserCreated) {
            var brokerConnectionInfo = BrokerConnectionInfo.Create(name, path, rCommandLineArguments);
            return new Connection(brokerConnectionInfo, path, isUserCreated);
        }

        private Connection(BrokerConnectionInfo brokerConnectionInfo, IConnectionInfo connectionInfo) 
            : base(connectionInfo) {
            BrokerConnectionInfo = brokerConnectionInfo;
        }

        private Connection(BrokerConnectionInfo brokerConnectionInfo, string path, bool isUserCreated) 
            : base(brokerConnectionInfo.Name, path, brokerConnectionInfo.RCommandLineArguments, isUserCreated) {
            BrokerConnectionInfo = brokerConnectionInfo;
        }

        public BrokerConnectionInfo BrokerConnectionInfo { get; }

        public Uri Uri => BrokerConnectionInfo.Uri;
    }
}