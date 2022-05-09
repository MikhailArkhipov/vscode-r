// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.R.Core.AST.DataTypes;
using Microsoft.R.Core.Parser;
using Microsoft.R.Core.Tokens;

namespace Microsoft.R.Core.AST.Expressions {
    /// <summary>
    /// Represents named argument placeholder as in R 4.2 pipe expression
    /// such as underscore in `mtcars |> lm(mpg ~ disp, data = _)`
    /// </summary>
    [DebuggerDisplay("NamedArgumentPlaceholderExpression [{Start}...{End})")]
    public sealed class NamedArgumentPlaceholderExpression : RValueNode, IExpression {
        #region IExpression
        public TokenNode OpenBrace { get; private set; }
        public IRValueNode Content { get; private set; }
        public TokenNode CloseBrace { get; private set; }
        #endregion

        public NamedArgumentPlaceholderExpression() {
            Value = RNull.Null;
        }

        public override bool Parse(ParseContext context, IAstNode parent) {
            if (context.Tokens.CurrentToken.TokenType == RTokenType.Identifier) {
                context.AddError(new ParseError(ParseErrorType.UnexpectedToken, ErrorLocation.Token, context.Tokens.CurrentToken));
            }

            return false;
        }
    }
}
