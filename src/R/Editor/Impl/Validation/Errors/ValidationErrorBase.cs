// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Languages.Core.Text;
using Microsoft.R.Core.AST;
using Microsoft.R.Core.Parser;

namespace Microsoft.R.Editor.Validation.Errors {
    public class ValidationErrorBase : TextRange, IValidationError {
        /// <summary>
        /// Error or warning message.
        /// </summary>
        public string Message { get; }

        /// <summary>
        /// Error location
        /// </summary>
        public ErrorLocation Location { get; }

        /// <summary>
        /// Error severity
        /// </summary>
        public ErrorSeverity Severity { get; }

        /// <summary>
        /// Version of the text buffer snapshot the error applies to.
        /// </summary>
        public int SnapshotVersion { get; }

        public ValidationErrorBase(ITextRange range, string message, ErrorLocation location, ErrorSeverity severity, int snapshotVersion) :
            base(range) {
            Message = message;
            Severity = severity;
            Location = location;
            SnapshotVersion = snapshotVersion;
        }
        
        public ValidationErrorBase(IAstNode node, string message, ErrorLocation location, ErrorSeverity severity) 
            : this(node, message, location, severity, node.Root.TextProvider.Version) { }
    }
}
