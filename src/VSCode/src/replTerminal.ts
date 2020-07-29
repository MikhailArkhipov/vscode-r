// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import { Terminal, window } from 'vscode';
import { Disposable } from 'vscode-languageclient';

export class ReplTerminal implements Disposable {
    private readonly disposables: Disposable[] = [];
    private terminal: Terminal;

    constructor(private readonly interpreterPath: string, private readonly terminalPath: string | undefined) {
        this.disposables.push(
            window.onDidCloseTerminal((closedTerminal: Terminal) => {
                if (this.terminal === closedTerminal) {
                    this.terminal = undefined;
                }
            })
        );
    }

    public dispose() {
        while (this.disposables.length) {
            this.disposables.pop()?.dispose();
        }
    }

    public show() {
        if (this.terminal === undefined) {
            this.terminal = window.createTerminal('R', this.getRTerminalPath());
        }
        this.terminal.show(true);
    }

    public close() {
        if (this.terminal !== undefined) {
            this.terminal.dispose();
            this.terminal = undefined;
        }
    }

    public sendText(text: string) {
        this.show();
        this.terminal.sendText(text);
    }

    private getRTerminalPath(): string {
        return this.terminalPath ?? this.interpreterPath;
    }
}
