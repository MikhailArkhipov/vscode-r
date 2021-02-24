// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Languages.Core.Text;
using Microsoft.R.Core.AST;
using Microsoft.R.Core.Parser;

namespace Microsoft.R.Editor.Validation.Errors {
    public class ValidationWarning : ValidationErrorBase {
        /// <summary>
        /// Constructs validation warning for an element at a specified location.
        /// </summary>
        public ValidationWarning(ITextRange range, string message, ErrorLocation location, int snapshotVersion) :
             base(range, message, location, ErrorSeverity.Warning, snapshotVersion) {
        }
        public ValidationWarning(IAstNode node, string message, ErrorLocation location) :
             base(node, message, location, ErrorSeverity.Warning, node.Root.TextProvider.Version) {
        }
    }
}
