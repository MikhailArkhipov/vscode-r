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

#include "rstrtmgr.h"
#include "Rapi.h"
#include <RestartManager.h>

namespace rhost {
    namespace util {
#define PACKAGE_LOCK_STATE_UNLOCKED        "unlocked"
#define PACKAGE_LOCK_STATE_LOCKED_BY_R     "locked_by_r"
#define PACKAGE_LOCK_STATE_LOCKED_BY_OTHER "locked_by_other"

        const char* lock_state_by_file(const char* file_path) {
            WCHAR fpath[MAX_PATH] = {};
            size_t wchar_len = 0, char_len = strlen(file_path);

            auto err = mbstowcs_s(&wchar_len, fpath, file_path, char_len);
            if (err != 0) {
                Rf_error("Unable to check the library lock state.");
            }

            std::unique_ptr<WCHAR> session_uuid_str(new WCHAR[128]);
            DWORD session_handle = 0;
            DWORD error = RmStartSession(&session_handle, 0, session_uuid_str.get());
            if (error == ERROR_SUCCESS) {
                LPCWSTR files[] = { fpath };
                error = RmRegisterResources(session_handle, 1, files, 0, nullptr, 0, nullptr);
            }

            UINT process_info_needed = 0;
            UINT process_info_count = 0;
            DWORD reboot_reason = RmRebootReasonNone;
            if (error == ERROR_SUCCESS) {
                error = RmGetList(session_handle, &process_info_needed, &process_info_count, nullptr, &reboot_reason);
            }

            std::vector<DWORD> process_ids;
            do {
                std::unique_ptr<RM_PROCESS_INFO> process_info_data(new RM_PROCESS_INFO[process_info_needed]);
                process_info_count = process_info_needed;
                error = RmGetList(session_handle, &process_info_needed, &process_info_count, process_info_data.get(), &reboot_reason);
                if (error == ERROR_SUCCESS) {
                    for (UINT i = 0; i < process_info_count; ++i) {
                        RM_PROCESS_INFO* pinfo = (process_info_data.get() + i);
                        if (pinfo != nullptr) {
                            process_ids.push_back(pinfo->Process.dwProcessId);
                        }
                    }
                    break;
                }
            } while (error == ERROR_MORE_DATA);

            RmEndSession(session_handle);

            DWORD rhost_pid = GetCurrentProcessId();

            if (process_ids.size() == 1 && process_ids[0] == rhost_pid) {
                return PACKAGE_LOCK_STATE_LOCKED_BY_R;
            } else if (process_ids.size() > 0) {
                return PACKAGE_LOCK_STATE_LOCKED_BY_OTHER;
            }

            return PACKAGE_LOCK_STATE_UNLOCKED;
        }
    }
}