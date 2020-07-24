// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.R.Host.Client.Host {
    public interface IBrokerClient : IDisposable {
        BrokerConnectionInfo ConnectionInfo { get; }
        string Name { get; }
        Task<RHost> ConnectAsync(HostConnectionInfo connectionInfo, CancellationToken cancellationToken = default);
        Task TerminateSessionAsync(string name, CancellationToken cancellationToken = default);
        Task<string> HandleUrlAsync(string url, CancellationToken cancellationToken = default);
        Task<T> GetHostInformationAsync<T>(CancellationToken cancellationToken = default);
    }
}