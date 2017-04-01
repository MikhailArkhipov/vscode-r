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
#include "stdafx.h"
#include "log.h"

#define SCOPE_WARDEN(NAME, ...)                \
    auto xx##NAME##xx = [&]() { __VA_ARGS__ }; \
    ::rhost::util::scope_warden<decltype(xx##NAME##xx)> NAME(xx##NAME##xx)

#define SCOPE_WARDEN_RESTORE(NAME) \
    auto NAME##_old_value = (NAME); \
    SCOPE_WARDEN(restore_##NAME, (NAME) = NAME##_old_value;)

namespace rhost {
    namespace util {
        template<typename F>
        class scope_warden {
        public:
            scope_warden(F& f) noexcept
                : _p(std::addressof(f))  {
            }

            void dismiss() noexcept {
                _p = nullptr;
            }

            void run() noexcept {
                if (_p) {
                    (*_p)();
                }
                dismiss();
            }

            ~scope_warden() noexcept {
                if (_p) {
                    try {
                        (*_p)();
                    } catch (...) {
                        std::terminate();
                    }
                }
            }

        private:
            F* _p;

            scope_warden(F&&) = delete;
            scope_warden(const scope_warden&) = delete;
            scope_warden& operator=(const scope_warden&) = delete;
        };


        struct SEXP_delete {
            typedef SEXP pointer;

            void operator() (SEXP sexp) {
                if (sexp) {
                    R_ReleaseObject(sexp);
                }
            }
        };

        class protected_sexp : public std::unique_ptr<SEXP, SEXP_delete> {
        public:
            using unique_ptr::unique_ptr;

            protected_sexp() {}

            protected_sexp(SEXP other) :
                unique_ptr(other)
            {
                R_PreserveObject(other);
            }

            protected_sexp(const protected_sexp& other) :
                protected_sexp(other.get()) {}

            protected_sexp(protected_sexp&& other) :
                unique_ptr(std::move(other)) {}

            using unique_ptr::operator=;

            protected_sexp& operator= (SEXP other) {
                protected_sexp o(other);
                swap(o);
                return *this;
            }

            protected_sexp& operator= (const protected_sexp& other) {
                return *this = other.get();
            }

            protected_sexp& operator= (protected_sexp&& other) {
                swap(other);
                return *this;
            }
        };

#ifdef _WIN32
        std::string Rchar_to_utf8(const char* buf, size_t len);

        inline std::string Rchar_to_utf8(const std::string& s) {
            return Rchar_to_utf8(s.data(), s.size());
        }
        std::string from_utf8(const std::string& u8s);
#else
        inline std::string Rchar_to_utf8(const char* buf, size_t len) {
            return std::string(buf, len);
        }

        inline std::string Rchar_to_utf8(const std::string& s){
            return s;
        }

        inline std::string from_utf8(const std::string& u8s){
            return u8s;
        }
#endif

        inline picojson::value to_utf8_json(const char* buf) {
            return buf ? picojson::value(Rchar_to_utf8(buf)) : picojson::value();
        }
        
        const std::locale& single_byte_locale();

        fs::path path_from_string_elt(SEXP string_elt);

        inline void append(picojson::array& msg) {
        }

        template<class Arg>
        inline void append(picojson::array& msg, Arg&& arg) {
            msg.push_back(picojson::value(std::forward<Arg>(arg)));
        }

        template<class Arg, class... Args>
        inline void append(picojson::array& msg, Arg&& arg, Args&&... args) {
            msg.push_back(picojson::value(std::forward<Arg>(arg)));
            append(msg, std::forward<Args>(args)...);
        }

        // A C++-friendly helper for Rf_error. Invoking Rf_error directly is not a good idea, because
        // it performs a longjmp, which will skip all C++ destructors when unwinding stack frames - so
        // the only way to perform it safely is right at the boundary. This helper function will catch
        // any exception type derived from std::exception, and invoke Rf_error with what() as message.
        template<class F>
        inline auto exceptions_to_errors(F f) -> decltype(f()) {
            try {
                return f();
            } catch (std::exception& ex) {
                Rf_error(ex.what());
            }
        }

        // Executes the callback in its own context, protecting the caller from
        // any call to Rf_error.
        // Do not use C++ objects that rely on their destructor running in the callback.
        // Returns true on if there are no errors, false otherwise (Rf_error was called).
        // Note that if there are any errors, they have been displayed already.
        // Meaning, there's no need to fetch the error message, turn it into a
        // std::exception, only to have exceptions_to_error report the same
        // error a second time!
        template <class FExecute>
        inline bool r_top_level_exec(FExecute protected_eval, const char* log_error_prefix = nullptr) {
            if (!R_ToplevelExec([](void* arg) { (*reinterpret_cast<FExecute*>(arg))(); }, &protected_eval)) {
                const char* err = R_curErrorBuf();
                if (log_error_prefix != nullptr) {
                    log::logf(log::log_verbosity::normal, "%s: error: %s\n", log_error_prefix, err);
                }
                return false;
            }
            return true;
        }

        class r_error : public std::runtime_error {
        public:
            explicit r_error(const std::string& msg)
                : std::runtime_error(msg) {
            }

            explicit r_error(const char* msg)
                : std::runtime_error(msg) {
            }
        };

        template <class FExecute>
        inline void errors_to_exceptions(FExecute protected_eval) {
            if (!r_top_level_exec(protected_eval)) {
                const char* err = R_curErrorBuf();
                throw r_error(err);
            }
        }

        inline std::string deparse(SEXP sexp) {
            return R_CHAR(STRING_ELT(Rf_deparse1line(sexp, R_FALSE), 0));
        }

        inline double ensure_fits_double(size_t size) {
            double dsize = static_cast<double>(size);
            size_t tsize = static_cast<size_t>(dsize);
            if (size != tsize) {
                log::fatal_error("Overflow during cast");
            }

            return dsize;
        }
    }
}
