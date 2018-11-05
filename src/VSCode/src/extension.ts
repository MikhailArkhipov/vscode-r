// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
"use strict";

// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from "vscode";
import * as languageClient from "vscode-languageclient";
import { Commands } from "./commands";
import { RLanguage } from "./constants";
import * as deps from "./dependencies";
import { REngine } from "./rengine";
import { bool } from "getenv";

let client: languageClient.LanguageClient;
let rEngine: IREngine;
let commands: Commands;

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export async function activate(context: vscode.ExtensionContext) {
    const config = vscode.workspace.getConfiguration("r");
    const check = config.get<boolean>("dependencyChecks");
    if (check && !await deps.checkDotNet()) {
        return;
    }

    // Make sure Microsoft.R.Host is marked as executable on Mac/Linux.
    deps.ensureExecutableHost(context);

    console.log("Activating R Language Server...");
    await activateLanguageServer(context);
    console.log("Startup completed.");
}

export async function activateLanguageServer(context: vscode.ExtensionContext) {
    // The server is implemented in C#
    const commandOptions = { stdio: "pipe" };
    const serverModule = context.extensionPath + "/ls/Microsoft.R.LanguageServer.dll";

    // If the extension is launched in debug mode then the debug server options are used
    // Otherwise the run options are used
    const serverOptions: languageClient.ServerOptions = {
        debug: { command: "dotnet", args: [serverModule, "--debug"], options: commandOptions },
        run: { command: "dotnet", args: [serverModule], options: commandOptions },
    };

    // Options to control the language client
    const clientOptions: languageClient.LanguageClientOptions = {
        // Register the server for R documents
        documentSelector: [{language: RLanguage.language, scheme: 'file'}],
        synchronize: {
            configurationSection: RLanguage.language,
        },
    };

    // Create the language client and start the client.
    client = new languageClient.LanguageClient(RLanguage.language, "R Tools", serverOptions, clientOptions);
    context.subscriptions.push(client.start());

    await client.onReady();

    rEngine = new REngine(client);
    commands = new Commands(rEngine);
    context.subscriptions.push(...commands.activateCommandsProvider());
}

// this method is called when your extension is deactivated
export async function deactivate() {
    if (client !== undefined || client !== null) {
        client.stop();
    }
}
