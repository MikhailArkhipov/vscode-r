﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Languages.Core.Tokens;
using Microsoft.R.Core.AST.Arguments;
using Microsoft.R.Core.AST.Scopes;
using Microsoft.R.Core.Parser;
using Microsoft.R.Core.Tokens;

namespace Microsoft.R.Core.AST.Functions {
    /// <summary>
    /// Represents code that defines a function such as
    /// 'function(a, b) { }'
    /// </summary>
    [DebuggerDisplay("Function Definition [{Start}...{End})")]
    public class FunctionDefinition : RValueNode, IFunctionDefinition {
        #region IKeyword
        public TokenNode Keyword { get; private set; }

        public string Text { get; private set; }
        #endregion

        #region IFunctionDefinition
        /// <summary>
        /// Function definition scope. Can be typical
        /// { } scope or a simple scope as in
        /// x &lt;- function(a) return(a+1)
        /// </summary>
        public IScope Scope { get; private set; }
        #endregion

        #region IFunction
        /// <summary>
        /// Function arguments
        /// </summary>
        public TokenNode OpenBrace { get; private set; }

        /// <summary>
        /// Function arguments
        /// </summary>
        public ArgumentList Arguments { get; private set; }

        /// <summary>
        /// Closing brace. May be null if closing brace is missing.
        /// </summary>
        public TokenNode CloseBrace { get; private set; }

        /// <summary>
        /// Returns end of a function signature (list of arguments).
        /// In case closing brace is missing scope extends to a
        /// nearest recovery point which may be an identifier
        /// or a keyword (except inline 'if').
        /// </summary>
        public int SignatureEnd {
            get {
                if (CloseBrace != null) {
                    return CloseBrace.End;
                } else if (Arguments.Count > 0) {
                    return Arguments.End;
                }

                return OpenBrace.End;
            }
        }
        #endregion

        public override bool Parse(ParseContext context, IAstNode parent) {
            TokenStream<RToken> tokens = context.Tokens;

            // Since R 4.2: either "(function (x) {...})(1)" OR "(\(x) {...})(1)"
            if (tokens.CurrentToken.TokenType == RTokenType.Keyword) {
                Keyword = RParser.ParseKeyword(context, this);
                Text = context.TextProvider.GetText(Keyword);
            } else if(tokens.CurrentToken.TokenType == RTokenType.Backslash && tokens.NextToken.TokenType == RTokenType.OpenBrace) {
                Keyword = RParser.ParseToken(context, this);
                Text = context.TextProvider.GetText(Keyword);
            } else {
                Debug.Assert(false, "Unexpected start token when parsing function definition.");
            }

            if (tokens.CurrentToken.TokenType == RTokenType.OpenBrace) {
                OpenBrace = RParser.ParseToken(context, this);

                Arguments = new ArgumentList(RTokenType.CloseBrace);
                Arguments.Parse(context, this);

                if (tokens.CurrentToken.TokenType == RTokenType.CloseBrace) {
                    CloseBrace = RParser.ParseToken(context, this);
                    Scope = RParser.ParseScope(context, this, allowsSimpleScope: true, terminatingKeyword: null);
                    if (Scope != null) {
                        return base.Parse(context, parent);
                    } else {
                        context.AddError(new ParseError(ParseErrorType.FunctionBodyExpected, ErrorLocation.Token, tokens.PreviousToken));
                    }
                } else {
                    context.AddError(new ParseError(ParseErrorType.CloseBraceExpected, ErrorLocation.Token, tokens.CurrentToken));
                }
            } else {
                context.AddError(new ParseError(ParseErrorType.OpenBraceExpected, ErrorLocation.Token, tokens.CurrentToken));
            }

            return false;
        }
    }
}
