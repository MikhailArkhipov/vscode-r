﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;
using Microsoft.Common.Core.Services;
using Microsoft.Languages.Core.Text;
using Microsoft.Languages.Editor.Text;
using Microsoft.R.Core.Formatting;
using Microsoft.R.Core.Tokens;
using Microsoft.R.Editor;
using Microsoft.R.Editor.Document;
using Microsoft.R.Editor.Formatting;
using Microsoft.R.LanguageServer.Extensions;
using Microsoft.R.LanguageServer.Text;

namespace Microsoft.R.LanguageServer.Formatting {
    internal sealed class CodeFormatter {
        private readonly IServiceContainer _services;

        public CodeFormatter(IServiceContainer services) {
            _services = services;
        }

        public TextEdit[] Format(IEditorBufferSnapshot snapshot) {
            var settings = _services.GetService<IREditorSettings>();

            return DoFormatAction(snapshot, () => {
                var formattedText = new RFormatter(settings.FormatOptions).Format(snapshot.GetText());
                snapshot.EditorBuffer.Replace(TextRange.FromBounds(0, snapshot.Length), formattedText);
            });
        }

        public TextEdit[] FormatRange(IEditorBufferSnapshot snapshot, Range range) {
            var editorBuffer = snapshot.EditorBuffer;
            var editorView = new EditorView(editorBuffer, range.ToTextRange(snapshot).Start);
            var rangeFormatter = new RangeFormatter(_services, editorView, editorBuffer, new IncrementalTextChangeHandler());

             return DoFormatAction(snapshot, () => rangeFormatter.FormatRange(range.ToTextRange(snapshot)));
        }

        public TextEdit[] Autoformat(IEditorBufferSnapshot snapshot, Position position, string typedChar) {
            var editorBuffer = snapshot.EditorBuffer;
            var editorView = new EditorView(editorBuffer, position.ToStreamPosition(snapshot));
            var formatter = new AutoFormat(_services, editorView, editorBuffer, new IncrementalTextChangeHandler());

            // AST build happens asynchronously, give it a chance to finish since 
            // up -to-date AST improves outcome of the on-type formatting.
            var document = editorBuffer.GetEditorDocument<IREditorDocument>();
            if (!document.EditorTree.IsReady) {
                SpinWait.SpinUntil(() => document.EditorTree.IsReady, 50);
            }

            return DoFormatAction(snapshot, () => formatter.HandleTyping(typedChar[0], position.ToStreamPosition(snapshot)));
        }

        private TextEdit[] DoFormatAction(IEditorBufferSnapshot snapshot, Action action) {
            if (snapshot == null) {
                return new TextEdit[0];
            }

            var before = snapshot;
            action();
            var after = snapshot.EditorBuffer.CurrentSnapshot;

            return GetDifference(before, after);
        }

        /// <summary>
        /// Determines whitespace difference between two snapshots
        /// </summary>
        /// <param name="before">Snapshot before the change</param>
        /// <param name="after">Snapshot after the change</param>
        /// <returns></returns>
        private static TextEdit[] GetDifference(IEditorBufferSnapshot before, IEditorBufferSnapshot after) {
            var tokenizer = new RTokenizer();
            var oldTokens = tokenizer.Tokenize(before.GetText());
            var newTokens = tokenizer.Tokenize(after.GetText());

            if (newTokens.Count != oldTokens.Count) {
                return new[] { new TextEdit {
                    newText = after.GetText(),
                    range = TextRange.FromBounds(0, before.Length).ToLineRange(before)
                }};
            }

            var edits = new List<TextEdit>();
            var oldEnd = before.Length;
            var newEnd = after.Length;
            for (var i = newTokens.Count - 1; i >= 0; i--) {
                var oldText = before.GetText(TextRange.FromBounds(oldTokens[i].End, oldEnd));
                var newText = after.GetText(TextRange.FromBounds(newTokens[i].End, newEnd));
                if (oldText != newText) {
                    var range = new TextRange(oldTokens[i].End, oldEnd - oldTokens[i].End);
                    edits.Add(new TextEdit {
                        range = range.ToLineRange(before),
                        newText = newText
                    });

                }
                oldEnd = oldTokens[i].Start;
                newEnd = newTokens[i].Start;
            }

            var r = TextRange.FromBounds(0, oldEnd);
            var oldTextBeforeFitstToken = before.GetText(r);
            var newTextBeforeFirstToken = after.GetText(TextRange.FromBounds(0, newEnd));

            if (oldTextBeforeFitstToken != newTextBeforeFirstToken) {
                edits.Add(new TextEdit {
                    newText = newTextBeforeFirstToken,
                    range = r.ToLineRange(before)
                });
            }

            return edits.ToArray();
        }
    }
}

