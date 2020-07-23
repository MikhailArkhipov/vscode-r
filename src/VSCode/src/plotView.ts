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
    private disposables: Disposable[] = [];

    public static createOrShow() {
        const viewColumn = PlotView.getViewColumn();
        // If we already have a panel, show it.
        if (PlotView.currentPanel) {
            PlotView.currentPanel.panel.reveal(viewColumn);
            return;
        }

        // Otherwise, create a new panel.
        const panel = window.createWebviewPanel(PlotView.viewType, 'Plot', {
            viewColumn,
            preserveFocus: true,
        });

        PlotView.currentPanel = new PlotView(panel);
    }

    public static revive(panel: WebviewPanel) {
        PlotView.currentPanel = new PlotView(panel);
    }

    private constructor(private readonly panel: WebviewPanel) {
        // Listen for when the panel is disposed
        // This happens when the user closes the panel or when the panel is closed programatically
        this.panel.onDidDispose(() => this.dispose(), null, this.disposables);
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

    private static getViewColumn(): ViewColumn {
        const column = window.activeTextEditor ? window.activeTextEditor.viewColumn : ViewColumn.One;
        switch (column) {
            case ViewColumn.One:
                return ViewColumn.Two;
            case ViewColumn.Two:
                return ViewColumn.Three;
            case ViewColumn.Three:
                return ViewColumn.Four;
            case ViewColumn.Four:
                return ViewColumn.Five;
        }
        return ViewColumn.One;
    }
}
