// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Services;
using Microsoft.R.Components.InteractiveWorkflow;
using Microsoft.R.Editor.Functions;
using Microsoft.R.LanguageServer.Commands;
using Microsoft.R.LanguageServer.Documents;
using Microsoft.R.LanguageServer.InteractiveWorkflow;
using Microsoft.R.LanguageServer.Server;
using Microsoft.R.LanguageServer.Services;
using Microsoft.R.LanguageServer.Threading;
using Microsoft.R.Platform.Interpreters;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using StreamJsonRpc;

namespace Microsoft.R.LanguageServer {
    public class LanguageServer : IDisposable {
        /// <remarks>
        /// In VS Code editor operations such as formatting are not supposed
        /// to change local copy of the text buffer. Instead, they return
        /// a set of edits that VS Code applies to its buffer and then sends
        /// <see cref="didChange(TextDocumentIdentifier, ICollection{TextDocumentContentChangeEvent})"/>
        /// event. However, existing R formatters works by modifying underlying buffer.
        /// Therefore, in formatting operations we let formatter to change local copy 
        /// of the buffer, then calculate difference with the original state and send edits
        /// to VS Code, which then will invokes 'didChange'. Since local buffer is already 
        /// up to date, we must ignore this call.
        /// </remarks>
        private static volatile bool _ignoreNextChange;
        private readonly IServiceContainer _services;
        private readonly CancellationTokenSource _sessionTokenSource = new CancellationTokenSource();

        private IDocumentCollection _documents;
        private IIdleTimeTracker _idleTimeTracker;
        private IMainThreadPriority _mainThread;
        private IFunctionIndex _functionIndex;
        private InitializeParams _initParams;
        private IREvalSession _evalSession;
        private bool _shutdown;

        private IMainThreadPriority MainThreadPriority => _mainThread ??= _services.GetService<IMainThreadPriority>();
        private IDocumentCollection Documents => _documents ??= _services.GetService<IDocumentCollection>();
        private IIdleTimeTracker IdleTimeTracker => _idleTimeTracker ??= _services.GetService<IIdleTimeTracker>();
        private IFunctionIndex FunctionIndex => _functionIndex ??= _services.GetService<IFunctionIndex>();
        private IREvalSession EvalSession => _evalSession ??= _services.GetService<IREvalSession>();

        public LanguageServer(IServiceContainer services) {
            _services = services;
        }

        public CancellationToken Start() => _sessionTokenSource.Token;

        public void Dispose() { }

        [JsonRpcMethod("textDocument/hover")]
        public Task<Hover> hover(JToken token, CancellationToken ct) {
            var p = token.ToObject<TextDocumentPositionParams>();
            var doc = Documents.GetDocument(p.textDocument.uri);
            return doc != null ? doc.GetHoverAsync(p.position, ct) : Task.FromResult((Hover)null);
        }

        [JsonRpcMethod("textDocument/signatureHelp")]
        public Task<SignatureHelp> signatureHelp(JToken token, CancellationToken ct) {
            return MainThreadPriority.SendAsync(async () => {
                var p = token.ToObject<TextDocumentPositionParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                return doc != null ? await doc.GetSignatureHelpAsync(p.position) : new SignatureHelp();
            }, ThreadPostPriority.Background);
        }

