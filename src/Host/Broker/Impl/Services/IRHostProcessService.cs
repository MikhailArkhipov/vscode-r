// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Common.Core.OS;

namespace Microsoft.R.Host.Broker.Services {
    public interface IRHostProcessService {
        IProcess StartHost(string interpreterPath, string architecture, string commandLine);
    }
}