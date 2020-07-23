// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';
import * as path from 'path';
import { Uri, ViewColumn, WebviewPanel, window } from 'vscode';
import { Disposable } from 'vscode';

import { createDeferred } from './deferred';

export class PlotView implements Disposable {
    public static currentPanel: PlotView | undefined;
    public static readonly viewType = 'plot';

    private uri: Uri;
    private buffer = '';
    private disposables: Disposable[] = [];

    public static createOrShow(extensionPath: string) {
        // If we already have a panel, show it.
        if (PlotView.currentPanel) {
            PlotView.currentPanel.panel.reveal(ViewColumn.Two);
            return;
        }

        // Otherwise, create a new panel.
        const panel = window.createWebviewPanel(PlotView.viewType, 'Plot', ViewColumn.Two, {
            // Enable javascript in the webview
            enableScripts: true,
            // And restrict the webview to only loading content from our extension's `media` directory.
            localResourceRoots: [Uri.file(path.join(extensionPath, 'media'))],
        });

        PlotView.currentPanel = new PlotView(panel, extensionPath);
    }

    public static revive(panel: WebviewPanel, extensionPath: string) {
        PlotView.currentPanel = new PlotView(panel, extensionPath);
    }

    private constructor(private readonly panel: WebviewPanel, private readonly extensionPath: string) {
        // Set the webview's initial html content
        this.clear();

        // Listen for when the panel is disposed
        // This happens when the user closes the panel or when the panel is closed programatically
        this.panel.onDidDispose(() => this.dispose(), null, this.disposables);

        // Update the content based on view changes
        // this.panel.onDidChangeViewState(
        //     (e) => {
        //         if (this.panel.visible) {
        //             this.update();
        //         }
        //     },
        //     null,
        //     this.disposables
        // );
    }

    public dispose() {
        PlotView.currentPanel = undefined;
        // Clean up our resources
        this.panel.dispose();
        while (this.disposables.length) {
            const x = this.disposables.pop();
            if (x) {
                x.dispose();
            }
        }
    }

    public clear() {
        if (PlotView.currentPanel) {
            PlotView.currentPanel.panel.webview.html = this.generateResultsView('');
        }
    }

    public append(result: string): void {
        if (result.startsWith('$$IMAGE ')) {
            const base64 = result.substring(8, result.length);
            // tslint:disable-next-line:max-line-length
            const output = `<img src='data:image/gif;base64, ${base64}' style='display:block; margin: 8,0,8,0; text-align: center; width: 90%' />`;
            PlotView.currentPanel.panel.webview.html = this.generateResultsView(output);
        }
    }

    private generateResultsView(content: string): string {
        const htmlContent = `
                <!DOCTYPE html>
                <head>
                    <style type="text/css">
                        html, body { height:100%; width:100%; }
                    </style>
                </head>
                <body>${content}</body>
            </html>`;

        return htmlContent;
    }
}