        [JsonRpcMethod("textDocument/didOpen")]
        public void didOpen(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            var p = token.ToObject<DidOpenTextDocumentParams>();
            MainThreadPriority.Post(() => Documents.AddDocument(p.textDocument.text, p.textDocument.uri), ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/didChange")]
        public Task didChange(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();

            if (_ignoreNextChange) {
                _ignoreNextChange = false;
                return Task.CompletedTask;
            }

            return MainThreadPriority.SendAsync(() => {
                var p = token.ToObject<DidChangeTextDocumentParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                doc?.ProcessChanges(p.contentChanges);
                return true;
            }, ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/willSave")]
        public void willSave(JToken token, CancellationToken ct) { }

        [JsonRpcMethod("textDocument/didClose")]
        public void didClose(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            var p = token.ToObject<DidCloseTextDocumentParams>();
            MainThreadPriority.Post(() => Documents.RemoveDocument(p.textDocument.uri), ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/completion")]
        public Task<CompletionList> completion(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            var p = token.ToObject<CompletionParams>();
            return MainThreadPriority.SendAsync(async () => {
                var doc = Documents.GetDocument(p.textDocument.uri);
                return doc != null ? await doc.GetCompletionsAsync(p.position) : new CompletionList();
            }, ThreadPostPriority.Background);
        }

        // The request is sent from the client to the server to resolve additional information
        // for a given completion item.
        [JsonRpcMethod("completionItem/resolve")]
        public async Task<CompletionItem> resolve(JToken token, CancellationToken ct) {
            var item = token.ToObject<CompletionItem>();
            if (item.kind != CompletionItemKind.Function) {
                return item;
            }
            var info = await FunctionIndex.GetFunctionInfoAsync(item.label, ((JToken)item.data).Type == JTokenType.String ? (string)item.data : null);
            if (info != null) {
                item.documentation = new MarkupContent {
                    value = info.Description.RemoveLineBreaks()
                };
            }
            return item;
        }

        [JsonRpcMethod("textDocument/formatting")]
        public Task<TextEdit[]> formatting(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            return MainThreadPriority.SendAsync(() => {
                var p = token.ToObject<DocumentFormattingParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                var result = doc != null ? doc.Format() : Array.Empty<TextEdit>();
                _ignoreNextChange = !IsEmptyChange(result);
                return result;
            }, ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/rangeFormatting")]
        public Task<TextEdit[]> rangeFormatting(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            return MainThreadPriority.SendAsync(() => {
                var p = token.ToObject<DocumentRangeFormattingParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                var result = doc.FormatRange(p.range);
                _ignoreNextChange = !IsEmptyChange(result);
                return result;
            }, ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/onTypeFormatting")]
        public Task<TextEdit[]> onTypeFormatting(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            return MainThreadPriority.SendAsync(() => {
                var p = token.ToObject<DocumentOnTypeFormattingParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                var result = doc.Autoformat(p.position, p.ch);
                _ignoreNextChange = !IsEmptyChange(result);
                return result;
            }, ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/documentSymbol")]
#pragma warning disable VSTHRD200 // Use "Async" suffix for async methods
        public Task<DocumentSymbol[]> documentSymbol(JToken token, CancellationToken ct) {
            return MainThreadPriority.SendAsync(() => {
                var p = token.ToObject<DocumentSymbolParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                return doc != null ? doc.GetSymbols(p.textDocument.uri) : Array.Empty<DocumentSymbol>();
            }, ct);
        }
#pragma warning restore VSTHRD200 // Use "Async" suffix for async methods

        [JsonRpcMethod("workspace/didChangeConfiguration")]
        public Task didChangeConfiguration(JToken token, CancellationToken ct) {
            IdleTimeTracker.NotifyUserActivity();
            var settings = new LanguageServerSettings();

            var s = token["settings"];
            var r = s?["r"];
            if (r == null) {
                return Task.CompletedTask;
            }

            settings.InterpreterIndex = GetSetting(r, "interpreter", 0);
            settings.InterpreterPath = GetSetting(r, "interpreterPath", string.Empty);

            var editor = r["editor"];
            settings.Editor.TabSize = GetSetting(editor, "tabSize", 2);
            settings.Editor.FormatOnType = GetSetting(editor, "formatOnType", true);
            settings.Editor.FormatScope = GetSetting(editor, "formatScope", true);
            settings.Editor.SpaceAfterComma = GetSetting(editor, "spaceAfterComma", true);
            settings.Editor.SpaceAfterKeyword = GetSetting(editor, "spaceAfterKeyword", true);
            settings.Editor.SpacesAroundEquals = GetSetting(editor, "spacesAroundEquals", true);
            settings.Editor.SpaceBeforeCurly = GetSetting(editor, "spaceBeforeCurly", true);
            settings.Editor.BreakMultipleStatements = GetSetting(editor, "breakMultipleStatements", true);
            settings.Editor.BracesOnNewLine = GetSetting(editor, "bracesOnNewLine", false);

            var linting = r["linting"];
            settings.Linting.Enabled = GetSetting(linting, "enable", false);
            settings.Linting.CamelCase = GetSetting(linting, "camelCase", true);
            settings.Linting.SnakeCase = GetSetting(linting, "snakeCase", false);
            settings.Linting.PascalCase = GetSetting(linting, "pascalCase", false);
            settings.Linting.UpperCase = GetSetting(linting, "upperCase", false);
            settings.Linting.MultipleDots = GetSetting(linting, "multipleDots", true);
            settings.Linting.NameLength = GetSetting(linting, "nameLength", true);
            settings.Linting.MaxNameLength = GetSetting(linting, "maxNameLength", 32);
            settings.Linting.TrueFalseNames = GetSetting(linting, "trueFalseNames", true);
            settings.Linting.AssignmentType = GetSetting(linting, "assignmentType", true);
            settings.Linting.SpacesAroundComma = GetSetting(linting, "spacesAroundComma", true);
            settings.Linting.SpacesAroundOperators = GetSetting(linting, "spacesAroundOperators", true);
            settings.Linting.CloseCurlySeparateLine = GetSetting(linting, "closeCurlySeparateLine", true);
            settings.Linting.SpaceBeforeOpenBrace = GetSetting(linting, "spaceBeforeOpenBrace", true);
            settings.Linting.SpacesInsideParenthesis = GetSetting(linting, "spacesInsideParenthesis", true);
            settings.Linting.NoSpaceAfterFunctionName = GetSetting(linting, "noSpaceAfterFunctionName", true);
            settings.Linting.OpenCurlyPosition = GetSetting(linting, "openCurlyPosition", true);
            settings.Linting.NoTabs = GetSetting(linting, "noTabs", true);
            settings.Linting.TrailingWhitespace = GetSetting(linting, "trailingWhitespace", true);
            settings.Linting.TrailingBlankLines = GetSetting(linting, "trailingBlankLines", true);
            settings.Linting.DoubleQuotes = GetSetting(linting, "doubleQuotes", true);
            settings.Linting.LineLength = GetSetting(linting, "lineLength", false);
            settings.Linting.MaxLineLength = GetSetting(linting, "maxLineLength", 132);
            settings.Linting.Semicolons = GetSetting(linting, "semicolons", true);
            settings.Linting.MultipleStatements = GetSetting(linting, "multipleStatements", true);

            return MainThreadPriority.SendAsync(() => {
                _services.GetService<ISettingsManager>().UpdateSettings(settings);
            }, ct);
        }

        private static T GetSetting<T>(JToken section, string settingName, T defaultValue) {
            var value = section?[settingName];
            try {
                return value != null ? value.ToObject<T>() : defaultValue;
            } catch (JsonException) {
            }
            return defaultValue;
        }

        [JsonRpcMethod("workspace/executeCommand")]
        public Task<object> executeCommand(string command, object[] arguments) {
            IdleTimeTracker.NotifyUserActivity();
            return _services.GetService<IController>().ExecuteAsync(command, arguments);
        }

        [JsonRpcMethod("initialize")]
        public InitializeResult initialize(JToken token, CancellationToken ct) {
            _initParams = token.ToObject<InitializeParams>();
            MonitorParentProcess(_initParams);

            return new InitializeResult {
                capabilities = new ServerCapabilities {
                    hoverProvider = true,
                    signatureHelpProvider = new SignatureHelpOptions {
                        triggerCharacters = new[] { "(", ",", ")" }
                    },
                    completionProvider = new CompletionOptions {
                        resolveProvider = true,
                        triggerCharacters = new[] { ".", ":", "$", "<", "%" }
                    },
                    textDocumentSync = new TextDocumentSyncOptions {
                        openClose = true,
                        willSave = true,
                        change = TextDocumentSyncKind.Incremental
                    },
                    documentFormattingProvider = true,
                    documentRangeFormattingProvider = true,
                    documentOnTypeFormattingProvider = new DocumentOnTypeFormattingOptions {
                        firstTriggerCharacter = ";",
                        moreTriggerCharacter = new[] { "}", "\n" }
                    },
                    documentSymbolProvider = true,
                    executeCommandProvider = new ExecuteCommandOptions {
                        commands = Controller.Commands
                    }
                }
            };
        }

        [JsonRpcMethod("shutdown")]
        public void shutdown() {
            // Shutdown, but do not exit.
            // https://microsoft.github.io/language-server-protocol/specification#shutdown
            _shutdown = true;
            _idleTimeTracker.Dispose();
        }

        [JsonRpcMethod("exit")]
        public void exit() {
            _sessionTokenSource.Cancel();
            _idleTimeTracker.Dispose();
            // Per https://microsoft.github.io/language-server-protocol/specification#exit
            Environment.Exit(_shutdown ? 0 : 1);
        }

        [JsonRpcMethod("$/cancelRequest")]
        public void cancelRequest(JToken token) { }

        #region R Commands
        [JsonRpcMethod("r/getInterpreterPath")]
        public string getInterpreterPath() {
            if (!IsRInstalled()) {
                return null;
            }

            var provider = _services.GetService<IRInteractiveWorkflowProvider>();
            var workflow = provider.GetOrCreate();
            var homePath = workflow.RSessions.Broker.ConnectionInfo.Uri.OriginalString.Replace('/', Path.DirectorySeparatorChar);

            var binPath = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? Path.Combine("bin", "x64", "R.exe")
                : Path.Combine("bin", "R");

            return Path.Combine(homePath, binPath);
        }

        private bool IsRInstalled() {
            var ris = _services.GetService<IRInstallationService>();
            var engines = ris
                .GetCompatibleEngines(new SupportedRVersionRange(3, 2, 4, 9))
                .OrderByDescending(x => x.Version)
                .ToList();

            return engines.Count > 0;
        }

        [JsonRpcMethod("r/execute")]
        public Task<string> execute(string code) {
            IdleTimeTracker.NotifyUserActivity();
            return EvalSession.ExecuteCodeAsync(code, CancellationToken.None);
        }

        [JsonRpcMethod("r/interrupt")]
        public Task interrupt() {
            IdleTimeTracker.NotifyUserActivity();
            return EvalSession.InterruptAsync(CancellationToken.None);
        }

        [JsonRpcMethod("r/reset")]
        public Task reset() => EvalSession.ResetAsync(CancellationToken.None);
        #endregion

        private bool IsEmptyChange(IEnumerable<TextEdit> changes)
            => changes.All(x => string.IsNullOrEmpty(x.newText) && IsRangeEmpty(x.range));

        private bool IsRangeEmpty(Range range)
            => range.start.line == range.end.line && range.start.character == range.end.character;

        private void MonitorParentProcess(InitializeParams p) {
            // Monitor parent process
            Process parentProcess = null;
            if (p.processId.HasValue) {
                try {
                    parentProcess = Process.GetProcessById(p.processId.Value);
                } catch (ArgumentException) { }

                Debug.Assert(parentProcess != null, "Parent process does not exist");
                if (parentProcess != null) {
                    parentProcess.Exited += (s, e) => _sessionTokenSource.Cancel();
                }
            }

            if (parentProcess != null) {
                Task.Run(async () => {
                    while (!_sessionTokenSource.IsCancellationRequested) {
                        await Task.Delay(2000);
                        if (parentProcess.HasExited) {
                            _sessionTokenSource.Cancel();
                        }
                    }
                }).DoNotWait();
            }
        }
    }
}
