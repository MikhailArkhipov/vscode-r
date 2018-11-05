// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using Microsoft.Common.Core.Services;
using Microsoft.Common.Core.Shell;
using Microsoft.Languages.Editor.Text;
using Microsoft.R.Core.Parser;
using Microsoft.R.Editor;
using Microsoft.R.Editor.Document;
using Microsoft.R.Editor.Tree;
using Microsoft.R.Editor.Validation;
using Microsoft.R.Editor.Validation.Errors;
using Microsoft.R.LanguageServer.Extensions;
using Microsoft.R.LanguageServer.Threading;

namespace Microsoft.R.LanguageServer.Validation {
    internal sealed class DiagnosticsPublisher {
        private readonly ConcurrentQueue<IValidationError> _resultsQueue;
        private readonly IMainThreadPriority _mainThread;
        private readonly IREditorSettings _settings;
        private readonly IIdleTimeService _idleTime;
        private readonly IClient _client;
        private readonly Uri _documentUri;
        private IREditorDocument _document;
        private List<Diagnostic> _lastDiagnostic = new List<Diagnostic>();

        public DiagnosticsPublisher(IREditorDocument document, Uri documentUri, IServiceContainer services) {
            _client = services.GetService<IClient>();
            _document = document;
            _documentUri = documentUri;

            _settings = services.GetService<IREditorSettings>();
            _idleTime = services.GetService<IIdleTimeService>();
            _mainThread = services.GetService<IMainThreadPriority>();

            var validator = _document.EditorBuffer.GetService<TreeValidator>();
            validator.Cleared += OnCleared;
            _resultsQueue = validator.ValidationResults;
            _idleTime.Idle += OnIdle;

            _document.Closing += OnDocumentClosing;
            _document.EditorTree.UpdateCompleted += OnTreeUpdateCompleted;
        }

        private void OnIdle(object sender, EventArgs eventArgs) {
            if (!_settings.SyntaxCheckEnabled || _document == null) {
                return;
            }

            var errors = _resultsQueue.ToArray();
            var diagnostic = new List<Diagnostic>();

            foreach (var e in errors.Where(e => !string.IsNullOrEmpty(e.Message))) {
                var range = GetRange(e);
                if (range == null) {
                    continue;
                }
                diagnostic.Add(new Diagnostic {
                    message = e.Message,
                    severity = ToDiagnosticSeverity(e.Severity),
                    range = range.Value,
                    source = "R"
                });
            }

            if (!diagnostic.SequenceEqual(_lastDiagnostic, new DiagnosticComparer())) {
                _lastDiagnostic = diagnostic;
                _mainThread.Post(() => _client.PublishDiagnostics(_documentUri, diagnostic), ThreadPostPriority.Idle);
            }
        }

        private static DiagnosticSeverity ToDiagnosticSeverity(ErrorSeverity s) {
            switch (s) {
                case ErrorSeverity.Warning:
                    return DiagnosticSeverity.Warning;
                case ErrorSeverity.Informational:
                    return DiagnosticSeverity.Information;
            }
            return DiagnosticSeverity.Error;
        }

        private Range? GetRange(IValidationError e) {
            try {
                return _document.EditorBuffer.ToLineRange(e.Start, e.End);
            } catch (ArgumentException) { }
            return null;
        }

        private void OnCleared(object sender, EventArgs e) => ClearAllDiagnostic();

        private void OnTreeUpdateCompleted(object sender, TreeUpdatedEventArgs e) {
            if (e.UpdateType != TreeUpdateType.PositionsOnly || _settings.LintOptions.Enabled) {
                ClearAllDiagnostic();
            }
        }

        private void ClearAllDiagnostic() {
            _client.PublishDiagnostics(_documentUri, Enumerable.Empty<Diagnostic>());
            _lastDiagnostic = Enumerable.Empty<Diagnostic>().ToList();
        }

        private void OnDocumentClosing(object sender, EventArgs e) {
            if (_document != null) {
                _idleTime.Idle -= OnIdle;

                _document.EditorTree.UpdateCompleted -= OnTreeUpdateCompleted;
                _document.Closing -= OnDocumentClosing;
                _document = null;
            }
        }

        private class DiagnosticComparer : IEqualityComparer<Diagnostic> {
            public bool Equals(Diagnostic x, Diagnostic y)
                => x.range.start.line == y.range.start.line && x.range.start.character == y.range.start.character &&
                   x.range.end.line == y.range.end.line && x.range.end.character == y.range.end.character &&
                   x.message == y.message;

            public int GetHashCode(Diagnostic obj) => obj?.GetHashCode() ?? 0;
        }
    }
}
