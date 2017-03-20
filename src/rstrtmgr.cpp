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
#include "util.h"

#ifdef _WIN32
#include "Rapi.h"
#include <RestartManager.h>
#endif

namespace rhost {
    namespace util {
        file_lock_state lock_state_by_file(std::vector<std::wstring>& wpaths) {
#ifdef _WIN32
            std::vector<LPCWSTR> wfilepaths(wpaths.size());
            std::transform(wpaths.begin(), wpaths.end(), wfilepaths.begin(), [](std::wstring& wpath) { return wpath.data(); });

            std::unique_ptr<WCHAR> session_uuid_str(new WCHAR[128]);
            DWORD session_handle = 0;
            SCOPE_WARDEN(rstrtmgr_end, {
                if (session_handle) {
                    RmEndSession(session_handle);
                }
            });

            DWORD error = RmStartSession(&session_handle, 0, session_uuid_str.get());
            if (error != ERROR_SUCCESS) {
                return file_lock_state::unlocked;
            }

            error = RmRegisterResources(session_handle, static_cast<UINT>(wfilepaths.size()), wfilepaths.data(), 0, nullptr, 0, nullptr);
            if (error != ERROR_SUCCESS) {
                return file_lock_state::unlocked;
            }

            UINT process_info_needed = 0;
            UINT process_info_count = 0;
            DWORD reboot_reason = RmRebootReasonNone;
            error = RmGetList(session_handle, &process_info_needed, &process_info_count, nullptr, &reboot_reason);
            if (error != ERROR_SUCCESS && error != ERROR_MORE_DATA) {
                return file_lock_state::unlocked;
            }

            std::vector<RM_PROCESS_INFO> process_info_data;
            while (error == ERROR_MORE_DATA) {
                process_info_data.resize(process_info_needed);
                process_info_count = process_info_needed;
                error = RmGetList(session_handle, &process_info_needed, &process_info_count, process_info_data.data(), &reboot_reason);
            }

            if (error != ERROR_SUCCESS) {
                return file_lock_state::unlocked;
            }

            std::vector<DWORD> process_ids(process_info_data.size());
            std::transform(process_info_data.begin(), process_info_data.end(), process_ids.begin(), [](RM_PROCESS_INFO p) { return p.Process.dwProcessId; });

            DWORD rhost_pid = GetCurrentProcessId();
            if (process_ids.size() == 1 && process_ids[0] == rhost_pid) {
                return file_lock_state::locked_by_r_session;
            } else if (process_ids.size() > 0) {
                return file_lock_state::locked_by_other;
            }
#endif
            return file_lock_state::unlocked;
        }
    }
}