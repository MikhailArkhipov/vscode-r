// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics.CodeAnalysis;
using Microsoft.R.Core.Test.Utility;
using Microsoft.UnitTests.Core.XUnit;
using Xunit;

namespace Microsoft.R.Core.Test.Parser {
    [ExcludeFromCodeCoverage]
    [Category.R.Parser]
    public class ParsePipeOperatorTest {
        [CompositeTest]
        [InlineData("4.1")]
        [InlineData("4.2")]
        [InlineData("5.0")]
        [InlineData("5.1")]
        public void PipeSupported(string ver) {
            const string expected =
@"GlobalScope  [Global]
    ExpressionStatement  [a |> b]
        Expression  [a |> b]
            TokenOperator  [|> [2...4)]
                Variable  [a]
                TokenNode  [|> [2...4)]
                Variable  [b]
";
            ParserTest.VerifyParse(expected, "a |> b", new Version(ver));
        }

        //[CompositeTest]
        [InlineData("4.0")]
        [InlineData("3.9")]
        [InlineData("3.2")]
        [InlineData("1.0")]
        public void PipeNotSupportedBelow41(string ver) {
            const string expected =
@"GlobalScope  [Global]

RightOperandExpected Token [3...4)
";
            ParserTest.VerifyParse(expected, "a |> b", new Version(ver));
        }
    }
}
