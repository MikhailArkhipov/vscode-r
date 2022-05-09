// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.


using System;
using Microsoft.Languages.Core.Text;

namespace Microsoft.Languages.Core.Tokens {
    /// <summary>
    /// Generic tokenizer
    /// </summary>
    public interface ITokenizer<T> where T : ITextRange {
        /// <summary>
        /// Tokenize text in a string
        /// </summary>
        /// <param name="text">Test to tokenize</param>
        /// <param name="languageVersion">Language version</param>
        /// <returns>Collection of tokens</returns>
        IReadOnlyTextRangeCollection<T> Tokenize(string text, Version languageVersion);

        /// <summary>
        /// Tokenize text from a given provider
        /// </summary>
        /// <param name="textProvider">Text provider</param>
        /// <param name="start">Start position</param>
        /// <param name="length">Length of fragent to tokenize</param>
        /// <param name="excludePartialTokens">True if tokenizeer should exclude partial token that may intersect end of the specified span</param>
        /// <param name="languageVersion">Language version</param>
        /// <returns>Collection of tokens</returns>
        IReadOnlyTextRangeCollection<T> Tokenize(ITextProvider textProvider, int start, int length, bool excludePartialTokens, Version languageVersion);

        /// <summary>
        /// Tokenize text from a given provider
        /// </summary>
        /// <param name="textProvider">Text provider</param>
        /// <param name="start">Start position</param>
        /// <param name="length">Length of fragent to tokenize</param>
        /// <param name="languageVersion">Language version</param>
        /// <returns>Collection of tokens</returns>
        IReadOnlyTextRangeCollection<T> Tokenize(ITextProvider textProvider, int start, int length, Version languageVersion);
    }
}
