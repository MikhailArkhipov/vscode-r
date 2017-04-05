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

#define RHOST_NO_API_REDIRECT
#include "stdafx.h"

HMODULE r_module = nullptr;
HMODULE rgraphapp_module = nullptr;

#define _GET_PROC(m, api) _RAPI_PTR(api) = get_proc<decltype(api)>(m, _RAPI_STR(api));
#define _RAPI_GET_PROC(api) _GET_PROC(r_module, api)
#define _RAPI_UNLOAD(api) _RAPI_PTR(api) = nullptr;

#ifdef _WIN32
#define _RGRAPHAPPAPI_GET_PROC(api) _GET_PROC(rgraphapp_module, api)
#endif

namespace rhost {
    namespace rapi {
        _RAPI_SET(_RAPI_DEFINE_NULLPTR);
        _RGRAPHAPPAPI_SET(_RAPI_DEFINE_NULLPTR);

        namespace {
            template<typename T>
            T* get_proc(HMODULE module, std::string proc) {
                T* ptr = nullptr;
#ifdef _WIN32
                ptr = (T*)GetProcAddress(module, proc.c_str());
#endif
                return ptr;
            }

            void internal_load_r_apis() {
                _RAPI_SET(_RAPI_GET_PROC);
            }

            void internal_load_rgraphapp_apis() {
                _RGRAPHAPPAPI_SET(_RGRAPHAPPAPI_GET_PROC);
            }

            void internal_unload_r_apis() {
                _RAPI_SET(_RAPI_UNLOAD);
            }
        }
        

        void load_r_apis(fs::path r_dll_dir) {
#ifdef _WIN32 
            fs::path r_path = r_dll_dir / "r.dll";
            fs::path rgraphapp_path = r_dll_dir / "rgraphapp.dll";

            r_module = LoadLibraryEx(r_path.make_preferred().wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_USER_DIRS);
            rgraphapp_module = LoadLibraryEx(rgraphapp_path.make_preferred().wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_USER_DIRS);
            internal_load_rgraphapp_apis();
#endif
            internal_load_r_apis();
        }

        void unload_r_apis() {
            internal_unload_r_apis();
        }
    }
}

