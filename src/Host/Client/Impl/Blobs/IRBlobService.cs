// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.R.Host.Client {
    public interface IRBlobService {
        Task<ulong> CreateBlobAsync(CancellationToken cancellationToken = default);
        Task DestroyBlobsAsync(IEnumerable<ulong> blobIds, CancellationToken cancellationToken = default);
        Task<byte[]> BlobReadAllAsync(ulong blobId, CancellationToken cancellationToken = default);
        Task<byte[]> BlobReadAsync(ulong blobId, long position, long count, CancellationToken cancellationToken = default);
        Task <long> BlobWriteAsync(ulong blobId, byte[] data, long position, CancellationToken cancellationToken = default);
        Task<long> GetBlobSizeAsync(ulong blobId, CancellationToken cancellationToken = default);
        Task<long> SetBlobSizeAsync(ulong blobId, long size, CancellationToken cancellationToken = default);
    }
}
