// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.Common.Core.Services;
using Microsoft.Languages.Core.Formatting;
using Microsoft.Languages.Editor.Settings;
using Microsoft.R.Core.Formatting;
using Microsoft.R.Editor;
using Microsoft.R.Editor.Validation.Lint;
using Microsoft.R.LanguageServer.Settings;

namespace Microsoft.R.LanguageServer.Server {
    internal sealed class SettingsManager : ISettingsManager {
        private readonly REngineSettings _engineSettings = new REngineSettings();
        private readonly REditorSettings _editorSettings = new REditorSettings();

        public SettingsManager(IServiceManager serviceManager) {
            serviceManager
                .AddService(_engineSettings)
                .AddService(_editorSettings);
        }

        public void Dispose() => _editorSettings.Dispose();

        public void UpdateSettings(LanguageServerSettings vscodeSettings) {

            var e = vscodeSettings.Editor;
            _editorSettings.FormatScope = e.FormatScope;
            _editorSettings.AutoFormat = e.FormatOnType;
            _editorSettings.FormatOptions.IndentSize = e.TabSize;
            _editorSettings.FormatOptions.TabSize = e.TabSize;
            _editorSettings.FormatOptions.SpaceAfterComma = e.SpaceAfterComma;
            _editorSettings.FormatOptions.SpaceAfterKeyword = e.SpaceAfterKeyword;
            _editorSettings.FormatOptions.SpaceBeforeCurly = e.SpaceBeforeCurly;
            _editorSettings.FormatOptions.SpacesAroundEquals = e.SpacesAroundEquals;
            _editorSettings.FormatOptions.BreakMultipleStatements = e.BreakMultipleStatements;
            _editorSettings.LintOptions = vscodeSettings.Linting;
            _editorSettings.RaiseChanged();

            _engineSettings.InterpreterIndex = vscodeSettings.InterpreterIndex;
            _engineSettings.InterpreterPath = vscodeSettings.InterpreterPath;

            SettingsChanged?.Invoke(this, EventArgs.Empty);
        }

        public event EventHandler SettingsChanged;

#pragma warning disable 67
        private sealed class REngineSettings : IREngineSettings {
            public int InterpreterIndex { get; set; }
            public string InterpreterPath { get; set; }
        }

        private sealed class REditorSettings : IREditorSettings {
            private readonly IEditorSettingsStorage _storage;

            public REditorSettings() {
                _storage = new EditorSettingsStorage();
                LintOptions = new LintOptions(() => _storage);
            }

            public void Dispose() => _storage.Dispose();

            public event EventHandler<EventArgs> SettingsChanged;
            public bool AutoFormat { get; set; } = true;
            public bool CompletionEnabled { get; } = true;
            public int IndentSize { get; set; } = 2;
            public IndentType IndentType { get; } = IndentType.Spaces;
            public int TabSize { get; set; } = 2;
            public IndentStyle IndentStyle { get; } = IndentStyle.Smart;
            public bool SyntaxCheckEnabled { get; } = true;
            public bool SignatureHelpEnabled { get; } = true;
            public bool InsertMatchingBraces { get; } = true;
            public bool FormatOnPaste { get; set; }
            public bool FormatScope { get; set; }
            public bool CommitOnSpace { get; set; } = false;
            public bool CommitOnEnter { get; set; } = true;
            public bool ShowCompletionOnFirstChar { get; set; } = true;
            public bool ShowCompletionOnTab { get; set; } = true;
            public bool SyntaxCheckInRepl { get; set; }
            public bool PartialArgumentNameMatch { get; set; }
            public bool EnableOutlining { get; set; }
            public bool SmartIndentByArgument { get; set; } = true;
            public RFormatOptions FormatOptions { get; set; } = new RFormatOptions();
            public ILintOptions LintOptions { get; set; }

            public void RaiseChanged() => SettingsChanged?.Invoke(this, EventArgs.Empty);
        }
     }
}
