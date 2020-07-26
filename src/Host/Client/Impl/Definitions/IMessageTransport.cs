// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Threading;
using System.Threading.Tasks;
using Microsoft.R.Host.Protocol;

namespace Microsoft.R.Host.Client {
    public interface IMessageTransport {
        string CloseStatusDescription { get; }

        Task CloseAsync(CancellationToken cancellationToken = default);
        Task SendAsync(Message message, CancellationToken cancellationToken = default);
        Task<Message> ReceiveAsync(CancellationToken cancellationToken = default);
    }
}
