// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Common.Core.Telemetry;

namespace Microsoft.R.LanguageServer.Services {
    class TelemetryService : ITelemetryService {
        public bool IsEnabled => false;
        public void ReportEvent(TelemetryArea area, string eventName, object parameters = null) { }
    }
}
