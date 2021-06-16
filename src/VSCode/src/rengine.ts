// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import { LanguageClient } from 'vscode-languageclient/node';

export class REngine {
    private client: LanguageClient;

    constructor(client: LanguageClient) {
        this.client = client;
    }

    public getInterpreterPath(): Promise<string> {
        return this.client.sendRequest<string>('r/getInterpreterPath');
    }

    public execute(code: string): Promise<string> {
        return this.client.sendRequest<string>('r/execute', { code });
    }

    public async interrupt(): Promise<void> {
        await this.client.sendRequest('r/interrupt');
    }

    public async reset(): Promise<void> {
        await this.client.sendRequest('r/reset');
    }

    public async source(filePath: string): Promise<void> {
        await this.client.sendRequest('r/source', filePath);
    }
}
