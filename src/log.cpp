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
#include "log.h"
#include "Rapi.h"

using namespace std::literals;

namespace rhost {
    namespace log {
        namespace {
#ifdef _MSC_VER
            const MINIDUMP_TYPE fulldump_type = MINIDUMP_TYPE(
                MiniDumpWithFullMemory |
                MiniDumpWithDataSegs |
                MiniDumpWithHandleData |
                MiniDumpWithProcessThreadData |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithThreadInfo |
                MiniDumpIgnoreInaccessibleMemory |
                MiniDumpWithTokenInformation |
                MiniDumpWithModuleHeaders);

            const DWORD fatal_error_exception_code = 0xE0000001;
#endif

            std::mutex log_mutex, terminate_mutex;
            fs::path log_filename, stackdump_filename, fulldump_filename;
            FILE* logfile;
            int indent;
            log::log_verbosity current_verbosity;

            void log_flush_thread() {
                for (;;) {
                    std::this_thread::sleep_for(1s);
                    flush_log();
                }
            }
        }

#ifdef _MSC_VER
        void create_minidump(_EXCEPTION_POINTERS* ei) {
            // Don't let another thread interrupt us by terminating while we're doing this.
            std::lock_guard<std::mutex> terminate_lock(terminate_mutex);

            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = ei;
            mei.ClientPointers = FALSE;

            // Create a regular minidump.
            HANDLE dump_file = CreateFileA(stackdump_filename.make_preferred().string().c_str(), GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump_file, MiniDumpNormal, &mei, nullptr, nullptr)) {
                logf(log_verbosity::minimal, "Stack-only minidump written out to %s\n", stackdump_filename.c_str());
            } else {
                logf(log_verbosity::minimal, "Failed to write stack-only minidump to %s\n", stackdump_filename.c_str());
            }
            CloseHandle(dump_file);
            flush_log();

            // Create a full heap minidump with as much data as possible.
            dump_file = CreateFileA(fulldump_filename.make_preferred().string().c_str(), GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump_file, fulldump_type, &mei, nullptr, nullptr)) {
                logf(log_verbosity::minimal, "Full minidump written out to %s\n", fulldump_filename.c_str());
            } else {
                logf(log_verbosity::minimal, "Failed to write full minidump to %s\n", fulldump_filename.c_str());
            }
            CloseHandle(dump_file);
            flush_log();
        }

        LONG WINAPI unhandled_exception_filter(_EXCEPTION_POINTERS* ei) {
            // Prevent recursion if an unhandled exception happens inside the filter itself.
            static bool in_unhandled_exception_filter;
            if (in_unhandled_exception_filter) {
                return EXCEPTION_CONTINUE_SEARCH;
            }
            in_unhandled_exception_filter = true;

            // Flush log, so that if anything below fails (e.g. if heap is corrupted too badly, or
            // if we're out of memory), at least the stuff that's already in the log gets written
            flush_log();

            logf(log_verbosity::minimal, "Terminating process due to unhandled Win32 exception 0x%x\n", ei->ExceptionRecord->ExceptionCode);
            flush_log();
            create_minidump(ei);

            in_unhandled_exception_filter = false;
            return EXCEPTION_CONTINUE_SEARCH;
        }
