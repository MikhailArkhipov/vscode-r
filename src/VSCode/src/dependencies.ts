// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
"use strict";

import { exec, spawn } from "child_process";
import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";

import { createDeferred } from "./deferred";
import * as os from "./os";

import getenv = require('getenv');
import open = require('open');

export async function checkDependencies(context: vscode.ExtensionContext): Promise<boolean> {
    if (!await checkDotNet()) {
        return false;
    }
    
    ensureHostExecutable(context);
    
    if (!IsLibZipInstalled()) {
        if (!isBrewInstalled()) {
            await installBrew();
        }
        await installLibZip();
    }
    return true;
}

 async function getR(r: IREngine): Promise<string> {
    const interpreterPath = await r.getInterpreterPath();
    if (interpreterPath === undefined || interpreterPath === null) {
        if (await vscode.window.showErrorMessage("Unable to find R interpreter. Would you like to install R now?",
            "Yes", "No") === "Yes") {
            InstallR();
            vscode.window.showWarningMessage("Please restart VS Code after R installation is complete.");
        }
        return null;
    }
    return interpreterPath;
}

async function checkDotNet(): Promise<boolean> {
    getOutput().append("Checking for .NET Core... ");
    if (!IsDotNetInstalled()) {
        if (await vscode.window.showErrorMessage("R Tools require .NET Core Runtime. Would you like to install it now?",
            "Yes", "No") === "Yes") {
            InstallDotNet();
            await vscode.window.showWarningMessage("Please restart VS Code after .NET Runtime installation is complete.");
        }
        return false;
    }
    getOutput().appendLine("OK");
    return true;
}

function ensureHostExecutable(context: vscode.ExtensionContext): void {
    if(!os.IsWindows()) {
        const osDir = os.IsMac() ? "Mac" : "Linux";
        const hostBinPath = path.join(context.extensionPath, "ls", "Host", osDir, "Microsoft.R.Host");
        fs.chmodSync(hostBinPath, "764");
    }
}

function IsDotNetInstalled(): boolean {
    let dir: string;

    if (os.IsWindows()) {
        const drive = getenv("SystemDrive");
        dir = drive + "\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App";
    } else if(os.IsMac()) {
        dir = "/usr/local/share/dotnet/shared/Microsoft.NETCore.App";
    } else {
        dir = "/usr/share/dotnet/shared/Microsoft.NETCore.App";
    }
    return fs.existsSync(dir);
}

function InstallDotNet(): void {
    open("https://www.microsoft.com/net/download/core#/runtime");
}

function InstallR(): void {
    let url: string;
    if (os.IsWindows()) {
        url = "https://cran.r-project.org/bin/windows/base/";
    } else if (os.IsMac()) {
        url = "https://cran.r-project.org/bin/macosx/";
    } else {
        url = "https://cran.r-project.org/bin/linux/";
    }
    open(url);
}

function IsLibZipInstalled(): boolean {
    if (!os.IsMac()) {
        return true;
    }
    return fs.existsSync("/usr/local/opt/libzip/lib/libzip.5.dylib");
}

function isBrewInstalled(): boolean {
    if (!os.IsMac()) {
        return true;
    }
    return fs.existsSync("/usr/local/bin/brew");
}

async function installBrew(): Promise<void> {
    getOutput().append("Installing Homebrew (required to install libzip library)... ");
    await execute("/usr/bin/ruby", ["-e", "\"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\""]);
    getOutput().appendLine("OK");
}

async function installLibZip() {
    getOutput().append("Installing libzip (required for the R process executable)... ");
    await execute("brew", ["install", "libzip"]);
    getOutput().appendLine("OK");
}

function execute(command: string, args: string[]): Promise<void> {
    const deferred = createDeferred<void>();
    const proc = spawn(command, args);

    proc.stdout.on("data", (data: Buffer) => getOutput().append(data.toString()));
    proc.stderr.on("data", (data: Buffer) => getOutput().append(data.toString()));
 
    proc.once("exit", (code, signal) => deferred.resolve())
    proc.once("close", (code, signal) => deferred.resolve())
    proc.once("error", (code, signal) => {
        getOutput().appendLine(signal);
        deferred.reject(code);
    });
    return deferred.promise;
}

let outputChannel: vscode.OutputChannel;
function getOutput(): vscode.OutputChannel {
    if(!outputChannel) {
        outputChannel = vscode.window.createOutputChannel("R Tools Startup Log");
    }
    return outputChannel;
}