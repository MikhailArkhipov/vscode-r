// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Services;
using Microsoft.R.Components.InteractiveWorkflow;
using Microsoft.R.Editor.Functions;
using Microsoft.R.LanguageServer.Commands;
using Microsoft.R.LanguageServer.Diagnostics;
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
    public class LanguageServer: IDisposable {
        /// <remarks>
        /// In VS Code editor operations such as formatting are not supposed
        /// to change local copy of the text buffer. Instead, they return
        /// a set of edits that VS Code applies to its buffer and then sends
        /// <see cref="didChange(TextDocumentIdentifier, ICollection{TextDocumentContentChangeEvent})"/>
        /// event. However, existing R formatters works by modifying underlying buffer.
        /// Therefore, in formatting operations we let formatter to change local copy 
        /// of the buffer, then calculate difference with the original state and send edits
        /// to VS Code, which then will ivokes 'didChange'. Since local buffer is already 
        /// up to date, we must ignore this call.
        /// </remarks>
        private static volatile bool _ignoreNextChange;
        private readonly IServiceContainer _services;
        private readonly CancellationTokenSource _sessionTokenSource = new CancellationTokenSource();

        private IDocumentCollection _documents;
        private IIdleTimeNotification _idleTimeNotification;
        private IMainThreadPriority _mainThread;
        private IFunctionIndex _functionIndex;
        private InitializeParams _initParams;
        private IREvalSession _evalSession;

        private IMainThreadPriority MainThreadPriority => _mainThread ?? (_mainThread = _services.GetService<IMainThreadPriority>());
        private IDocumentCollection Documents => _documents ?? (_documents = _services.GetService<IDocumentCollection>());
        private IIdleTimeNotification IdleTimeNotification => _idleTimeNotification ?? (_idleTimeNotification = _services.GetService<IIdleTimeNotification>());
        private IFunctionIndex FunctionIndex => _functionIndex ?? (_functionIndex = _services.GetService<IFunctionIndex>());
        private IREvalSession EvalSession => _evalSession ?? (_evalSession = _services.GetService<IREvalSession>());

        public LanguageServer(IServiceContainer services) {
            _services = services;
        }

        public CancellationToken Start() {
            return _sessionTokenSource.Token;
        }

        public void Dispose() { }

        [JsonRpcMethod("textDocument/hover")]
        public Task<Hover> hover(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/hover")) {
                var p = token.ToObject<TextDocumentPositionParams>();
                var doc = Documents.GetDocument(p.textDocument.uri);
                return doc != null ? doc.GetHoverAsync(p.position, ct) : Task.FromResult((Hover)null);
            }
        }

        [JsonRpcMethod("textDocument/signatureHelp")]
        public Task<SignatureHelp> signatureHelp(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/signatureHelp")) {
                return MainThreadPriority.SendAsync(async () => {
                    var p = token.ToObject<TextDocumentPositionParams>();
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    return doc != null ? await doc.GetSignatureHelpAsync(p.position) : new SignatureHelp();
                }, ThreadPostPriority.Background);
            }
        }

        [JsonRpcMethod("textDocument/didOpen")]
        public void didOpen(JToken token, CancellationToken ct) {
            IdleTimeNotification.NotifyUserActivity();
            var p = token.ToObject<DidOpenTextDocumentParams>();
            MainThreadPriority.Post(() => Documents.AddDocument(p.textDocument.text, p.textDocument.uri), ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/didChange")]
        public async Task didChange(JToken token, CancellationToken ct) {
            if (_ignoreNextChange) {
                _ignoreNextChange = false;
                return;
            }

            IdleTimeNotification.NotifyUserActivity();

            using (new DebugMeasureTime("textDocument/didChange")) {
                await MainThreadPriority.SendAsync(async () => {
                    var p = token.ToObject<DidChangeTextDocumentParams>();
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    if (doc != null) {
                        await doc.ProcessChangesAsync(p.contentChanges);
                    }
                    return true;
                }, ThreadPostPriority.Normal);
            }
        }

        [JsonRpcMethod("textDocument/willSave")]
        public void willSave(JToken token, CancellationToken ct) { }

        [JsonRpcMethod("textDocument/didClose")]
        public void didClose(JToken token, CancellationToken ct) {
            var p = token.ToObject<DidCloseTextDocumentParams>();
            MainThreadPriority.Post(() => Documents.RemoveDocument(p.textDocument.uri), ThreadPostPriority.Normal);
        }

        [JsonRpcMethod("textDocument/completion")]
        public Task<CompletionList> completion(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/completion")) {
                var p = token.ToObject<CompletionParams>();
                return MainThreadPriority.SendAsync(async () => {
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    return doc != null ? await doc.GetCompletionsAsync(p.position) : new CompletionList();
                }, ThreadPostPriority.Background);
            }
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
                    kind = "plaintext",
                    value = info.Description.RemoveLineBreaks()
                };
            }
            return item;
        }

        [JsonRpcMethod("textDocument/formatting")]
        public Task<TextEdit[]> formatting(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/formatting")) {
                return MainThreadPriority.SendAsync(async () => {
                    var p = token.ToObject<DocumentFormattingParams>();
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    var result = doc != null ? await doc.FormatAsync() : new TextEdit[0];
                    _ignoreNextChange = !IsEmptyChange(result);
                    return result;
                }, ThreadPostPriority.Background);
            }
        }

        [JsonRpcMethod("textDocument/rangeFormatting")]
        public Task<TextEdit[]> rangeFormatting(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/rangeFormatting")) {
                return MainThreadPriority.SendAsync(async () => {
                    var p = token.ToObject<DocumentRangeFormattingParams>();
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    var result = await doc.FormatRangeAsync(p.range);
                    _ignoreNextChange = !IsEmptyChange(result);
                    return result;
                }, ThreadPostPriority.Background);
            }
        }

        [JsonRpcMethod("textDocument/onTypeFormatting")]
        public Task<TextEdit[]> onTypeFormatting(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/onTypeFormatting")) {
                return MainThreadPriority.SendAsync(async () => {
                    var p = token.ToObject<DocumentOnTypeFormattingParams>();
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    var result = await doc.AutoformatAsync(p.position, p.ch);
                    _ignoreNextChange = !IsEmptyChange(result);
                    return result;
                }, ThreadPostPriority.Background);
            }
        }

        [JsonRpcMethod("textDocument/documentSymbol")]
        public Task<SymbolInformation[]> documentSymbol(JToken token, CancellationToken ct) {
            using (new DebugMeasureTime("textDocument/documentSymbol")) {
                return MainThreadPriority.SendAsync(() => {
                    var p = token.ToObject<DocumentSymbolParams>();
                    var doc = Documents.GetDocument(p.textDocument.uri);
                    return doc != null ? doc.GetSymbols(p.textDocument.uri) : new SymbolInformation[0];
                });
            }
        }

        [JsonRpcMethod("workspace/didChangeConfiguration")]
        public Task didChangeConfiguration(JToken token, CancellationToken ct) {
            return MainThreadPriority.SendAsync(() => {
                var settings = new LanguageServerSettings();

                var r = token["settings"];
                if (r == null) {
                    return;
                }

                settings.Interpreter = GetSetting(r, "interpreter", 0);

                var editor = r["editor"];
                settings.Editor.BreakMultipleStatements = GetSetting<bool>(editor, "breakMultipleStatements", true);
                settings.Editor.FormatScope = GetSetting<bool>(editor, "formatScope", true);
                settings.Editor.SpaceAfterKeyword = GetSetting<bool>(editor, "spaceAfterKeyword", true);
                settings.Editor.SpacesAroundEquals = GetSetting<bool>(editor, "spacesAroundEquals", true);
                settings.Editor.SpaceBeforeCurly = GetSetting<bool>(editor, "spaceBeforeCurly", true);
                settings.Editor.TabSize = GetSetting<int>(editor, "tabSize", 4);

                var linting = r["linting"];
                settings.Linting.Enabled = GetSetting<bool>(linting, "enabled", false);
                settings.Linting.CamelCase = GetSetting<bool>(linting, "camelCase", true);
                settings.Linting.SnakeCase = GetSetting<bool>(linting, "snakeCase", false);
                settings.Linting.PascalCase = GetSetting<bool>(linting, "pascalCase", false);
                settings.Linting.UpperCase = GetSetting<bool>(linting, "upperCase", false);
                settings.Linting.MultipleDots = GetSetting<bool>(linting, "multipleDots", true);
                settings.Linting.NameLength = GetSetting<bool>(linting, "nameLength", true);
                settings.Linting.MaxNameLength = GetSetting<int>(linting, "maxNameLength", 32);
                settings.Linting.TrueFalseNames = GetSetting<bool>(linting, "trueFalseNames", true);
                settings.Linting.AssignmentType = GetSetting<bool>(linting, "assignmentType", true);
                settings.Linting.SpacesAroundComma = GetSetting<bool>(linting, "spacesAroundComma", true);
                settings.Linting.SpacesAroundOperators = GetSetting<bool>(linting, "spacesAroundOperators", true);
                settings.Linting.CloseCurlySeparateLine = GetSetting<bool>(linting, "closeCurlySeparateLine", true);
                settings.Linting.SpaceBeforeOpenBrace = GetSetting<bool>(linting, "spaceBeforeOpenBrace", true);
                settings.Linting.SpacesInsideParenthesis = GetSetting<bool>(linting, "spacesInsideParenthesis", true);
                settings.Linting.NoSpaceAfterFunctionName = GetSetting<bool>(linting, "noSpaceAfterFunctionName", true);
                settings.Linting.OpenCurlyPosition = GetSetting<bool>(linting, "openCurlyPosition", true);
                settings.Linting.NoTabs = GetSetting<bool>(linting, "noTabs", true);
                settings.Linting.TrailingWhitespace = GetSetting<bool>(linting, "trailingWhitespace", true);
                settings.Linting.TrailingBlankLines = GetSetting<bool>(linting, "trailingBlankLines", true);
                settings.Linting.DoubleQuotes = GetSetting<bool>(linting, "doubleQuotes", true);
                settings.Linting.LineLength = GetSetting<bool>(linting, "lineLength", false);
                settings.Linting.MaxLineLength = GetSetting<int>(linting, "maxLineLength", 132);
                settings.Linting.Semicolons = GetSetting<bool>(linting, "semicolons", true);
                settings.Linting.MultipleStatements = GetSetting<bool>(linting, "multipleStatements", true);

                _services.GetService<ISettingsManager>().UpdateSettings(settings);
            });
        }

        private T GetSetting<T>(JToken section, string settingName, T defaultValue) {
            var value = section?[settingName];
            try {
                return value != null ? value.ToObject<T>() : defaultValue;
            } catch (JsonException) {
            }
            return defaultValue;
        }

        [JsonRpcMethod("workspace/executeCommand")]
        public Task<object> executeCommand(string command, object[] arguments)
            => _services.GetService<IController>().ExecuteAsync(command, arguments);

        [JsonRpcMethod("initialize")]
        public InitializeResult initialize(JToken token, CancellationToken ct) {
            _initParams = token.ToObject<InitializeParams>();

            return new InitializeResult {
                capabilities = new ServerCapabilities {
                    hoverProvider = true,
                    signatureHelpProvider = new SignatureHelpOptions {
                        triggerCharacters = new[] { "(", ",", ")" }
                    },
                    completionProvider = new CompletionOptions {
                        resolveProvider = true,
                        triggerCharacters = new[] { "." }
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

        [JsonRpcMethod("r/getInterpreterPath")]
        public string getInterpreterPath() {
            if (!IsRInstalled()) {
                return null;
            }

            var provider = _services.GetService<IRInteractiveWorkflowProvider>();
            var workflow = provider.GetOrCreate();
            var homePath = workflow.RSessions.Broker.ConnectionInfo.Uri.OriginalString.Replace('/', Path.DirectorySeparatorChar);
            var binPath = $"bin{Path.DirectorySeparatorChar}x64{Path.DirectorySeparatorChar}R.exe";
            return Path.Combine(homePath, binPath);
        }

        private bool IsRInstalled() {
            var ris = _services.GetService<IRInstallationService>();
            var engines = ris
                .GetCompatibleEngines(new SupportedRVersionRange(3, 2, 3, 9))
                .OrderByDescending(x => x.Version)
                .ToList();

            return engines.Count > 0;
        }

        [JsonRpcMethod("exit")]
        public void exit() {
            _sessionTokenSource.Cancel();
        }

        [JsonRpcMethod("$/cancelRequest")]
        public void cancelRequest(JToken token) {}

        [JsonRpcMethod("r/execute")]
        public Task<string> execute(string code) => EvalSession.ExecuteCodeAsync(code, CancellationToken.None);

        [JsonRpcMethod("r/interrupt")]
        public Task interrupt() => EvalSession.InterruptAsync(CancellationToken.None);

        [JsonRpcMethod("r/reset")]
        public Task reset() => EvalSession.ResetAsync(CancellationToken.None);

        private bool IsEmptyChange(IEnumerable<TextEdit> changes)
            => changes.All(x => string.IsNullOrEmpty(x.newText) && IsRangeEmpty(x.range));

        private bool IsRangeEmpty(Range range)
            => range.start.line == range.end.line && range.start.character == range.end.character;
    }
}