#endif

        void init_log(const std::string& log_suffix, const fs::path& log_dir, log::log_verbosity verbosity, bool suppress_ui) {
            {
                current_verbosity = verbosity;

                std::string filename = "Microsoft.R.Host_";
                if (!log_suffix.empty()) {
                    filename += log_suffix + "_";
                }

                time_t t;
                time(&t);

                tm tm;
#ifdef _WIN32
                localtime_s(&tm, &t);
#else
                localtime_r(&t, &tm);
#endif
                size_t len = filename.size();
                filename.resize(len + 1 + RHOST_MAX_PATH);
                auto it = filename.begin() + len;
                strftime(&*it, filename.end() - it, "%Y%m%d_%H%M%S", &tm);
                filename.resize(strlen(filename.c_str()));

                // Add PID to prevent conflicts in case two hosts with the same suffix
                // get started at the same time.
                filename += "_pid" + std::to_string(getpid());

                log_filename = log_dir / (filename + ".log");
                stackdump_filename = log_dir / (filename + ".stack.dmp");
                fulldump_filename = log_dir / (filename + ".full.dmp");
            }

#ifdef _MSC_VER
            logfile = _fsopen(log_filename.make_preferred().string().c_str(), "wc", _SH_DENYWR);
#else
            logfile = fopen(log_filename.make_preferred().string().c_str(), "w");
#endif
            if (logfile) {
                // Logging happens often, so use a large buffer to avoid hitting the disk all the time.
                setvbuf(logfile, nullptr, _IOFBF, 0x100000);

                // Start a thread that will flush the buffer periodically.
                std::thread(log_flush_thread).detach();
            } else {
                std::string error = "Error creating logfile: " + log_filename.make_preferred().string() + "\r\n";
                fprintf(stderr, "Error: %d\r\n", errno);
                fputs(error.c_str(), stderr);
                
                if (!suppress_ui) {
#ifdef _WIN32
                    MessageBoxA(HWND_DESKTOP, error.c_str(), "Microsoft R Host", MB_OK | MB_ICONWARNING);
#endif
                }
            }

#ifdef _MSC_VER
            SetUnhandledExceptionFilter(unhandled_exception_filter);
#endif
        }

        void vlogf(log_verbosity verbosity, log_level message_type, const char* format, va_list va) {
            if (verbosity > current_verbosity) {
                return;
            }

            std::lock_guard<std::mutex> lock(log_mutex);

            va_list va2;
            va_copy(va2, va);

            if (logfile) {
                for (int i = 0; i < indent; ++i) {
                    fputc('\t', logfile);
                }
                vfprintf(logfile, format, va);

#ifndef NDEBUG
                // In Debug builds, flush on every write so that log is always up-to-date.
                // In Release builds, we rely on flush_log being called on process shutdown.
                fflush(logfile);
#endif
            }

            // Don't log trace level messages to stderr by default.
            if (message_type != log_level::trace) {
                for (int i = 0; i < indent; ++i) {
                    fputc('\t', stderr);
                }
                vfprintf(stderr, format, va2);
            }

            va_end(va2);
        }

        void indent_log(int n) {
            indent += n;
            if (indent < 0) {
                indent = 0;
            }
        }

        void flush_log() {
            std::lock_guard<std::mutex> lock(log_mutex);
            if (logfile) {
                fflush(logfile);
            }
        }


        void terminate(bool unexpected, const char* format, va_list va) {
            std::lock_guard<std::mutex> terminate_lock(terminate_mutex);

            char message[0xFFFF];
            vsprintf_s(message, format, va);
            if (unexpected) {
                logf(log_verbosity::minimal, "Fatal error: ");
            }
            logf(log_verbosity::minimal, unexpected ? log_level::error : log_level::information, "%s\n", message);
            flush_log();

            if (unexpected) {
                std::string msgbox_text;
                for (size_t i = 0; i < strlen(message); ++i) {
                    char c = message[i];
                    if (c == '\n') {
                        msgbox_text += '\r';
                    } 
                    msgbox_text += c;
                }
                
#ifdef _MSC_VER
                // Raise and catch an exception so that minidump with a stacktrace can be produced from it.
                [&] {
                    terminate_mutex.unlock();
                    __try {
                        RaiseException(fatal_error_exception_code, 0, 0, nullptr);
                    } __except(unhandled_exception_filter(GetExceptionInformation()), EXCEPTION_CONTINUE_EXECUTION) {
                    }
                    terminate_mutex.lock();
                }();
#endif
            }
            
            R_CleanUp(SA_NOSAVE, (unexpected ? EXIT_FAILURE : EXIT_SUCCESS), 0);
        }

        void terminate(const char* format, ...) {
            va_list va;
            va_start(va, format);
            terminate(false, format, va);
            va_end(va);
        }

        void fatal_error(const char* format, ...) {
            va_list va;
            va_start(va, format);
            terminate(true, format, va);
            va_end(va);
        }
    }
}
