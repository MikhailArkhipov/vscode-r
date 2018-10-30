// Python Tools for Visual Studio
// Copyright(c) Microsoft Corporation
// All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License); you may not use
// this file except in compliance with the License. You may obtain a copy of the
// License at http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS
// OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY
// IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
//
// See the Apache Version 2.0 License for specific language governing
// permissions and limitations under the License.

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
        public TextDocumentIdentifier textDocument;
        public TextDocumentSaveReason reason;
    }

    [Serializable]
    public struct DidSaveTextDocumentParams {
        public TextDocumentIdentifier textDocument;
        public string content;
    }

    [Serializable]
    public struct DidCloseTextDocumentParams {
        public TextDocumentIdentifier textDocument;
    }

    [Serializable]
    public struct TextDocumentPositionParams {
        public TextDocumentIdentifier textDocument;
        public Position position;
    }

    [Serializable]
    public struct CompletionParams {
        public TextDocumentIdentifier textDocument;
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
        public TextDocumentIdentifier textDocument;
        public Position position;
        public ReferenceContext? context;
    }

    public struct ReferenceContext {
        public bool includeDeclaration;
    }

    [Serializable]
    public struct DocumentSymbolParams {
        public TextDocumentIdentifier textDocument;
    }

    [Serializable]
    public struct CodeActionParams {
        public TextDocumentIdentifier textDocument;
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
        public TextDocumentIdentifier textDocument;
    }

    [Serializable]
    public struct DocumentFormattingParams {
        public TextDocumentIdentifier textDocument;
        public FormattingOptions options;
    }

    [Serializable]
    public struct DocumentRangeFormattingParams {
        public TextDocumentIdentifier textDocument;
        public Range range;
        public FormattingOptions options;
    }

    [Serializable]
    public struct DocumentOnTypeFormattingParams {
        public TextDocumentIdentifier textDocument;
        public Position position;
        public string ch;
        public FormattingOptions options;
    }

    [Serializable]
    public struct RenameParams {
        public TextDocumentIdentifier textDocument;
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
