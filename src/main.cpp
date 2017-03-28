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
#include "msvcrt.h"
#include "log.h"
#include "r_util.h"
#include "host.h"
#include "Rapi.h"
#include "util.h"
#include "eval.h"
#include "detours.h"
#include "grdeviceside.h"
#include "grdevicesxaml.h"
#include "exports.h"
#include "transport.h"

using namespace rhost::eval;
using namespace rhost::log;
using namespace rhost::util;
namespace po = boost::program_options;

typedef void (*ptr_RHOST_WriteConsoleEx)(const char *, int, int);

namespace rhost {
#ifdef _WIN32
    // ID for the timer 
    const UINT_PTR IDT_RESET_TIMER = 1;
#endif

    struct command_line_args {
        fs::path log_dir, rdata;
        std::string name;
        log::log_verbosity log_level;
        std::chrono::seconds idle_timeout;
        std::vector<std::string> unrecognized;
        bool suppress_ui;
        bool is_interactive;
        int argc;
        std::vector<char*> argv;
    };

    command_line_args parse_command_line(int argc, char** argv) {
        command_line_args args = {};

        po::option_description
            help("rhost-help", new po::untyped_value(true),
                "Produce help message."),
            name("rhost-name", po::value<std::string>(),
                "Name of this host instance."),
            log_level("rhost-log-verbosity", po::value<int>(),
                "Log verbosity."),
            log_dir("rhost-log-dir", po::value<std::string>(),
                "Directory to store host logs and dumps."),
            rdata("rhost-rdata", po::value<std::string>(),
                "RData file to load initial workspace from, and to save it to when suspending."),
            idle_timeout("rhost-idle-timeout", po::value<std::chrono::seconds::rep>(), (
                "Shut down the host if it is idle for the specified duration in seconds. "
                "If " + rdata.long_name() + " was specified, save workspace before exiting."
                ).c_str()),
            suppress_ui("rhost-suppress-ui", new po::untyped_value(true),
                "Suppress any UI (e.g., Message Box) from this host instance."),
            is_interactive("rhost-interactive", new po::untyped_value(true),
                "This R is configured to start in interactive mode.");

        po::options_description desc;
        for (auto&& opt : { help, name, log_level, log_dir, rdata, idle_timeout, suppress_ui, is_interactive }) {
            boost::shared_ptr<po::option_description> popt(new po::option_description(opt));
            desc.add(popt);
        }

        po::variables_map vm;
        try {
            auto parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
            po::store(parsed, vm);
            po::notify(vm);
            args.unrecognized = po::collect_unrecognized(parsed.options, po::include_positional);
        } catch (po::error& e) {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << desc << std::endl;
            std::exit(EXIT_FAILURE);
        }

        if (vm.count(help.long_name())) {
            std::cerr << desc << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        auto name_arg = vm.find(name.long_name());
        if (name_arg != vm.end()) {
            args.name = name_arg->second.as<std::string>();
        }

        auto log_level_arg = vm.find(log_level.long_name());
        if (log_level_arg != vm.end()) {
            args.log_level = static_cast<log_verbosity>(log_level_arg->second.as<int>());
        } else {
            args.log_level = log_verbosity::normal;
        }

        auto log_dir_arg = vm.find(log_dir.long_name());
        if (log_dir_arg != vm.end()) {
            args.log_dir = log_dir_arg->second.as<std::string>();
        } else {
            args.log_dir = fs::temp_directory_path();
        }

        auto rdata_arg = vm.find(rdata.long_name());
        if (rdata_arg != vm.end()) {
            args.rdata = rdata_arg->second.as<std::string>();
        }

        auto idle_timeout_arg = vm.find(idle_timeout.long_name());
        if (idle_timeout_arg != vm.end()) {
            auto n = idle_timeout_arg->second.as<std::chrono::seconds::rep>();
            args.idle_timeout = std::chrono::seconds(n);
        }

        args.suppress_ui = vm.count(suppress_ui.long_name()) != 0;
        args.is_interactive = vm.count(is_interactive.long_name()) != 0;

        args.argv.push_back(argv[0]);
        for (auto& s : args.unrecognized) {
            args.argv.push_back(&s[0]);
        }
        args.argc = int(args.argv.size());
        args.argv.push_back(nullptr);

        return args;
    }

    void set_memory_limit() {
#ifdef _WIN32
        MEMORYSTATUSEX ms = {};
        ms.dwLength = sizeof ms;
        if (!GlobalMemoryStatusEx(&ms)) {
            logf(log_verbosity::minimal, "Couldn't set R memory limit - GlobalMemoryStatusEx failed with GetLastError=%u\n", GetLastError());
            return;
        }

        double new_limit = static_cast<double>(ms.ullTotalPhys / 1024 / 1024);

        double old_limit = 0;
        r_top_level_exec([&] {
            old_limit = *REAL(in_memsize(R_LogicalNAValue));
        });
        if (old_limit >= new_limit) {
            return;
        }

        logf(log_verbosity::minimal, "Setting R memory limit to %0.0f MB\n", new_limit);

        protected_sexp limit = Rf_allocVector(REALSXP, 1);
        *REAL(limit.get()) = new_limit;
        if (!r_top_level_exec([&] { in_memsize(limit.get()); })) {
            logf(log_verbosity::minimal, "Couldn't set R memory limit - in_memsize failed: %s\n", R_curErrorBuf());
        }
#endif
    }

    void suggest_mro(ptr_RHOST_WriteConsoleEx write_console_ex) {
        ParseStatus ps;
        auto res = r_try_eval_str("if (exists('Revo.version')) 'REVO' else 'CRAN'", R_BaseEnv, ps);

        assert(res.has_value);
        if (!res.has_value) {
            return;
        }

        if (res.value == "REVO") {
            // This is Revolution R or Microsoft R.
            return;
        }

        static const char mro_banner[] = "Check out Microsoft's enhanced R distribution at https://aka.ms/mrclient. \n\n";
        write_console_ex(mro_banner, static_cast<int>(strlen(mro_banner)), 0);
    }

#ifdef _WIN32
    int run_r_windows(command_line_args& args) {
        init_log(args.name, args.log_dir, args.log_level, args.suppress_ui);
        transport::initialize();
        
        R_setStartTime();
        structRstart rp = {};
        R_DefParams(&rp);

        rp.rhome = get_R_HOME();
        rp.home = getRUser();
        rp.CharacterMode = RGui;
        rp.R_Quiet = R_FALSE;
        rp.R_Interactive = args.is_interactive ? R_TRUE : R_FALSE;
        rp.RestoreAction = SA_NORESTORE;
        rp.SaveAction = SA_NOSAVE;

        rhost::host::initialize(rp, args.rdata, args.idle_timeout);

        // suppress UI is set only in the remote case, for now can be used to
        // as equivalent of is_remote.
        rhost::detours::init_ui_detours(args.suppress_ui);

        R_set_command_line_arguments(args.argc, args.argv.data());
        R_common_command_line(&args.argc, args.argv.data(), &rp);
        R_SetParams(&rp);

        GA_initapp(0, 0);
        readconsolecfg();

        DllInfo *dll = R_getEmbeddingDllInfo();
        rhost::r_util::init(dll);
        rhost::grdevices::xaml::init(dll);
        rhost::grdevices::ide::init(dll);
        rhost::exports::register_all(dll);

        CharacterMode = LinkDLL;
        setup_Rmainloop();
        CharacterMode = RGui;

        set_memory_limit();

        // setup_Rmainloop above prints out the license banner, so this will follow that.
        suggest_mro(rp.WriteConsoleEx);

        if (!args.rdata.empty()) {
            std::string s = args.rdata.string();
            log::logf(log_verbosity::minimal, "Loading workspace from file %s\n", s.c_str());

            bool ok = r_top_level_exec([&] {
                R_RestoreGlobalEnvFromFile(s.c_str(), R_FALSE);
            });

            log::logf(log_verbosity::minimal, ok ? "Workspace loaded successfully.\n" : "Failed to load workspace.\n");
        }

        UINT_PTR timer = SetTimer(NULL, IDT_RESET_TIMER, 5000, [](HWND hWnd, UINT msg, UINT_PTR idEVent, DWORD dwTime) {
            rhost::host::do_r_callback(false);
        });

        SCOPE_WARDEN(destroy_timer, {
            KillTimer(NULL, timer);
        });

        run_Rmainloop();

        return EXIT_SUCCESS;
    }

#else // POSIX
    int run_r_posix(command_line_args& args) {
        init_log(args.name, args.log_dir, args.log_level, args.suppress_ui);
        transport::initialize();
        
        R_running_as_main_program = 1;
        
        ptr_R_ShowMessage = rhost::host::ShowMessage;

         char *argv[] = {"Microsoft.R.Host", "--interactive"};
        int argc = sizeof(argv)/sizeof(argv[0]);
        int res = Rf_initialize_R(argc, argv);

        structRstart rp = {};
        R_DefParams(&rp);
        rp.R_Quiet = R_FALSE;
        rp.R_Interactive = args.is_interactive ? R_TRUE : R_FALSE;
        rp.RestoreAction = SA_NORESTORE;
        rp.SaveAction = SA_NOSAVE;

        rhost::host::initialize(rp, args.rdata, args.idle_timeout);

        R_set_command_line_arguments(args.argc, args.argv.data());
        R_common_command_line(&args.argc, args.argv.data(), &rp);
        R_SetParams(&rp);
        
        // This is a exported static library member Rf_initialize_R sets it to TRUE
        R_Interactive = args.is_interactive ? R_TRUE : R_FALSE;
        R_Consolefile = nullptr;
        R_Outputfile = nullptr;

        DllInfo *dll = R_getEmbeddingDllInfo();
        rhost::r_util::init(dll);
        rhost::grdevices::xaml::init(dll);
        rhost::grdevices::ide::init(dll);
        rhost::exports::register_all(dll);

        rhost::host::setCallbacksPOSIX();

        if (!args.rdata.empty()) {
            std::string s = args.rdata.string();
            log::logf(log_verbosity::minimal, "Loading workspace from file %s\n", s.c_str());

            bool ok = r_top_level_exec([&] {
                R_RestoreGlobalEnvFromFile(s.c_str(), R_FALSE);
            });

            log::logf(log_verbosity::minimal, ok ? "Workspace loaded successfully.\n" : "Failed to load workspace.\n");
        }

        Rf_mainloop();

        return 0;
    }
#endif

    int run(int argc, char** argv) {
        auto args = rhost::parse_command_line(argc, argv);
#ifdef _WIN32
        return rhost::run_r_windows(args);
#else
        return rhost::run_r_posix(args);
#endif 
    }
}

int main(int argc, char** argv) {
    setlocale(LC_NUMERIC, "C");

    SCOPE_WARDEN(_main_exit, {
        flush_log();
        rhost::detours::terminate_ui_detours();
    });

    return rhost::run(argc, argv);
}
