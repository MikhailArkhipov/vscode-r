﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Common.Core;
using Microsoft.Common.Core.Services;
using Microsoft.Languages.Core.Text;
using Microsoft.Languages.Editor.Completions;
using Microsoft.Languages.Editor.Text;
using Microsoft.R.Editor;
using Microsoft.R.Editor.Document;
using Microsoft.R.Editor.Functions;
using Microsoft.R.Editor.QuickInfo;
using Microsoft.R.Editor.Signatures;
using Microsoft.R.LanguageServer.Extensions;

namespace Microsoft.R.LanguageServer.Completions {
    internal sealed class SignatureManager {
        private readonly IServiceContainer _services;
        private readonly RFunctionSignatureEngine _signatureEngine;

        public SignatureManager(IServiceContainer services) {
            _services = services;
            _signatureEngine = new RFunctionSignatureEngine(services);
        }

        public async Task<SignatureHelp> GetSignatureHelpAsync(IRIntellisenseContext context) {
            context.EditorBuffer.GetEditorDocument<IREditorDocument>().EditorTree.EnsureTreeReady();

            var tcs = new TaskCompletionSource<SignatureHelp>();
            var sigs = _signatureEngine.GetSignaturesAsync(context, e => {
                using (context.AstReadLock()) {
                    tcs.TrySetResult(ToSignatureHelp(e.ToList(), context));
                }
            });
            if (sigs != null) {
                using (context.AstReadLock()) {
                    return ToSignatureHelp(sigs.ToList(), context);
                }
            }
            return await tcs.Task;
        }

        public Task<Hover> GetHoverAsync(IRIntellisenseContext context, CancellationToken ct) {
            var tcs = new TaskCompletionSource<Hover>();
            using (context.AstReadLock()) {
                var infos = _signatureEngine.GetQuickInfosAsync(context, e => {
                    var r = !ct.IsCancellationRequested ? ToHover(e.ToList(), context.EditorBuffer) : null;
                    tcs.TrySetResult(r);
                });
                return infos != null ? Task.FromResult(ToHover(infos.ToList(), context.EditorBuffer)) : tcs.Task;
            }
        }

        private SignatureHelp ToSignatureHelp(IReadOnlyList<IRFunctionSignatureHelp> signatures, IRIntellisenseContext context) {
            if (signatures == null) {
                return null;
            }
            var sigInfos = signatures.Select(s => new SignatureInformation {
                label = s.Content,
                documentation = new MarkupContent {
                    kind = "plaintext",
                    value = s.Documentation.RemoveLineBreaks()
                },
                parameters = s.Parameters.Select(p => new ParameterInformation {
                    label = ToLabelRange(s.Content, p.Locus.Start, p.Locus.End),
                    documentation = new MarkupContent {
                        kind = "plaintext",
                        value = p.Documentation.RemoveLineBreaks()
                    }
                }).ToArray()
            }).ToArray();

            return new SignatureHelp {
                signatures = sigInfos,
                activeParameter = sigInfos.Length > 0 ? ComputeActiveParameter(context, signatures.First().SignatureInfo) : 0
            };
        }

        private static int[] ToLabelRange(string signature, int start, int end) {
            // Locus points to range that includes leading whitespace and trailing comma.
            for (; start < end && char.IsWhiteSpace(signature[start]); start++) {
            }
            for (; end > start && char.IsWhiteSpace(signature[end]) || end == ','; end--) {
            }
            return new[] { start, end };
        }

        private static Hover ToHover(IReadOnlyList<IRFunctionQuickInfo> e, IEditorBuffer buffer) {
            if (e == null || e.Count == 0) {
                return new Hover();
            }
            var info = e[0];
            var content = info.Content?.FirstOrDefault();
            if (!string.IsNullOrEmpty(content)) {
                var snapshot = buffer.CurrentSnapshot;
                var start = info.ApplicableToRange.GetStartPoint(snapshot);
                var end = info.ApplicableToRange.GetEndPoint(snapshot);
                return new Hover {
                    contents = new MarkupContent {
                        kind = "plaintext",
                        value = content
                    },
                    range = buffer.ToLineRange(start, end)
                };
            }
            return new Hover();
        }

        private int ComputeActiveParameter(IRIntellisenseContext context, ISignatureInfo signatureInfo) {
            var settings = _services.GetService<IREditorSettings>();
            var parameterIndex = signatureInfo.ComputeCurrentParameter(context.EditorBuffer.CurrentSnapshot, context.AstRoot, context.Position, settings);
            if (parameterIndex < signatureInfo.Arguments.Count) {
                return parameterIndex;
            }
            return signatureInfo.Arguments.Count > 0 ? signatureInfo.Arguments.Count - 1 : 0;
        }
    }
}
