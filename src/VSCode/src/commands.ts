// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import { commands, Disposable, Uri, ViewColumn, window } from 'vscode';

import { getFilePath, getSelectedText } from './editor';
import { PlotView } from './plotView';
import { REngine } from './rengine';
import { ReplTerminal } from './replTerminal';

// Must match package.json declarations
// tslint:disable-next-line:no-namespace
export namespace CommandNames {
    export const Plot = 'r.plot';
    export const Interrupt = 'r.interrupt';
    export const Reset = 'r.reset';
    export const SourceFile = 'r.source';
    export const OpenTerminal = 'r.openTerminal';
    export const ExecuteInTerminal = 'r.executeInTerminal';
    export const SourceFileToTerminal = 'r.sourceToTerminal';
}

export class Commands {
    private repl: ReplTerminal;

    constructor(private readonly r: REngine) {}

    public activateCommandsProvider(): Disposable[] {
        const disposables: Disposable[] = [];
        disposables.push(commands.registerCommand(CommandNames.Plot, () => this.plot()));
        disposables.push(commands.registerCommand(CommandNames.Interrupt, () => this.r.interrupt()));
        disposables.push(commands.registerCommand(CommandNames.Reset, () => this.r.reset()));
        disposables.push(commands.registerCommand(CommandNames.OpenTerminal, () => this.openTerminal()));
        disposables.push(commands.registerCommand(CommandNames.ExecuteInTerminal, () => this.executeInTerminal()));
        disposables.push(commands.registerCommand(CommandNames.SourceFileToTerminal, () => this.sourceToTerminal()));
        return disposables;
    }

    private async plot() {
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
            await this.sendTextToTerminal(`source(${p})`);
        }
    }

    private async executeInTerminal() {
        const code = getSelectedText();
        if (code.length > 0) {
            await this.sendTextToTerminal(code);
        }
        await this.moveCaretDown();
    }

    private async sendTextToTerminal(text: string) {
        const repl = await this.getRepl();
        repl.sendText(text);
    }

    private async openTerminal() {
        await this.createTerminal();
        this.repl.show();
    }

    private async getRepl() {
        await this.createTerminal();
        this.repl.show();
        return this.repl;
    }

    private async createTerminal() {
        if (this.repl !== undefined && this.repl !== null) {
            return;
        }
        const interpreterPath = await this.r.getInterpreterPath();
        this.repl = new ReplTerminal(interpreterPath);
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
