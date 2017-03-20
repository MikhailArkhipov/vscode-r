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

namespace rhost {
    namespace detours {
#ifdef _WIN32
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
        decltype(MessageBoxW) *pMessageBoxW = nullptr;
        int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
            char ch[1000];
            wcstombs(ch, lpText, _countof(ch));
            return HostMessageBox(ch, uType);
        }

        // MessageBoxA
        decltype(MessageBoxA) *pMessageBoxA = nullptr;
        int WINAPI DetourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
            return HostMessageBox(lpText, uType);
        }

        std::unordered_map<HWND, std::string> hWndTitleMap;
        decltype(CreateWindowExA) *pCreateWindowExA = nullptr;
        HWND WINAPI DetourCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
            HWND hWnd = pCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

            // A window without parent or a window created to show message
            size_t len = 0;
            if (hWnd != nullptr && hWndParent == nullptr && 
                hWndTitleMap.find(hWnd) == hWndTitleMap.end() && 
                SUCCEEDED(StringCbLengthA(lpWindowName, STRSAFE_MAX_CCH, &len)) && len > 0) {

                int size = MultiByteToWideChar(CP_ACP, 0, lpWindowName, static_cast<int>(len), NULL, 0);
                if (size == 0) {
                    return hWnd;
                }

                std::unique_ptr<wchar_t[]> wstr(new wchar_t[size + 1]);
                MultiByteToWideChar(CP_ACP, 0, lpWindowName, static_cast<int>(len), wstr.get(), size);
                wstr[size] = '\0';

                std::wstring windowTitle = std::wstring(wstr.get());
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> strConverter;

                hWndTitleMap[hWnd] = strConverter.to_bytes(windowTitle);
                std::string text = "Created window - " + hWndTitleMap[hWnd];

                rhost::host::send_notification("!!", text);
            }

            return hWnd;
        }

        decltype(CreateWindowExW) *pCreateWindowExW = nullptr;
        HWND WINAPI DetourCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
            HWND hWnd = pCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
            
            // A window without parent or a window created to show message
            size_t len = 0;
            if (hWnd != nullptr && hWndParent == nullptr && 
                hWndTitleMap.find(hWnd) == hWndTitleMap.end() && 
                SUCCEEDED(StringCchLengthW(lpWindowName, STRSAFE_MAX_CCH, &len) && len > 0)) {

                std::wstring windowTitle = std::wstring(lpWindowName);
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> strConverter;

                hWndTitleMap[hWnd] = strConverter.to_bytes(windowTitle);
                std::string text = "Created window - " + hWndTitleMap[hWnd];

                rhost::host::send_notification("!!", text);
            }

            return hWnd;
        }

        decltype(DestroyWindow) *pDestroyWindow = nullptr;
        BOOL WINAPI DetourDestroyWindow(HWND hWnd) {
            BOOL stat = pDestroyWindow(hWnd);
            std::unordered_map<HWND, std::string>::iterator iter = hWndTitleMap.find(hWnd);
            if (iter != hWndTitleMap.end()) {
                std::string text = "Closing window - "  + iter->second;
                hWndTitleMap.erase(iter);
                rhost::host::send_notification("!!", text);
            }

            return stat;
        }
#endif

        void init_ui_detours(bool is_remote) {
#ifdef _WIN32
            MH_Initialize();

            MH_CreateHook(reinterpret_cast<void*>(&MessageBoxW), reinterpret_cast<void*>(&DetourMessageBoxW), reinterpret_cast<LPVOID*>(&pMessageBoxW));
            MH_CreateHook(reinterpret_cast<void*>(&MessageBoxA), reinterpret_cast<void*>(&DetourMessageBoxA), reinterpret_cast<LPVOID*>(&pMessageBoxA));

            if (is_remote) {
                // Apply hooks for Remote REPL or any other session irrespective of local or remote case.
                MH_CreateHookApi(L"User32.dll", "CreateWindowExW", reinterpret_cast<void*>(&DetourCreateWindowExW), reinterpret_cast<LPVOID*>(&pCreateWindowExW));
                MH_CreateHookApi(L"User32.dll", "CreateWindowExA", reinterpret_cast<void*>(&DetourCreateWindowExA), reinterpret_cast<LPVOID*>(&pCreateWindowExA));
                MH_CreateHookApi(L"User32.dll", "DestroyWindow", reinterpret_cast<void*>(&DetourDestroyWindow), reinterpret_cast<LPVOID*>(&pDestroyWindow));
            }

            MH_EnableHook(MH_ALL_HOOKS);
#endif
        }

        void terminate_ui_detours() {
#ifdef _WIN32
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
#endif
        }
    }
}