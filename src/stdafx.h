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
#include <filesystem>
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
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include "windows.h"
#include <strsafe.h>
#else
#include <unistd.h>
#endif


#pragma warning(push)
#pragma warning(disable:4091)
#include "dbghelp.h"
#pragma warning(pop)

#include "minhook.h"

#include "zip.h"

namespace fs = std::tr2::sys;
