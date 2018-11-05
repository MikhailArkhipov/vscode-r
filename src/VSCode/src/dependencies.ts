// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
"use strict";

import * as fs from "fs";
import * as vscode from "vscode";
import * as os from "./os";
import * as path from "path";

var getenv = require('getenv');
var opn = require('opn');

export async function getR(r: IREngine): Promise<string> {
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

export async function checkDotNet(): Promise<boolean> {
    console.log("Checking for .NET Core...");
    if (!IsDotNetInstalled()) {
        if (await vscode.window.showErrorMessage("R Tools require .NET Core Runtime. Would you like to install it now?",
            "Yes", "No") === "Yes") {
            InstallDotNet();
            await vscode.window.showWarningMessage("Please restart VS Code after .NET Runtime installation is complete.");
        }
        return false;
    }
    return true;
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
    opn("https://www.microsoft.com/net/download/core#/runtime");
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
    opn(url);
}

export function ensureExecutableHost(context: vscode.ExtensionContext): void {
    if(!os.IsWindows()) {
        const osName = os.IsMac() ? "Mac" : "Linux";
        fs.chmod(path.join(context.extensionPath, "ls", "Host", osName, "Microsoft.R.Host"), "0764", (err) => { 
            console.log(`Unable to make Microsoft.R.Host executable. Error: ${err.message}`);
        }); // -rwxrw-r--
    }
}
