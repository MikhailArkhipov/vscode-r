// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.Languages.Editor.Text;
using Microsoft.R.Core.AST;
using Microsoft.R.Core.AST.Operators;
using Microsoft.R.Core.AST.Statements;
using Microsoft.R.Core.AST.Variables;
using Microsoft.R.Editor.Document;
using Microsoft.R.LanguageServer.Extensions;

namespace Microsoft.R.LanguageServer.Symbols {
    internal sealed class DocumentSymbolsProvider : IAstVisitor {
        private static readonly Guid _treeUserId = new Guid("5A8CE561-DC03-4CDA-8568-947DDB84F5FA");

        private sealed class Scope {
            public IAstNode Node { get; }
            public DocumentSymbol Symbol { get; }
            public List<DocumentSymbol> Children { get; } = new List<DocumentSymbol>();
            public Scope(IAstNode node, DocumentSymbol ds) {
                Node = node;
                Symbol = ds;
            }
        }

        private sealed class SearchParams {
            public Uri Uri { get; }
            public List<DocumentSymbol> Symbols { get; } = new List<DocumentSymbol>();
            public IEditorBufferSnapshot Snapshot { get; }
            public AstRoot Ast { get; }

            public Stack<Scope> Scopes { get; } = new Stack<Scope>();

            public SearchParams(Uri uri, IEditorBufferSnapshot snapshot, AstRoot ast) {
                Uri = uri;
                Snapshot = snapshot;
                Ast = ast;
            }

            public void AddSymbol(DocumentSymbol ds) {
                if (Scopes.Count > 0) {
                    Scopes.Peek().Children.Add(ds);
                } else {
                    Symbols.Add(ds);
                }
            }
        }

        public DocumentSymbol[] GetSymbols(IREditorDocument document, Uri uri) {
            var ast = document.EditorTree.AcquireReadLock(_treeUserId);
            try {
                var p = new SearchParams(uri, document.EditorBuffer.CurrentSnapshot, ast);
                ast.Accept(this, p);
                return p.Symbols.ToArray();
            } finally {
                document.EditorTree.ReleaseReadLock(_treeUserId);
            }
        }

        public bool Visit(IAstNode node, object parameter) {
            var p = (SearchParams)parameter;
            // Only accept x <-, x = , -> x

            if (node is not Variable v || v.Parent is not IOperator op) {
                return true;
            }

            switch (op.OperatorType) {
                case OperatorType.LeftAssign:
                case OperatorType.Equals:
                    if (op.LeftOperand != v) {
                        return true;
                    }
                    break;
                case OperatorType.RightAssign:
                    if (op.RightOperand != v) {
                        return true;
                    }
                    break;
                default:
                    return true;
            }

            // Locate function definition, if any. Function defines new scope.
            if (op.Parent?.Parent is IExpressionStatement es) {
                var fd = es.GetVariableOrFunctionDefinition(out var funcNameVar);
                if (fd != null && !string.IsNullOrEmpty(funcNameVar?.Name)) {
                    var ds = new DocumentSymbol {
                        name = v.Name,
                        kind = SymbolKind.Function,
                        range = node.ToLineRange(p.Snapshot),
                        selectionRange = funcNameVar.ToLineRange(p.Snapshot),
                        deprecated = false
                    };

                    p.AddSymbol(ds);
                    p.Scopes.Push(new Scope(es, ds));
                    return true;
                }
            }

            if (p.Scopes.Count == 0) {
                var kind = string.IsNullOrEmpty(p.Ast.IsInLibraryStatement(node.Start)) ? SymbolKind.Variable : SymbolKind.Package;
                p.AddSymbol(new DocumentSymbol {
                    name = v.Name,
                    kind = kind,
                    range = node.ToLineRange(p.Snapshot),
                    selectionRange = node.ToLineRange(p.Snapshot),
                    deprecated = false
                });
            }
            return true;
        }

        public void EndVisit(IAstNode node, object parameter) {
            var p = (SearchParams)parameter;
            if (p.Scopes.Count > 0 && p.Scopes.Peek().Node == node) {
                var s = p.Scopes.Pop();
                s.Symbol.children = s.Children.ToArray();
            }
        }
    }
}
