﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import { Terminal, window } from 'vscode';

export class ReplTerminal {
    private terminal: Terminal;
    private interpreterPath: string;

    constructor(ip: string) {
        this.interpreterPath = ip;

        window.onDidCloseTerminal((closedTerminal: Terminal) => {
            if (this.terminal === closedTerminal) {
                this.terminal = undefined;
            }
        });
    }

    public show() {
        if (this.terminal === undefined) {
            this.terminal = window.createTerminal('R', this.interpreterPath);
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
}
