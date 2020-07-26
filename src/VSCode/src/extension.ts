// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import { ConfigurationTarget, ExtensionContext, WebviewPanel, window, workspace } from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient';

import { Commands } from './commands';
import { RLanguage } from './constants';
import { checkDependencies, ensureHostExecutable } from './dependencies';
import { PlotView } from './plotView';
import { REngine } from './rengine';

let client: LanguageClient;
let rEngine: REngine;
let commands: Commands;

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export async function activate(context: ExtensionContext) {
    // Associate RMD with markdown editor
    const files = workspace.getConfiguration('files');
    const associations = files.get<{ ext: string; editor: string }>('associations');
    associations['*.rmd'] = 'markdown';
    await files.update('associations', associations, ConfigurationTarget.Global);

    const config = workspace.getConfiguration('r');
    ensureHostExecutable(context);

    const check = config.get<boolean>('dependencyChecks');
    if (check && !(await checkDependencies(context))) {
        return;
    }

    await activateLanguageServer(context);
}

export async function activateLanguageServer(context: ExtensionContext) {
    // The server is implemented in C#
    const serverModule = context.extensionPath + '/ls/Microsoft.R.LanguageServer.dll';

    // If the extension is launched in debug mode then the debug server options are used
    // Otherwise the run options are used
    const serverOptions: ServerOptions = {
        transport: TransportKind.pipe,
        debug: { command: 'dotnet', args: [serverModule, '--debug'] },
        run: { command: 'dotnet', args: [serverModule] },
    };

    // Options to control the language client
    const clientOptions: LanguageClientOptions = {
        // Register the server for R documents
        documentSelector: [{ language: RLanguage.language, scheme: 'file' }],
        synchronize: {
            configurationSection: RLanguage.language,
        },
    };

    // Create the language client and start the client.
    client = new LanguageClient(RLanguage.language, 'R Tools', serverOptions, clientOptions);
    context.subscriptions.push(client.start());

    await client.onReady();

    rEngine = new REngine(client);

    window.registerWebviewPanelSerializer(PlotView.viewType, {
        async deserializeWebviewPanel(webviewPanel: WebviewPanel, state: any) {
            PlotView.revive(webviewPanel);
        },
    });

    commands = new Commands(rEngine);
    context.subscriptions.push(...commands.activateCommandsProvider());
}

// this method is called when your extension is deactivated
export async function deactivate() {
    if (client !== undefined || client !== null) {
        client.stop();
    }
}
