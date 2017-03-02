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

#pragma once
#pragma warning(disable: 4996)

#define NOMINMAX

#include <atomic>
#include <cinttypes>
#include <codecvt>
#include <chrono>
#include <condition_variable>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "boost/algorithm/string.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/endian/buffers.hpp"
#include "boost/format.hpp"
#include "boost/program_options/cmdline.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/optional.hpp"
#include "boost/locale.hpp"
#include "boost/signals2/signal.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "boost/uuid/uuid_generators.hpp"

#include "picojson.h"

#ifdef _WIN32
#include <filesystem>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <windows.h>
#include <strsafe.h>

#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

#include "minhook.h"

#include "zip.h"

namespace fs = std::tr2::sys;
#else
#include "boost/filesystem.hpp"
#include <stdarg.h>
#include <unistd.h>
namespace fs = boost::filesystem;

#include <sys/syscall.h>

#ifdef SYS_gettid
pid_t gettid() { return syscall(SYS_gettid); }
#else
#error "SYS_gettid unavailable on this system"
#endif

#endif

#if defined(_MSC_VER)
#define RHOST_EXPORT __declspec(dllexport)
#define RHOST_IMPORT __declspec(dllimport)
#define RHOST_NORETURN __declspec(noreturn)
#define RHOST_NOTHROW __declspec(nothrow)
#elif defined(__GNUC__)
#define RHOST_EXPORT __attribute__((visibility("default")))
#define RHOST_IMPORT
#define RHOST_NORETURN
#define RHOST_NOTHROW
#else
#define RHOST_EXPORT
#define RHOST_IMPORT
#define RHOST_NORETURN
#define RHOST_NOTHROW
#pragma warning Unknown DLL import/export.
#endif

#ifdef _WIN32
#define RHOST_MAX_PATH MAX_PATH
typedef DWORD threadid_t;
#define RhostGetCurrentThreadId GetCurrentThreadId
#define RHOST_TRY __try
#define RHOST_FINALLY_START __finally {
#define RHOST_FINALLY_END }
#define RHOST_mbstowcs msvcrt::mbstowcs
#define RHOST_wctomb msvcrt::wctomb
#define RHOST_mbtowc msvcrt::mbtowc
#else

#define RHOST_MAX_PATH PATH_MAX
typedef pid_t threadid_t;
#define RhostGetCurrentThreadId gettid
#define RHOST_TRY try
#define RHOST_FINALLY_START catch(...) {
#define RHOST_FINALLY_END throw;}
#define RHOST_mbstowcs std::mbstowcs
#define RHOST_wctomb std::wctomb
#define RHOST_mbtowc std::mbtowc
#endif


