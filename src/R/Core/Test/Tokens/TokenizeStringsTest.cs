// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics.CodeAnalysis;
using FluentAssertions;
using Microsoft.Languages.Core.Test.Tokens;
using Microsoft.R.Core.Tokens;
using Microsoft.UnitTests.Core.XUnit;
using Xunit;

namespace Microsoft.R.Core.Test.Tokens {
    [ExcludeFromCodeCoverage]
    public class TokenizeStringsTest : TokenizeTestBase<RToken, RTokenType> {
        [CompositeTest]
        [Category.R.Tokenizer]
        [InlineData("'\"x\"'")]
        [InlineData("\"'x'\"")]
        public void TokenizeString(string s) {
            var tokens = Tokenize(s, new RTokenizer());

            tokens.Should().ContainSingle()
                .Which.Should().HaveType(RTokenType.String)
                .And.StartAt(0)
                .And.HaveLength(s.Length);
        }

        [CompositeTest]
        [Category.R.Tokenizer]
        [InlineData("r\"(\"x\")\"")]
        [InlineData("R\"(\"x\")\"")]
        [InlineData("r\"[\"x\"]\"")]
        [InlineData("r\"{\"x\"}\"")]
        [InlineData("r\"('x\"')\"")]
        [InlineData("R\"('x\"')\"")]
        [InlineData("r\"['x'\"']\"")]
        [InlineData("r\"{'x\"'}\"")]
        public void TokenizeRawString(string s) {
            var tokens = Tokenize(s, new RTokenizer());

            tokens.Should().ContainSingle()
                .Which.Should().HaveType(RTokenType.String)
                .And.StartAt(0)
                .And.HaveLength(s.Length);
        }
    }
}
