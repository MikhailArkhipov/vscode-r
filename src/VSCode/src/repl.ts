// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import * as fs from 'fs';
import { REngine } from './rengine';
import { Terminal, window, workspace } from 'vscode';
import { Disposable } from 'vscode-languageclient/node';

export class Repl implements Disposable {
    private readonly disposables: Disposable[] = [];
    private readonly terminals: Terminal[] = [];
    public activeTerminal: Terminal | undefined;

    constructor(private readonly r: REngine) {
        this.disposables.push(
            window.onDidCloseTerminal((t: Terminal) => {
                if (t.name !== 'R') {
                    return;
                }
                if (this.activeTerminal === t) {
                    this.activeTerminal = undefined;
                }
                const i = this.terminals.indexOf(t);
                if (i >= 0) {
                    this.terminals.splice(i, 1);
                }
            })
        );
        this.disposables.push(
            window.onDidChangeActiveTerminal((t: Terminal) => {
                this.activeTerminal = t.name === 'R' ? t : undefined;
            })
        );
    }

    public dispose() {
        while (this.disposables.length) {
            this.disposables.pop()?.dispose();
        }
    }

    public async create(): Promise<void> {
        this.activeTerminal = window.createTerminal('R', await this.getRTerminalPath());
        this.terminals.push(this.activeTerminal);
        return this.show();
    }

    public async show(): Promise<void> {
        if (this.activeTerminal === undefined) {
            if (this.terminals.length === 0) {
                await this.create();
            } else {
                this.activeTerminal = this.terminals[this.terminals.length - 1];
            }
        }
        this.activeTerminal.show(true);
    }

    public async sendText(text: string): Promise<void> {
        await this.show();
        this.activeTerminal.sendText(text);
    }

    private async getRTerminalPath(): Promise<string> {
        let terminalPath = workspace.getConfiguration('r').get<string>('terminalPath');
        if (terminalPath?.length === 0) {
            terminalPath = undefined;
        }
        if (terminalPath && fs.existsSync(terminalPath)) {
            return terminalPath;
        }

        return await this.r.getInterpreterPath();
    }
}
