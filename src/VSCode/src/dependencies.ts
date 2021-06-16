// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import * as fs from 'fs';
import * as path from 'path';
import { ExtensionContext, OutputChannel, window, workspace } from 'vscode';

import { IsMac, IsWindows } from './os';

import getenv = require('getenv');
import open = require('open');

export function ensureHostExecutable(context: ExtensionContext): void {
    if (IsWindows()) {
        return;
    }
    const hostDirs: string[] = [];
    if (IsMac()) {
        hostDirs.push(path.join('Mac', 'arm64'));
        hostDirs.push(path.join('Mac', 'x64'));
    } else {
        hostDirs.push('Linux');
    }
    while (hostDirs.length) {
        const hostBinPath = path.join(context.extensionPath, 'ls', 'Host', hostDirs.pop(), 'Microsoft.R.Host');
        fs.chmodSync(hostBinPath, '764');
    }
}

let outputChannel: OutputChannel;
function getOutput(): OutputChannel {
    if (!outputChannel) {
        outputChannel = window.createOutputChannel('R Tools Startup Log');
    }
    return outputChannel;
}
