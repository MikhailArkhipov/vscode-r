// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import { commands, Disposable, OutputChannel, Uri, window } from 'vscode';
import { OutputChannelName } from './constants';

import { getFilePath, getSelectedText } from './editor';
import { PlotView } from './plotView';
import { REngine } from './rengine';
import { Repl } from './repl';

// Must match package.json declarations
// tslint:disable-next-line:no-namespace
export namespace CommandNames {
    export const Execute = 'r.execute';
    export const Interrupt = 'r.interrupt';
    export const Reset = 'r.reset';
    export const SourceFile = 'r.source';
    export const CreateTerminal = 'r.createTerminal';
    export const OpenTerminal = 'r.openTerminal';
    export const ExecuteInTerminal = 'r.executeInTerminal';
    export const SourceFileToTerminal = 'r.sourceToTerminal';
}

export class Commands {
    private readonly disposables: Disposable[] = [];
    private readonly repl: Repl;
    private readonly output: OutputChannel;

    constructor(private readonly r: REngine) {
        this.disposables.push(this.output);
        this.repl = new Repl(r);
    }

    public activateCommandsProvider(): Disposable[] {
        this.disposables.push(commands.registerCommand(CommandNames.Execute, () => this.execute()));
        this.disposables.push(commands.registerCommand(CommandNames.Interrupt, () => this.r.interrupt()));
        this.disposables.push(commands.registerCommand(CommandNames.Reset, () => this.r.reset()));
        this.disposables.push(commands.registerCommand(CommandNames.CreateTerminal, () => this.repl.create()));
        this.disposables.push(commands.registerCommand(CommandNames.OpenTerminal, () => this.repl.show()));
        this.disposables.push(commands.registerCommand(CommandNames.ExecuteInTerminal, () => this.executeInTerminal()));
        this.disposables.push(
            commands.registerCommand(CommandNames.SourceFileToTerminal, () => this.sourceToTerminal())
        );
        return this.disposables;
    }

    private async execute() {
        const code = getSelectedText();
        if (code.length > 0) {
            const result = await this.r.execute(code);
            PlotView.createOrShow();
            PlotView.currentPanel.append(result);
        }
        await this.moveCaretDown();
    }

    private async sourceToTerminal(fileUri?: Uri) {
        const filePath = getFilePath(fileUri);
        if (filePath.length > 0) {
            let p = filePath.replace(/\\/g, '/');
            if (p.length > 0 && p[0] !== '"') {
                p = p = `"${p}"`;
            }
            const code = `source(${p})`;
            await this.repl.sendText(code);
            await this.r.execute(code);
        }
    }

    private async executeInTerminal() {
        const code = getSelectedText();
        if (code.length > 0) {
            await this.repl.sendText(code);
            await this.r.execute(code);
        }
        await this.moveCaretDown();
    }

    private async moveCaretDown() {
        // Take focus back to the editor
        await window.showTextDocument(window.activeTextEditor.document, window.activeTextEditor.viewColumn, false);
        const selectionEmpty = window.activeTextEditor.selection.isEmpty;
        if (selectionEmpty) {
            await commands.executeCommand('cursorMove', {
                by: 'line',
                to: 'down',
            });
        }
    }
}
