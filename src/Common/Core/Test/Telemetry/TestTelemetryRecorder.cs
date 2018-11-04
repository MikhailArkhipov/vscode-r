// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Text;
using Microsoft.Common.Core.Telemetry;

namespace Microsoft.Common.Core.Test.Telemetry {

    [ExcludeFromCodeCoverage]
    public sealed class TestTelemetryRecorder : ITelemetryRecorder, ITelemetryTestSupport {
        private readonly StringBuilder _stringBuilder = new StringBuilder();

        #region ITelemetryRecorder
        public bool IsEnabled => true;

        public bool CanCollectPrivateInformation => true;

        public void RecordEvent(string eventName, object parameters = null) {
            _stringBuilder.AppendLine(eventName);
            if (parameters != null) {
                if (parameters is string) {
                    WriteProperty("Value", parameters as string);
                } else {
                    WriteDictionary(DictionaryExtensions.FromAnonymousObject(parameters));
                }
            }
        }
        #endregion

        #region ITelemetryTestSupport
        public void Reset() => _stringBuilder.Clear();

        public string SessionLog => _stringBuilder.ToString();
        #endregion

        public void Dispose() { }

        private void WriteDictionary(IDictionary<string, object> dict) {
            foreach (var kvp in dict) {
                WriteProperty(kvp.Key, kvp.Value);
            }
        }

        private void WriteProperty(string name, object value) {
            _stringBuilder.Append('\t');
            _stringBuilder.Append(name);
            _stringBuilder.Append(" : ");
            _stringBuilder.AppendLine(value.ToString());
        }
    }
}
