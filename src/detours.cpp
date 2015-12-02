/* ****************************************************************************
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
*
* This file is part of Microsoft R Host.
*
* Microsoft R Host is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* Microsoft R Host is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Microsoft R Host.  If not, see <http://www.gnu.org/licenses/>.
*
* ***************************************************************************/

#include "stdafx.h"
#include "host.h"

namespace detours {
    // Converts host response back to MessageBox codes
    int ToMessageBoxCodes(int result, UINT mbType) {
        if (mbType == MB_YESNO) {
            return result == -1 ? IDNO : IDYES;
        } else if (mbType == MB_YESNOCANCEL) {
            return result == 0 ? IDCANCEL : (result == -1 ? IDNO : IDYES);
        }
        return result == 0 ? IDCANCEL : IDOK;
    }

    // Displays host message box. Host provides title and the parent window.
    // Communication with the host is in UTF-8 and single-byte characters
    // are converted to UTF-8 JSON in the rhost::host::ShowMessageBox().
    int HostMessageBox(LPCSTR lpText, UINT uType) {
        UINT mbType = uType & 0x0F;
        if (mbType == MB_OKCANCEL) {
            return ToMessageBoxCodes(rhost::host::OkCancel(lpText), mbType);
        } else if (mbType == MB_YESNO) {
            return ToMessageBoxCodes(rhost::host::YesNo(lpText), mbType);
        } else if (mbType == MB_YESNOCANCEL) {
            return ToMessageBoxCodes(rhost::host::YesNoCancel(lpText), mbType);
        }
        rhost::host::ShowMessage(lpText);
        return IDOK;
    }

    // MessageBoxW
    decltype(MessageBoxW) *pMessageBoxW = NULL;
    int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
        char ch[1000];
        wcstombs(ch, lpText, _countof(ch));
        return HostMessageBox(ch, uType);
    }

    // MessageBoxA
    decltype(MessageBoxW) *pMessageBoxA = NULL;
    int WINAPI DetourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
        return HostMessageBox(lpText, uType);
    }

    void init_ui_detours() {
        MH_Initialize();
        MH_CreateHook(&MessageBoxW, &DetourMessageBoxW, reinterpret_cast<LPVOID*>(&pMessageBoxW));
        MH_CreateHook(&MessageBoxA, &DetourMessageBoxA, reinterpret_cast<LPVOID*>(&pMessageBoxA));

        MH_EnableHook(&MessageBoxW);
        MH_EnableHook(&MessageBoxA);
    }

    void terminate_ui_detours() {
        MH_DisableHook(&MessageBoxW);
        MH_DisableHook(&MessageBoxA);
        MH_Uninitialize();
    }
}