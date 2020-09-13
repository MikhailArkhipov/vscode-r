// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import * as fs from 'fs';
import { commands, Disposable, Uri, window, workspace } from 'vscode';
import { OutputChannelName } from './constants';

import { getFilePath, getSelectedText } from './editor';
import { PlotView } from './plotView';
import { REngine } from './rengine';
import { ReplTerminal } from './replTerminal';

// Must match package.json declarations
// tslint:disable-next-line:no-namespace
export namespace CommandNames {
    export const Execute = 'r.execute';
    export const Interrupt = 'r.interrupt';
    export const Reset = 'r.reset';
    export const SourceFile = 'r.source';
    export const OpenTerminal = 'r.openTerminal';
    export const ExecuteInTerminal = 'r.executeInTerminal';
    export const SourceFileToTerminal = 'r.sourceToTerminal';
}

export class Commands {
    private readonly disposables: Disposable[] = [];
    private repl: ReplTerminal;

    constructor(private readonly r: REngine) {}

    public activateCommandsProvider(): Disposable[] {
        this.disposables.push(commands.registerCommand(CommandNames.Execute, () => this.execute()));
        this.disposables.push(commands.registerCommand(CommandNames.Interrupt, () => this.r.interrupt()));
        this.disposables.push(commands.registerCommand(CommandNames.Reset, () => this.r.reset()));
        this.disposables.push(commands.registerCommand(CommandNames.OpenTerminal, () => this.openTerminal()));
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
            await this.sendTextToTerminal(code);
            await this.r.execute(code);
        }
    }

    private async executeInTerminal() {
        const code = getSelectedText();
        if (code.length > 0) {
            await this.sendTextToTerminal(code);
            await this.r.execute(code);
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

        let terminalPath = workspace.getConfiguration('r').get<string>('terminalPath');
        if (terminalPath?.length === 0) {
            terminalPath = undefined;
        }
        if (terminalPath && !fs.existsSync(terminalPath)) {
            this.getOutput().appendLine(
                `r.terminalPath ('${terminalPath}') does not exist. Trying 'r.interpreterPath' instead.`
            );
        }

        let interpreterPath = workspace.getConfiguration('r').get<string>('interpreterPath');
        if (interpreterPath?.length === 0) {
            interpreterPath = undefined;
        }

        if (interpreterPath && !fs.existsSync(interpreterPath)) {
            this.getOutput().appendLine(
                `r.interpreterPath ('${interpreterPath}') does not exist. Trying to discover R interpreter automatically.`
            );
        }

        interpreterPath = interpreterPath ?? (await this.r.getInterpreterPath());
        this.repl = new ReplTerminal(interpreterPath, terminalPath);
        this.disposables.push(this.repl);
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

    private getOutput() {
        return window.createOutputChannel(OutputChannelName);
    }
}
