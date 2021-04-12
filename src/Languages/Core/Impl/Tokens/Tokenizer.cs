// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.Languages.Core.Text;

namespace Microsoft.Languages.Core.Tokens {
    public static class Tokenizer {
        /// <summary>
        /// Handle generic comment. Comment goes to the end of the line.
        /// </summary>
        public static void HandleEolComment(CharacterStream cs, Action<int, int> addToken) {
            var start = cs.Position;

            while (!cs.IsEndOfStream() && !cs.IsAtNewLine()) {
                cs.MoveToNextChar();
            }

            int length = cs.Position - start;
            if (length > 0) {
                addToken(start, length);
            }
        }

        /// <summary>
        /// Handles string sequence with escapes
        /// </summary>
        public static void HandleString(char quote, CharacterStream cs) => HandleString(1, cs, x => x.CurrentChar == quote);

        /// <summary>
        /// Handles raw string to double quote followed by )
        /// </summary>
        /// <remarks>
        /// Raw character constants are also available using a syntax similar 
        /// to the one used in C++: r"(...)" with ... any character sequence, 
        /// except that it must not contain the closing sequence )". The delimiter 
        /// pairs [] and {} can also be used 
        /// </remarks>
        public static void HandleRawString(char quote, char openBrace, CharacterStream cs) {
            var closeBrace = GetMatchingBrace(openBrace);
            HandleString(2, cs, x => x.CurrentChar == closeBrace && x.NextChar == quote);
        }

        private static void HandleString(int separatorLength, CharacterStream cs, Func<CharacterStream, bool> terminatorCheck) {
            cs.Advance(separatorLength);

            if (!cs.IsEndOfStream()) {
                while (true) {
                    if (terminatorCheck(cs)) {
                        cs.Advance(separatorLength);
                        break;
                    }

                    if (cs.CurrentChar == '\\') {
                        cs.MoveToNextChar();
                    }

                    if (!cs.MoveToNextChar()) {
                        break;
                    }
                }
            }
        }

        public static char GetMatchingBrace(char brace) {
            switch (brace) {
                case '{':
                    return '}';

                case '[':
                    return ']';

                case '(':
                    return ')';
            }

            return char.MinValue;
        }

        public static void SkipIdentifier(CharacterStream cs, Func<CharacterStream, bool> isIdentifierLeadCharacter, Func<CharacterStream, bool> isIdentifierCharacter) {
            if (!isIdentifierLeadCharacter(cs)) {
                return;
            }

            if (cs.IsEndOfStream()) {
                return;
            }

            while (!cs.IsWhiteSpace()) {
                if (!isIdentifierCharacter(cs)) {
                    break;
                }

                if (!cs.MoveToNextChar()) {
                    break;
                }
            }
        }
    }
}
