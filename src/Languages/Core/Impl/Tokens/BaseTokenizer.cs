// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using Microsoft.Languages.Core.Text;

namespace Microsoft.Languages.Core.Tokens {
    public abstract class BaseTokenizer<T> : ITokenizer<T> where T : ITextRange {
        protected TextRangeCollection<T> _tokens;
        protected CharacterStream _cs;
        protected Version _languageVersion;

        #region ITokenizer
        public IReadOnlyTextRangeCollection<T> Tokenize(string text, Version languageVersion) 
            => Tokenize(new TextStream(text), 0, text.Length, languageVersion);

        public IReadOnlyTextRangeCollection<T> Tokenize(ITextProvider textProvider, int start, int length, Version languageVersion) 
            => Tokenize(textProvider, start, length, false, languageVersion);

        public virtual IReadOnlyTextRangeCollection<T> Tokenize(ITextProvider textProvider, int start, int length, bool excludePartialTokens, Version languageVersion) {
            var end = start + length;
            _languageVersion = languageVersion;

            InitializeTokenizer(textProvider, start, length);

            while (!_cs.IsEndOfStream()) {
                // Keep on adding tokens
                AddNextToken();

                if (_cs.Position >= end) {
                    break;
                }
            }

            if (excludePartialTokens) {
                // Exclude tokens that are beyond the specified range
                int i;
                for (i = _tokens.Count - 1; i >= 0; i--) {
                    if (_tokens[i].End <= end) {
                        break;
                    }
                }

                i++;
                if (i < _tokens.Count) {
                    _tokens.RemoveRange(i, _tokens.Count - i);
                }
            }

            return new ReadOnlyTextRangeCollection<T>(_tokens);
        }
        #endregion

        internal virtual void InitializeTokenizer(ITextProvider textProvider, int start, int length) {
            Debug.Assert(start >= 0 && length >= 0 && start + length <= textProvider.Length);

            _cs = new CharacterStream(textProvider) {Position = start};
            _tokens = new TextRangeCollection<T>();
        }

        public abstract void AddNextToken();

        public void SkipWhitespace() {
            if (_cs.IsEndOfStream()) {
                return;
            }

            while (_cs.IsWhiteSpace()) {
                if (!_cs.MoveToNextChar()) {
                    break;
                }
            }
        }
    }
}
