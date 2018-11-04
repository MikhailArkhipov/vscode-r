// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Common.Core.Services;
using Microsoft.R.Platform.Interpreters;
using Microsoft.R.Platform.Windows.Interpreters;

namespace Microsoft.R.Platform.Windows.Services {
    public static class ServicesExtensions {
        public static IServiceManager AddWindowsRInterpretersServices(this IServiceManager serviceManager) => serviceManager
            .AddService<IRInstallationService, RInstallation>();
    }
}
