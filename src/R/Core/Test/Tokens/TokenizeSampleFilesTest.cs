// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics.CodeAnalysis;
using Microsoft.Languages.Core.Test.Utility;
using Microsoft.R.Core.Tokens;
using Microsoft.UnitTests.Core.XUnit;
using Xunit;

namespace Microsoft.R.Core.Test.Tokens {
    [ExcludeFromCodeCoverage]
    [Category.R.Tokenizer]
    public class TokenizeRSampleFilesTest {
        private readonly CoreTestFilesFixture _files;

        public TokenizeRSampleFilesTest(CoreTestFilesFixture files) {
            _files = files;
        }

        [CompositeTest]
        [InlineData(@"Tokenization\lsfit.r")]
        public void TokenizeLeastSquares(string fileName)
            => TokenizeFiles.TokenizeFile<RToken, RTokenType, RTokenizer>(_files, fileName, "R");
    }
}
