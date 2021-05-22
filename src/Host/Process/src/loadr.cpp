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
#include "r_api.h"
#include "log.h"

#define RHOST_RAPI_DEFINE(api) decltype(api) *RHOST_RAPI_PTR(api)
#define RHOST_RAPI_DEFINE_NULLPTR(api) RHOST_RAPI_DEFINE(api) = nullptr;

#define RHOST_GD_DEFINE(api) \
    decltype(gd_api<10>::api) gd_api<10>::api; \
    decltype(gd_api<12>::api) gd_api<12>::api; \
    decltype(gd_api<14>::api) gd_api<14>::api;

#define RHOST_GET_PROC(m, api) RHOST_RAPI_PTR(api) = get_proc<decltype(api)*>(m, RHOST_RAPI_STR(api));
#define RHOST_RAPI_GET_PROC(api) RHOST_GET_PROC(r_module, api)
#define RHOST_RAPI_UNLOAD(api) RHOST_RAPI_PTR(api) = nullptr;

#define RHOST_GD_GET_PROC(api) api = get_proc<decltype(api)>(r_module, RHOST_RAPI_STR(api));
#define RHOST_GD_UNLOAD(api) api = nullptr;

#ifdef _WIN32
#define RHOST_RGRAPHAPPAPI_GET_PROC(api) RHOST_GET_PROC(rgraphapp_module, api)
#endif

#ifdef _WIN32
typedef HMODULE rhost_module_t;
#else
typedef void* rhost_module_t;
#endif 

namespace rhost {
    namespace rapi {
        RHOST_RAPI_SET(RHOST_RAPI_DEFINE_NULLPTR);
        RHOST_GD_SET(RHOST_GD_DEFINE);
#ifdef _WIN32
        RHOST_RGRAPHAPPAPI_SET(RHOST_RAPI_DEFINE_NULLPTR);
#endif

        namespace {
            rhost_module_t r_module = nullptr;
            rhost_module_t rgraphapp_module = nullptr;

            template<typename T>
            T get_proc(rhost_module_t module, const char* proc_name) {
                T ptr = nullptr;
#ifdef _WIN32
                ptr = reinterpret_cast<T>(GetProcAddress(module, proc_name));
                if (!ptr) {
                    log::fatal_error("Error failed to get address of %s: %d", proc_name, GetLastError());
                }
#else
                ptr = reinterpret_cast<T>(dlsym(module, proc_name));
                if (!ptr) {
                    log::fatal_error("Error failed to get address of %s: %s", proc_name, dlerror());
                }
#endif
                return ptr;
            }

            void internal_load_r_apis() {
                RHOST_RAPI_SET(RHOST_RAPI_GET_PROC);
            }

            void internal_unload_r_apis() {
                RHOST_RAPI_SET(RHOST_RAPI_UNLOAD);
            }

#ifdef _WIN32
            void internal_load_rgraphapp_apis() {
                RHOST_RGRAPHAPPAPI_SET(RHOST_RGRAPHAPPAPI_GET_PROC);
            }

            void internal_unload_rgraphapp_apis() {
                RHOST_RGRAPHAPPAPI_SET(RHOST_RAPI_UNLOAD);
            }
#endif
        }

        void gd_api<10>::load() {
            RHOST_GD_SET(RHOST_GD_GET_PROC);
        }

        void gd_api<10>::unload() {
            RHOST_GD_SET(RHOST_GD_UNLOAD);
        }

        void gd_api<12>::load() {
            RHOST_GD_SET(RHOST_GD_GET_PROC);
        }

        void gd_api<12>::unload() {
            RHOST_GD_SET(RHOST_GD_UNLOAD);
        }

        void gd_api<14>::load() {
            RHOST_GD_SET(RHOST_GD_GET_PROC);
        }

        void gd_api<14>::unload() {
            RHOST_GD_SET(RHOST_GD_UNLOAD);
        }

        void load_r_apis(fs::path& r_dll_dir) {
#ifdef _WIN32 
            fs::path r_path = r_dll_dir / "r.dll";
            fs::path rgraphapp_path = r_dll_dir / "rgraphapp.dll";

            r_module = LoadLibraryEx(r_path.make_preferred().wstring().c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (!r_module) {
                log::fatal_error("Error r module failed to load: %d", GetLastError());
            }

            rgraphapp_module = LoadLibraryEx(rgraphapp_path.make_preferred().wstring().c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (!rgraphapp_module) {
                log::fatal_error("Error rgraphapp module failed to load: %d", GetLastError());
            }

            internal_load_rgraphapp_apis();
#else // POSIX
            // Try Linux first
            fs::path r_path = r_dll_dir / "lib/libR.so";
            r_module = dlopen(r_path.make_preferred().string().c_str(), RTLD_LOCAL | RTLD_LAZY);
            if (!r_module) {
                // Try Mac
                r_path = r_dll_dir / "lib/libR.dylib";
                r_module = dlopen(r_path.make_preferred().string().c_str(), RTLD_LOCAL | RTLD_LAZY);
                if (!r_module) {
                    log::fatal_error("Error r module failed to load: %s", dlerror());
                }
            }
#endif
            internal_load_r_apis();

            switch (int ver = fp_R_GE_getVersion()) {
            case 10:
                gd_api<10>::load();
                break;
            case 11:
                gd_api<11>::load();
                break;
            case 12:
                gd_api<12>::load();
                break;
            case 14:
                gd_api<14>::load();
                break;
            default:
                log::fatal_error("Unsupported GD API version %d", ver);
            }
        }

        void unload_r_apis() {
            switch (int ver = fp_R_GE_getVersion()) {
            case 10:
                gd_api<10>::unload();
                break;
            case 11:
                gd_api<11>::unload();
                break;
            case 12:
                gd_api<12>::unload();
                break;
            case 14:
                gd_api<14>::unload();
                break;
            default:
                log::fatal_error("Unsupported GD API version %d", ver);
            }

            internal_unload_r_apis();
#ifdef _WIN32
            internal_unload_rgraphapp_apis();
            FreeLibrary(r_module);
            FreeLibrary(rgraphapp_module);
#else // POSIX
            dlclose(r_module);
#endif
        }
    }
}

