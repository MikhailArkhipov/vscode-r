﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.R.LanguageServer {
    [Serializable]
    public struct ResponseError {
        public int code;
        public string message;
    }

    [Serializable]
    public struct Location {
        public Uri uri;
        public Range range;
    }

    [Serializable]
    public struct Position {
        public int line;
        public int character;
    }

    [Serializable]
    public struct Range {
        public Position start, end;
    }

    [Serializable]
    public class MarkupContent {
        public string kind;
        public string value;
    }

    [Serializable]
    public struct Command {
        /// <summary>
        /// Title of the command, like `save`.
        /// </summary>
        public string title;

        /// <summary>
        /// The identifier of the actual command handler.
        /// </summary>
        public string command;

        /// <summary>
        /// Arguments that the command handler should be invoked with.
        /// </summary>
        public object[] arguments;
    }

    [Serializable]
    public struct TextEdit {
        public Range range;
        public string newText;
    }

    [Serializable]
    public struct TextDocumentEdit {
        public VersionedTextDocumentIdentifier textDocument;
        public TextEdit[] edits;
    }

    [Serializable]
    public struct WorkspaceEdit {
        public Dictionary<Uri, TextEdit[]> changes;
        public TextDocumentEdit[] documentChanges;
    }

    [Serializable]
    public class TextDocument {
        public Uri uri;
    }

    [Serializable]
    public class TextDocumentItem {
        public Uri uri;
        public string languageId;
        public int version;
        public string text;
    }

    [Serializable]
    public struct VersionedTextDocumentIdentifier {
        public Uri uri;
        public int? version;
        public int? _fromVersion;
    }

    [Serializable]
    public class WorkspaceClientCapabilities {
        public bool applyEdit;

        public struct WorkspaceEditCapabilities { public bool documentChanges; }
        public WorkspaceEditCapabilities? documentChanges;

        public struct DidConfigurationChangeCapabilities { public bool dynamicRegistration; }
        public DidConfigurationChangeCapabilities? didConfigurationChange;

        public struct DidChangeWatchedFilesCapabilities { public bool dynamicRegistration; }
        public DidChangeWatchedFilesCapabilities? didChangeWatchedFiles;

        [Serializable]
        public struct SymbolCapabilities {
            public bool dynamicRegistration;

            [Serializable]
            public struct SymbolKindCapabilities {
                /// <summary>
                /// The symbol kind values the client supports. When this
                /// property exists the client also guarantees that it will
                /// handle values outside its set gracefully and falls back
                /// to a default value when unknown.
                /// 
                /// If this property is not present the client only supports
                /// the symbol kinds from `File` to `Array` as defined in
                /// the initial version of the protocol.
                /// </summary>
                public SymbolKind[] valueSet;
            }
            public SymbolKindCapabilities? symbolKind;
        }

        public SymbolCapabilities? symbol;

        public struct ExecuteCommandCapabilities { public bool dynamicRegistration; }
        public ExecuteCommandCapabilities? executeCommand;
    }

    [Serializable]
    public class TextDocumentClientCapabilities {
        [Serializable]
        public struct SynchronizationCapabilities {
            public bool dynamicRegistration;
            public bool willSave;
            /// <summary>
            /// The client supports sending a will save request and
            /// waits for a response providing text edits which will
            /// be applied to the document before it is saved.
            /// </summary>
            public bool willSaveWaitUntil;
            public bool didSave;
        }
        public SynchronizationCapabilities? synchronization;

        [Serializable]
        public struct CompletionCapabilities {
            public bool dynamicRegistration;

            [Serializable]
            public struct CompletionItemCapabilities {
                /// <summary>
                /// Client supports snippets as insert text.
                /// 
                /// A snippet can define tab stops and placeholders with `$1`, `$2`
                /// and `${3:foo}`. `$0` defines the final tab stop, it defaults to
                /// the end of the snippet. Placeholders with equal identifiers are linked,
                /// that is typing in one will update others too.
                /// </summary>
                public bool snippetSupport;

                public bool commitCharactersSupport;

                public string[] documentationFormat;
            }
            public CompletionItemCapabilities? completionItem;

            [Serializable]
            public struct CompletionItemKindCapabilities {
                /// <summary>
                /// The completion item kind values the client supports. When this
                /// property exists the client also guarantees that it will
                /// handle values outside its set gracefully and falls back
                /// to a default value when unknown.
                /// 
                /// If this property is not present the client only supports
                /// the completion items kinds from `Text` to `Reference` as defined in
                /// the initial version of the protocol.
                /// </summary>
                public SymbolKind[] valueSet;
            }
            public CompletionItemKindCapabilities? completionItemKind;

            /// <summary>
            /// The client supports to send additional context information for a
            /// `textDocument/completion` request.
            /// </summary>
            public bool contextSupport;
        }
        public CompletionCapabilities? completion;

        [Serializable]
        public struct HoverCapabilities {
            public bool dynamicRegistration;
            /// <summary>
            /// Client supports the follow content formats for the content
            /// property.The order describes the preferred format of the client.
            /// </summary>
            public string[] contentFormat;
        }
        public HoverCapabilities? hover;

        [Serializable]
        public struct SignatureHelpCapabilities {
            public bool dynamicRegistration;

            public struct SignatureInformationCapabilities {
                /// <summary>
                ///  Client supports the follow content formats for the documentation
                /// property.The order describes the preferred format of the client.
                /// </summary>
                public string[] documentationFormat;

                /// <summary>
                /// When true, the label in the returned signature information will
                /// only contain the function name. Otherwise, the label will contain
                /// the full signature.
                /// </summary>
                public bool? _shortLabel;
            }
            public SignatureInformationCapabilities? signatureInformation;
        }
        public SignatureHelpCapabilities? signatureHelp;

        [Serializable]
        public struct ReferencesCapabilities { public bool dynamicRegistration; }
        public ReferencesCapabilities? references;

        [Serializable]
        public struct DocumentHighlightCapabilities { public bool dynamicRegistration; }
        public DocumentHighlightCapabilities? documentHighlight;

        [Serializable]
        public struct DocumentSymbolCapabilities {
            public bool dynamicRegistration;
            public struct SymbolKindCapabilities {
                /// <summary>
                /// The symbol kind values the client supports. When this
                /// property exists the client also guarantees that it will
                /// handle values outside its set gracefully and falls back
                /// to a default value when unknown.
                /// 
                /// If this property is not present the client only supports
                /// the symbol kinds from `File` to `Array` as defined in
                /// the initial version of the protocol.
                /// </summary>
                public SymbolKind[] valueSet;
            }
            public SymbolKindCapabilities? symbolKind;

            /// <summary>
            /// The client support hierarchical document symbols.
            /// </summary>
            public bool? hierarchicalDocumentSymbolSupport;
        }
        public DocumentSymbolCapabilities? documentSymbol;

        [Serializable]
        public struct FormattingCapabilities { public bool dynamicRegistration; }
        public FormattingCapabilities? formatting;

        [Serializable]
        public struct RangeFormattingCapabilities { public bool dynamicRegistration; }
        public RangeFormattingCapabilities? rangeFormatting;

        [Serializable]
        public struct OnTypeFormattingCapabilities { public bool dynamicRegistration; }
        public OnTypeFormattingCapabilities? onTypeFormatting;

        public struct DefinitionCapabilities { public bool dynamicRegistration; }
        public DefinitionCapabilities? definition;

        [Serializable]
        public struct CodeActionCapabilities { public bool dynamicRegistration; }
        public CodeActionCapabilities? codeAction;

        [Serializable]
        public struct CodeLensCapabilities { public bool dynamicRegistration; }
        public CodeLensCapabilities? codeLens;

        [Serializable]
        public struct DocumentLinkCapabilities { public bool dynamicRegistration; }
        public DocumentLinkCapabilities? documentLink;

        [Serializable]
        public struct RenameCapabilities { public bool dynamicRegistration; }
        public RenameCapabilities? rename;
    }

    [Serializable]
    public class ClientCapabilities {
        public WorkspaceClientCapabilities workspace;
        public TextDocumentClientCapabilities textDocument;
    }

    [Serializable]
    public struct CompletionOptions {
        /// <summary>
        /// The server provides support to resolve additional
        /// information for a completion item.
        /// </summary>
        public bool resolveProvider;
        /// <summary>
        /// The characters that trigger completion automatically.
        /// </summary>
        public string[] triggerCharacters;
    }

    [Serializable]
    public struct SignatureHelpOptions {
        /// <summary>
        /// The characters that trigger signature help
        /// automatically.
        /// </summary>
        public string[] triggerCharacters;
    }

    [Serializable]
    public struct CodeLensOptions {
        public bool resolveProvider;
    }

    [Serializable]
    public struct DocumentOnTypeFormattingOptions {
        public string firstTriggerCharacter;
        public string[] moreTriggerCharacter;
    }

    [Serializable]
    public struct DocumentLinkOptions {
        public bool resolveProvider;
    }

    [Serializable]
    public struct ExecuteCommandOptions {
        public string[] commands;
    }

    [Serializable]
    public class SaveOptions {
        public bool includeText;
    }

    [Serializable]
    public class TextDocumentSyncOptions {
        /// <summary>
        /// Open and close notifications are sent to the server.
        /// </summary>
        public bool openClose;
        public TextDocumentSyncKind change;
        public bool willSave;
        public bool willSaveWaitUntil;
        public SaveOptions save;
    }

    [Serializable]
    public struct ServerCapabilities {
        public TextDocumentSyncOptions textDocumentSync;
        public bool hoverProvider;
        public CompletionOptions? completionProvider;
        public SignatureHelpOptions? signatureHelpProvider;
        public bool definitionProvider;
        public bool referencesProvider;
        public bool documentHighlightProvider;
        public bool documentSymbolProvider;
        public bool workspaceSymbolProvider;
        public bool codeActionProvider;
        public CodeLensOptions? codeLensProvider;
        public bool documentFormattingProvider;
        public bool documentRangeFormattingProvider;
        public DocumentOnTypeFormattingOptions? documentOnTypeFormattingProvider;
        public bool renameProvider;
        public DocumentLinkOptions? documentLinkProvider;
        public ExecuteCommandOptions? executeCommandProvider;
        public object experimental;
    }

    [Serializable]
    public struct MessageActionItem {
        public string title;
    }
  
    [Serializable]
    public struct TextDocumentContentChangedEvent {
        public Range? range;
        public int? rangeLength;
        public string text;
    }

    [Serializable]
    public class CompletionList {
        /// <summary>
        /// This list is not complete. Further typing should result in recomputing
        /// this list.
        /// </summary>
        public bool isIncomplete;
        public CompletionItem[] items;
    }

    [Serializable]
    public class CompletionItem {
        public string label;
        public CompletionItemKind kind;
        public string detail;
        public MarkupContent documentation;
        public string sortText;
        public string filterText;
        public bool? preselect; // VS Code 1.25+
        public string insertText;
        public InsertTextFormat insertTextFormat;
        public TextEdit? textEdit;
        public TextEdit[] additionalTextEdits;
        public string[] commitCharacters;
        public Command? command;
        public object data;
    }

    // Not in LSP spec
    [Serializable]
    public struct CompletionItemValue {
        public string description;
        public string documentation;
        public Reference[] references;
    }

    [Serializable]
    public class Hover {
        public MarkupContent contents;
        public Range? range;
    }

    [Serializable]
    public class SignatureHelp {
        public SignatureInformation[] signatures;
        public int activeSignature;
        public int activeParameter;
    }

    [Serializable]
    public class SignatureInformation {
        public string label;
        public MarkupContent documentation;
        public ParameterInformation[] parameters;
    }

    [Serializable]
    public class ParameterInformation {
        public object label;
        public MarkupContent documentation;
    }

    /// <summary>
    /// Used instead of Position when returning references so we can include
    /// the kind.
    /// </summary>
    [Serializable]
    public class Reference {
        public Uri uri;
        public Range range;
    }

    [Serializable]
    public struct DocumentHighlight {
        public Range range;
        public DocumentHighlightKind kind;
    }

    [Serializable]
    public struct DocumentSymbol {
        /// <summary>
        /// The name of this symbol.
        /// </summary>
        public string name;

        /// <summary>
        /// More detail for this symbol, e.g the signature of a function. If not provided the
        /// name is used.
        /// </summary>
        public string detail;

        /// <summary>
        /// The kind of this symbol.
        /// </summary>
        public SymbolKind kind;

        /// <summary>
        /// Indicates if this symbol is deprecated.
        /// </summary>
        public bool deprecated;

        /// <summary>
        /// The range enclosing this symbol not including leading/trailing whitespace but everything else
        /// like comments.This information is typically used to determine if the clients cursor is
        /// inside the symbol to reveal in the symbol in the UI.
        /// </summary>
        public Range range;

        /// <summary>
        /// The range that should be selected and revealed when this symbol is being picked, 
        /// e.g the name of a function. Must be contained by the `range`.
        /// </summary>
        public Range selectionRange;

        /// <summary>
        /// Children of this symbol, e.g. properties of a class.
        /// </summary>
        public DocumentSymbol[] children;
    }

    [Serializable]
    public struct SymbolInformation {
        public string name;
        public SymbolKind kind;
        public Location location;
        /// <summary>
        /// The name of the symbol containing this symbol. This information is for
        /// user interface purposes (e.g.to render a qualifier in the user interface
        /// if necessary). It can't be used to re-infer a hierarchy for the document
        /// symbols.
        /// </summary>
        public string containerName;
    }

    [Serializable]
    public struct CodeLens {
        public Range range;
        public Command? command;
        public object data;
    }

    [Serializable]
    public struct DocumentLink {
        public Range range;
        public Uri target;
    }

    [Serializable]
    public struct FormattingOptions {
        public int tabSize;
        public bool insertSpaces;
    }

    [Serializable]
    public class Diagnostic {
        public Range range;
        public DiagnosticSeverity severity;
        public string code;
        public string source;
        public string message;
    }

    public enum DiagnosticSeverity {
        Unspecified = 0,
        Error = 1,
        Warning = 2,
        Information = 3,
        Hint = 4
    }

    [Serializable]
    public class PublishDiagnosticsParams {
        public Uri uri;
        public Diagnostic[] diagnostics;
    }
}
