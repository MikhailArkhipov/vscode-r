﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.Common.Core.Services;
using Microsoft.Languages.Core;
using Microsoft.Languages.Editor.Completions;
using Microsoft.R.Core.Tokens;
using Microsoft.R.Editor.Completions.Engine;
using Microsoft.R.Editor.Document;
using Newtonsoft.Json.Linq;

namespace Microsoft.R.LanguageServer.Completions {
    internal sealed class CompletionManager {
        private readonly RCompletionEngine _completionEngine;

        public CompletionManager(IServiceContainer services) {
            _completionEngine = new RCompletionEngine(services);
        }

        public async Task<CompletionList> GetCompletionsAsync(IRIntellisenseContext context) {
            context.EditorBuffer.GetEditorDocument<IREditorDocument>().EditorTree.EnsureTreeReady();

            var providers = _completionEngine.GetCompletionForLocation(context);
            if (providers == null || providers.Count == 0) {
                return new CompletionList();
            }

            // Do not generate thousands of items, VSCode cannot handle that.
            // Filter based on the text typed so far right away.
            var prefix = GetFilterPrefix(context);
            var completions = (await Task.WhenAll(providers.Select(p => p.GetEntriesAsync(context, prefix)))).SelectMany(t => t).ToList();

            if (providers.All(p => p.AllowSorting)) {
                completions.Sort(new CompletionEntryComparer(StringComparison.OrdinalIgnoreCase));
                completions.RemoveDuplicates(new CompletionEntryComparer(StringComparison.Ordinal));
            }

            var sorted = new List<ICompletionEntry>();
            sorted.AddRange(completions.Where(c => c.DisplayText.EndsWith("=", StringComparison.Ordinal)));
            sorted.AddRange(completions.Where(c => char.IsLetter(c.DisplayText[0]) && !c.DisplayText.EndsWith("=", StringComparison.Ordinal)));

            var items = sorted
                .Select(c => new CompletionItem {
                    label = c.DisplayText,
                    insertText = c.InsertionText,
                    kind = (CompletionItemKind)c.ImageSource,
                    documentation = new MarkupContent {
                        value = c.Description
                    },
                    data = c.Data is string @string ? JToken.FromObject(@string) : null
                }).ToList();

            return new CompletionList {
                isIncomplete = true,
                items = items.ToArray()
            };
        }

        private static string GetFilterPrefix(IRIntellisenseContext context) {
            var snapshot = context.EditorBuffer.CurrentSnapshot;
            var line = snapshot.GetLineFromPosition(context.Position);
            var text = line.GetText();

            int i;
            var offset = context.Position - line.Start;
            for (i = offset - 1; i >= 0 && RTokenizer.IsIdentifierCharacter(text[i]); i--) { }

            i = Math.Min(offset, i + 1);
            return text.Substring(i, offset - i);
        }
    }
}
