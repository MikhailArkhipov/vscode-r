// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.Common.Core.Services;
using Microsoft.R.Components.PackageManager;
using Microsoft.R.Host.Client;

namespace Microsoft.R.Components.InteractiveWorkflow {
    public interface IRInteractiveWorkflow : IDisposable {
        IServiceContainer Services { get; }
        IRPackageManager Packages { get; }
        IRSessionProvider RSessions { get; }
        IRSession RSession { get; }
        IRInteractiveWorkflowOperations Operations { get; }
    }
}