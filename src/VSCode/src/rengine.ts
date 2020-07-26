// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
'use strict';

import { LanguageClient } from 'vscode-languageclient';

export class REngine {
    private client: LanguageClient;

    constructor(client: LanguageClient) {
        this.client = client;
    }

    public getInterpreterPath(): Thenable<string> {
        return this.client.sendRequest<string>('r/getInterpreterPath');
    }

    public execute(code: string): Thenable<string> {
        return this.client.sendRequest<string>('r/execute', { code });
    }

    public async interrupt() {
        await this.client.sendRequest('r/interrupt');
    }

    public async reset() {
        await this.client.sendRequest('r/reset');
    }

    public async source(filePath: string) {
        await this.client.sendRequest('r/source', filePath);
    }
}
