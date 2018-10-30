// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.R.LanguageServer {
    [Serializable]
    public struct InitializeResult {
        public ServerCapabilities? capabilities;
    }

    [Serializable]
    public struct InitializeParams {
        public int? processId;
        public string rootPath;
        public Uri rootUri;
        public ClientCapabilities capabilities;
    }

    public sealed class CommandEventArgs: EventArgs {
        public string command;
        public object[] arguments;
    }

    [Serializable]
    public struct DidChangeConfigurationParams {
        public object settings;
    }

    [Serializable]
    public struct WorkspaceSymbolParams {
        public string query;
    }

    [Serializable]
    public struct ExecuteCommandParams {
        public string command;
        public object[] arguments;
    }

    [Serializable]
    public struct DidOpenTextDocumentParams {
        public TextDocumentItem textDocument;
    }

    [Serializable]
    public struct DidChangeTextDocumentParams {
        public VersionedTextDocumentIdentifier textDocument;
        public TextDocumentContentChangedEvent[] contentChanges;
    }

    [Serializable]
    public struct WillSaveTextDocumentParams {
        public TextDocument textDocument;
        public TextDocumentSaveReason reason;
    }

    [Serializable]
    public struct DidSaveTextDocumentParams {
        public TextDocument textDocument;
        public string content;
    }

    [Serializable]
    public struct DidCloseTextDocumentParams {
        public TextDocument textDocument;
    }

    [Serializable]
    public struct TextDocumentPositionParams {
        public TextDocument textDocument;
        public Position position;
    }

    [Serializable]
    public struct CompletionParams {
        public TextDocument textDocument;
        public Position position;
        public CompletionContext? context;
    }

    [Serializable]
    public struct CompletionContext {
        public CompletionTriggerKind triggerKind;
        public string triggerCharacter;
    }

    [Serializable]
    public struct ReferencesParams {
        public TextDocument textDocument;
        public Position position;
        public ReferenceContext? context;
    }

    public struct ReferenceContext {
        public bool includeDeclaration;
    }

    [Serializable]
    public struct DocumentSymbolParams {
        public TextDocument textDocument;
    }

    [Serializable]
    public struct CodeActionParams {
        public TextDocument textDocument;
        public Range range;
        public CodeActionContext? context;

        /// <summary>
        /// The intended version that range applies to. The request may fail if
        /// the server cannot map correctly.
        /// </summary>
        public int? _version;
    }

    [Serializable]
    public struct CodeActionContext {
        public Diagnostic[] diagnostics;
    }

    [Serializable]
    public struct DocumentLinkParams {
        public TextDocument textDocument;
    }

    [Serializable]
    public struct DocumentFormattingParams {
        public TextDocument textDocument;
        public FormattingOptions options;
    }

    [Serializable]
    public struct DocumentRangeFormattingParams {
        public TextDocument textDocument;
        public Range range;
        public FormattingOptions options;
    }

    [Serializable]
    public struct DocumentOnTypeFormattingParams {
        public TextDocument textDocument;
        public Position position;
        public string ch;
        public FormattingOptions options;
    }

    [Serializable]
    public struct RenameParams {
        public TextDocument textDocument;
        public Position position;
        public string newName;
    }

    [Serializable]
    public class ShowMessageRequestParams {
        public MessageType type;
        public string message;
        public MessageActionItem[] actions;
    }
}
