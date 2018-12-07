// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
"use strict";

import {commands, Disposable, Uri, window, ViewColumn} from "vscode";
import * as editor from "./editor";
import { ReplTerminal } from "./replTerminal";

// Must match package.json declarations
// tslint:disable-next-line:no-namespace
export namespace CommandNames {
    export const Interrupt = "r.interrupt";
    export const Reset = "r.reset";
    export const SourceFile = "r.source";
    export const OpenTerminal = "r.openTerminal";
    export const ExecuteInTerminal = "r.executeInTerminal";
    export const SourceFileToTerminal = "r.sourceToTerminal";
}

export class Commands {
    private r: IREngine;
    private repl: IReplTerminal;

    constructor(r: IREngine) {
        this.r = r;
    }

    public activateCommandsProvider(): Disposable[] {
        const disposables: Disposable[] = [];
        disposables.push(commands.registerCommand(CommandNames.Interrupt, () => this.r.interrupt()));
        disposables.push(commands.registerCommand(CommandNames.Reset, () => this.r.reset()));
        disposables.push(commands.registerCommand(CommandNames.OpenTerminal, () => this.openTerminal()));
        disposables.push(commands.registerCommand(CommandNames.ExecuteInTerminal, () => this.executeInTerminal()));
        disposables.push(commands.registerCommand(CommandNames.SourceFileToTerminal, () => this.sourceToTerminal()));
        return disposables;
    }

    private async sourceToTerminal(fileUri?: Uri) {
        const filePath = editor.getFilePath(fileUri);
        if (filePath.length > 0) {
            await this.sendTextToTerminal(`source("${filePath.replace(/\\/g, '/')}")`);
        }
    }

    private async executeInTerminal() {
        const code = editor.getSelectedText();
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
        await window.showTextDocument(window.activeTextEditor.document, ViewColumn.Active, true);
        const selectionEmpty = window.activeTextEditor.selection.isEmpty;
        if (selectionEmpty) {
            await commands.executeCommand("cursorMove",
                {
                    by: "line",
                    to: "down",
                });
        }
    }
}
