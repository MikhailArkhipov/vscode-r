﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core.Services;
using Microsoft.Languages.Core.Text;
using Microsoft.Languages.Editor.Completions;
using Microsoft.Languages.Editor.Text;
using Microsoft.R.Editor.Completions;
using Microsoft.R.Editor.Document;
using Microsoft.R.LanguageServer.Completions;
using Microsoft.R.LanguageServer.Extensions;
using Microsoft.R.LanguageServer.Formatting;
using Microsoft.R.LanguageServer.Symbols;
using Microsoft.R.LanguageServer.Text;
using Microsoft.R.LanguageServer.Validation;

namespace Microsoft.R.LanguageServer.Documents {
    internal sealed class DocumentEntry : IDisposable {
        private readonly IServiceContainer _services;
        private readonly CompletionManager _completionManager;
        private readonly SignatureManager _signatureManager;
        private readonly DiagnosticsPublisher _diagnosticsPublisher;
        private readonly CodeFormatter _formatter;
        private readonly DocumentSymbolsProvider _symbolsProvider;

        public IEditorBuffer EditorBuffer { get; }
        public IREditorDocument Document { get; }

        public DocumentEntry(string content, Uri uri, IServiceContainer services) {
            _services = services;

            EditorBuffer = new EditorBuffer(content, "R");
            Document = new REditorDocument(EditorBuffer, services, false);

            _completionManager = new CompletionManager(services);
            _signatureManager = new SignatureManager(services);
            _diagnosticsPublisher = new DiagnosticsPublisher(Document, uri, services);
            _formatter = new CodeFormatter(_services);
            _symbolsProvider = new DocumentSymbolsProvider();
        }

        public void ProcessChanges(TextDocumentContentChangedEvent[] contentChanges) {

            foreach (var change in contentChanges.Where(c => c.range.HasValue)) {
                var position = EditorBuffer.ToStreamPosition(change.range.Value.start);
                var range = new TextRange(position, change.rangeLength ?? 0);
                if (!string.IsNullOrEmpty(change.text)) {
                    // Insert or replace
                    if (change.rangeLength == 0) {
                        EditorBuffer.Insert(position, change.text);
                    } else {
                        EditorBuffer.Replace(range, change.text);
                    }
                } else {
                    EditorBuffer.Delete(range);
                }
            }
        }

        [DebuggerStepThrough]
        public void Dispose() => Document?.Close();

        [DebuggerStepThrough]
        public Task<CompletionList> GetCompletionsAsync(Position position)
            => _completionManager.GetCompletionsAsync(CreateContext(position));

        public Task<SignatureHelp> GetSignatureHelpAsync(Position position)
            => _signatureManager.GetSignatureHelpAsync(CreateContext(position));

        [DebuggerStepThrough]
        public Task<Hover> GetHoverAsync(Position position, CancellationToken ct)
            => _signatureManager.GetHoverAsync(CreateContext(position), ct);

        [DebuggerStepThrough]
        public TextEdit[] Format()
            => _formatter.Format(EditorBuffer.CurrentSnapshot);

        [DebuggerStepThrough]
        public TextEdit[] FormatRange(Range range)
            => _formatter.FormatRange(EditorBuffer.CurrentSnapshot, range);

        [DebuggerStepThrough]
        public TextEdit[] Autoformat(Position position, string typeChar)
            => _formatter.Autoformat(EditorBuffer.CurrentSnapshot, position, typeChar);

        [DebuggerStepThrough]
        public SymbolInformation[] GetSymbols(Uri uri)
            => _symbolsProvider.GetSymbols(Document, uri);

        private IRIntellisenseContext CreateContext(Position position) {
            var bufferPosition = EditorBuffer.ToStreamPosition(position);
            var session = new EditorIntellisenseSession(new EditorView(EditorBuffer, position.ToStreamPosition(EditorBuffer.CurrentSnapshot)), _services);
            return new RIntellisenseContext(session, EditorBuffer, Document.EditorTree, bufferPosition);
        }
    }
}
