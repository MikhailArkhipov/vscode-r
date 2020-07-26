// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import * as fs from 'fs';
import * as path from 'path';
import { ExtensionContext, OutputChannel, window } from 'vscode';

import { IsMac, IsWindows } from './os';

import getenv = require('getenv');
import open = require('open');
import { REngine } from './rengine';

export async function checkDependencies(context: ExtensionContext): Promise<boolean> {
    if (!(await checkDotNet())) {
        return false;
    }

    ensureHostExecutable(context);
    return true;
}

async function checkDotNet(): Promise<boolean> {
    getOutput().append('Checking for .NET Core... ');
    if (!IsDotNetInstalled()) {
        if (
            (await window.showErrorMessage(
                'R Tools require .NET Core Runtime. Would you like to install it now?',
                'Yes',
                'No'
            )) === 'Yes'
        ) {
            InstallDotNet();
            await window.showWarningMessage('Please restart VS Code after .NET Runtime installation is complete.');
        }
        return false;
    }
    getOutput().appendLine('OK');
    return true;
}

export function ensureHostExecutable(context: ExtensionContext): void {
    if (!IsWindows()) {
        const osDir = IsMac() ? 'Mac' : 'Linux';
        const hostBinPath = path.join(context.extensionPath, 'ls', 'Host', osDir, 'Microsoft.R.Host');
        fs.chmodSync(hostBinPath, '764');
    }
}

function IsDotNetInstalled(): boolean {
    let dir: string;

    if (IsWindows()) {
        const drive = getenv('SystemDrive');
        dir = drive + '\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App';
    } else if (IsMac()) {
        dir = '/usr/local/share/dotnet/shared/Microsoft.NETCore.App';
    } else {
        dir = '/usr/share/dotnet/shared/Microsoft.NETCore.App';
    }
    return fs.existsSync(dir);
}

function InstallDotNet(): void {
    open('https://www.microsoft.com/net/download/core#/runtime');
}

let outputChannel: OutputChannel;
function getOutput(): OutputChannel {
    if (!outputChannel) {
        outputChannel = window.createOutputChannel('R Tools Startup Log');
    }
    return outputChannel;
}
