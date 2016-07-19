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
#include "Rapi.h"
#include "msvcrt.h"
#include "log.h"
#include "util.h"
#include "host.h"
#include "json.h"
#include "exports.h"

using namespace rhost::log;
using namespace rhost::util;
using namespace rhost::host;
using namespace rhost::json;

namespace rhost {
    namespace r_util {
        class memory_connection {
        public:
            const SEXP connection_sexp;

            static memory_connection* create(int max_size = R_NaInt, int expected_size = R_NaInt) {
                Rconnection conn;
                auto conn_sexp = R_new_custom_connection("", "w", "memory_connection", &conn);
                return new memory_connection(conn, conn_sexp, max_size, expected_size);
            }

            static memory_connection* create(SEXP max_size, SEXP expected_size) {
                return create(Rf_asInteger(max_size), Rf_asInteger(expected_size));
            }

            static memory_connection* of_connection_sexp(SEXP conn_sexp) {
                auto it = _instances.find(conn_sexp);
                if (it == _instances.end()) {
                    throw std::exception("Connection is not a memory_connection");
                }
                return it->second;
            }

            Rboolean open() {
                return R_TRUE;
            }

            void close() {
                _data.clear();
            }

            int vfprintf(const char* format, va_list va) {
                int count;

                // Try with a reasonably large stack allocated buffer first.
                // Use vsnprintf from msvcrt, since that is what R normally uses, and there are differences.
                va_list va2;
                va_copy(va2, va);
                char buf[0x1000], *pbuf = buf;
                size_t bufsize = sizeof buf;
                count = msvcrt::vsnprintf(buf, bufsize, format, va2);
                va_end(va2);

                std::unique_ptr<char[]> buf_deleter;
                while (count < 0) {
                    // If it didn't fit in the buffer, heap-allocate a larger buffer.
                    bufsize *= 2;
                    if (bufsize >= 100 * 1024 * 1024) {
                        throw std::exception("Output is too long");
                    }

                    // If we run out of memory, new will throw std::bad_alloc, which will
                    // be translated to Rf_error at the boundary.
                    buf_deleter.reset(pbuf = new char[bufsize *= 2]);

                    va_copy(va2, va);
                    count = msvcrt::vsnprintf(pbuf, bufsize, format, va2);
                    va_end(va2);
                }

                if (!_eof_marker.empty()) {
                    if (char* eof = strstr(pbuf, _eof_marker.c_str())) {
                        *eof = '\0';
                        _seen_eof = true;
                    }
                }

                _data.append(pbuf);
                if (_max_size != R_NaInt && _data.size() > _max_size) {
                    _data.resize(_max_size - _overflow_suffix.size());
                    _data += _overflow_suffix;
                    _overflown = true;
                    throw std::exception("Connection size limit exceeded");
                }

                if (_seen_eof) {
                    throw std::exception("EOF marker encountered");
                }

                return count;
            }

            const std::string& overflow_suffix() const {
                return _overflow_suffix;
            }

            const std::string& overflow_suffix(const std::string& value) {
                if (value.size() > _max_size) {
                    throw std::invalid_argument("max_size is not large enough to fit overflow_suffix");
                }
                return _overflow_suffix = value;
            }

            const std::string& overflow_suffix(SEXP value) {
                if (Rf_isNull(value)) {
                    return overflow_suffix("");
                }

                protected_sexp value_char(Rf_asChar(value));
                return overflow_suffix(R_CHAR(value_char.get()));
            }

            const std::string& eof_marker() const {
                return _eof_marker;
            }

            const std::string& eof_marker(const std::string& value) {
                return _eof_marker = value;
            }

            const std::string& eof_marker(SEXP value) {
                if (Rf_isNull(value)) {
                    return eof_marker("");
                }

                protected_sexp value_char(Rf_asChar(value));
                return eof_marker(R_CHAR(value_char.get()));
            }

            const std::string& data() const {
                return _data;
            }

            SEXP data_sexp() const {
                return Rf_mkString(_data.c_str());
            }

            bool overflown() const {
                return _overflown;
            }

            SEXP overflown_sexp() const {
                return _overflown ? R_TrueValue : R_FalseValue;
            }

        private:
            static std::unordered_map<SEXP, memory_connection*> _instances;

            Rconnection _conn;
            int _max_size;
            std::string _data, _overflow_suffix, _eof_marker;
            bool _overflown, _seen_eof;

            memory_connection(Rconnection conn, SEXP conn_sexp, int max_size, int expected_size) :
                connection_sexp(conn_sexp),
                _conn(conn),
                _max_size(max_size),
                _overflown(false),
                _seen_eof(false) {
                if (expected_size > 0 && expected_size != R_NaInt) {
                    _data.reserve(expected_size);
                }

                _conn->private_ = this;
                _conn->isopen = R_TRUE;
                _conn->canwrite = R_TRUE;

                _conn->destroy = [](Rconnection conn) {
                    delete reinterpret_cast<memory_connection*>(conn->private_);
                };

                _conn->open = [](Rconnection conn) {
                    return exceptions_to_errors([&] {
                        return reinterpret_cast<memory_connection*>(conn->private_)->open();
                    });
                };

                _conn->close = [](Rconnection conn) {
                    return exceptions_to_errors([&] {
                        return reinterpret_cast<memory_connection*>(conn->private_)->close();
                    });
                };

                _conn->vfprintf = [](Rconnection conn, const char* format, va_list va) {
                    return exceptions_to_errors([&] {
                        return reinterpret_cast<memory_connection*>(conn->private_)->vfprintf(format, va);
                    });
                };

                _instances[conn_sexp] = this;
            }

            ~memory_connection() {
                _instances.erase(connection_sexp);
                _conn->private_ = nullptr;
            }
        };

        std::unordered_map<SEXP, memory_connection*> memory_connection::_instances;

        extern "C" SEXP unevaluated_promise(SEXP name, SEXP env) {
            if (!Rf_isEnvironment(env)) {
                Rf_error("env is not an environment");
            }
            if (!Rf_isString(name) || Rf_length(name) != 1) {
                Rf_error("name is not a single string");
            }

            SEXP value = Rf_findVar(Rf_installChar(STRING_ELT(name, 0)), env);
            if (TYPEOF(value) != PROMSXP || PRVALUE(value) != R_UnboundValue) {
                return R_NilValue;
            }

            return PRCODE(value);
        }

        extern "C" SEXP memory_connection_new(SEXP max_size, SEXP expected_size, SEXP overflow_suffix, SEXP eof_marker) {
            return exceptions_to_errors([&] {
                auto btc = memory_connection::create(max_size, expected_size);
                btc->overflow_suffix(overflow_suffix);
                btc->eof_marker(eof_marker);
                return btc->connection_sexp;
            });
        }

        extern "C" SEXP memory_connection_tochar(SEXP conn_sexp) {
            return exceptions_to_errors([&] {
                return memory_connection::of_connection_sexp(conn_sexp)->data_sexp();
            });
        }

        extern "C" SEXP memory_connection_overflown(SEXP conn_sexp) {
            return exceptions_to_errors([&] {
                return memory_connection::of_connection_sexp(conn_sexp)->overflown_sexp();
            });
        }

        picojson::array parse_args_sexp(SEXP args_sexp) {
            size_t len = Rf_length(args_sexp);

            picojson::value json = to_json(args_sexp);
            if (!json.is<picojson::array>()) {
                fatal_error("send_* requires argument that serializes to JSON array; got %s", json.serialize().c_str());
            }

            return json.get<picojson::array>();
        }

        extern "C" SEXP send_notification(SEXP name_sexp, SEXP args_sexp) {
            return with_cancellation([&] {
                protected_sexp name_char(Rf_asChar(name_sexp));
                const char* name = R_CHAR(name_char.get());

                auto args = parse_args_sexp(args_sexp);
                host::send_notification(name, args);

                return R_NilValue;
            });
        }

        extern "C" SEXP send_request_and_get_response(SEXP name_sexp, SEXP args_sexp) {
            return with_cancellation([&] {
                protected_sexp name_char(Rf_asChar(name_sexp));
                const char* name = R_CHAR(name_char.get());

                auto args = parse_args_sexp(args_sexp);
                auto response = host::send_request_and_get_response(name, args);

                protected_sexp response_args(Rf_allocVector3(STRSXP, response.args.size(), nullptr));
                for (size_t i = 0; i < response.args.size(); ++i) {
                    auto s = response.args[i].serialize();
                    SET_STRING_ELT(response_args.get(), i, Rf_mkChar(from_utf8(s).c_str()));
                }
                return response_args.release();
            });
        }

        CCODE do_parse;
        SEXP instrumentation_callback;

        extern "C" SEXP detoured_parse(SEXP call, SEXP op, SEXP args, SEXP env) {
            // Because this invokes do_parse, which will use Rf_error to report errors anyway,
            // there's no point in using exceptions_to_errors. Therefore, the body of this function
            // must not rely on destructors being run during unwinding - so no STL classes, nor
            // protected_sexp.

            assert(do_parse);
            SEXP result = Rf_protect(do_parse(call, op, args, env));

            if (instrumentation_callback != R_NilValue) {
                static bool in_callback = false;
                if (!in_callback) {
                    in_callback = true;

                    SEXP call = Rf_protect(Rf_allocList(2));
                    SET_TYPEOF(call, LANGSXP);
                    SETCAR(call, instrumentation_callback);

                    SEXP arg = CDR(call);
                    SETCAR(arg, result);

                    // Reset debug flag to avoid eval entering Browse mode.
                    int rdebug = RDEBUG(R_GlobalEnv);
                    SET_RDEBUG(R_GlobalEnv, 0);

                    rhost::util::r_top_level_exec([&] {
                        SEXP instrumented = Rf_eval(call, R_GlobalEnv);
                        if (instrumented != R_NilValue) {
                            result = instrumented;
                        }
                    }, __FUNCTION__);

                    SET_RDEBUG(R_GlobalEnv, rdebug);

                    Rf_unprotect(1);
                    in_callback = false;
                }
            }

            Rf_unprotect(1);
            return result;
        }

        extern "C" SEXP set_instrumentation_callback(SEXP func) {
            return exceptions_to_errors([&] {
                if (!do_parse) {
                    FUNTAB* funtab = R_FunTab;
                    for (;;) {
                        if (!funtab->name) {
                            throw std::exception("R_FunTab does not contain an entry for 'parse'.");
                        } else if (strcmp(funtab->name, "parse") == 0) {
                            break;
                        } else {
                            ++funtab;
                        }
                    }

                    do_parse = funtab->cfun;
                    funtab->cfun = detoured_parse;
                }

                instrumentation_callback = func;
                return R_NilValue;
            });
        }

        extern "C" SEXP is_rdebug(SEXP obj) {
            return RDEBUG(obj) ? R_TrueValue : R_FalseValue;
        }

        extern "C" SEXP set_rdebug(SEXP obj, SEXP debug) {
            SET_RDEBUG(obj, Rf_asLogical(debug));
            return R_NilValue;
        }

        extern "C" SEXP browser_set_debug(SEXP n_sexp, SEXP skip_toplevel_sexp) {
            int n = Rf_asInteger(n_sexp);
            if (n < 1) {
                Rf_error("Number of contexts to skip must be positive.");
            }

            int skip_toplevel = Rf_asInteger(skip_toplevel_sexp);
            if (skip_toplevel < 0) {
                Rf_error("Number of top-level contexts to skip must be non-negative.");
            }

            // Skip the requisite number of top-level contexts first.
            auto ctx = R_GlobalContext;
            while (ctx && skip_toplevel) {
                if (ctx->callflag == CTXT_TOPLEVEL) {
                    --skip_toplevel;
                }
                ctx = ctx->nextcontext;
            }

            // Find the closest browser context, if any.
            while (ctx && ctx->callflag != CTXT_TOPLEVEL) {
                if (ctx->callflag == CTXT_BROWSER) {
                    break;
                }
                ctx = ctx->nextcontext;
            }
            if (!ctx || ctx->callflag != CTXT_BROWSER) {
                Rf_error("Step out can only be performed in a browser context.");
            }

            // Skip the first n function contexts.
            while (ctx && ctx->callflag != CTXT_TOPLEVEL && n > 0) {
                if (ctx->callflag & CTXT_FUNCTION) {
                    n--;
                }
                ctx = ctx->nextcontext;
            }

            if (!ctx || ctx->callflag == CTXT_TOPLEVEL) {
                Rf_error("Nowhere to step out to.");
            }

            if ((ctx->callflag & CTXT_FUNCTION) || (ctx->callflag & CTXT_LOOP)) {
                SET_RDEBUG(ctx->cloenv, 1);
            } else {
                Rf_error("Cannot step out into the designated context - unsupported context type %d", ctx->callflag);
            }

            return R_NilValue;
        }

        extern "C" SEXP toJSON(SEXP obj) {
            SEXP json = Rf_mkCharCE(exceptions_to_errors([&] { return to_json(obj).serialize(); }).c_str(), CE_UTF8);
            Rf_protect(json);
            SEXP result = Rf_allocVector(STRSXP, 1);
            SET_STRING_ELT(result, 0, json);
            Rf_unprotect(1);
            return result;
        }

        extern "C" SEXP create_blob(SEXP obj) {
            int type = TYPEOF(obj);
            size_t length = Rf_length(obj);

            if (type != RAWSXP || length == 0) {
                Rf_error("Object must be a RAW vector of length > 0.");
            }

            Rbyte* data = RAW(obj);
            rhost::util::blob blob;
            blob.push_back(rhost::util::blob_slice(data, data + length));
            int id = rhost::host::create_blob(blob);
            return Rf_ScalarInteger((int)id);
        }

        extern "C" SEXP get_blob(SEXP id) {
            int blob_id = Rf_asInteger(id);
            const std::vector<byte> data = rhost::host::get_blob_as_single_slice(blob_id);

            if (data.size() == 0) {
                return R_NilValue;
            }

            SEXP rawVector = nullptr;
            Rf_protect(rawVector = Rf_allocVector(RAWSXP, data.size()));
            Rbyte* dest = RAW(rawVector);
            memcpy_s(dest, data.size(), data.data(), data.size());
            Rf_unprotect(1);
            return rawVector;
        }

        extern "C" SEXP destroy_blob(SEXP id) {
            int blob_id = Rf_asInteger(id);
            rhost::host::destroy_blob(blob_id);
            return R_NilValue;
        }

        R_CallMethodDef call_methods[] = {
            { "Microsoft.R.Host::Call.unevaluated_promise", (DL_FUNC)unevaluated_promise, 2 },
            { "Microsoft.R.Host::Call.memory_connection", (DL_FUNC)memory_connection_new, 4 },
            { "Microsoft.R.Host::Call.memory_connection_tochar", (DL_FUNC)memory_connection_tochar, 1 },
            { "Microsoft.R.Host::Call.memory_connection_overflown", (DL_FUNC)memory_connection_overflown, 1 },
            { "Microsoft.R.Host::Call.send_notification", (DL_FUNC)send_notification, 2 },
            { "Microsoft.R.Host::Call.send_request_and_get_response", (DL_FUNC)send_request_and_get_response, 2 },
            { "Microsoft.R.Host::Call.set_instrumentation_callback", (DL_FUNC)set_instrumentation_callback, 1 },
            { "Microsoft.R.Host::Call.is_rdebug", (DL_FUNC)is_rdebug, 1 },
            { "Microsoft.R.Host::Call.set_rdebug", (DL_FUNC)set_rdebug, 2 },
            { "Microsoft.R.Host::Call.browser_set_debug", (DL_FUNC)browser_set_debug, 2 },
            { "Microsoft.R.Host::Call.toJSON", (DL_FUNC)toJSON, 1 },
            { "Microsoft.R.Host::Call.create_blob", (DL_FUNC)create_blob, 1 },
            { "Microsoft.R.Host::Call.get_blob", (DL_FUNC)get_blob, 1 },
            { "Microsoft.R.Host::Call.destroy_blob", (DL_FUNC)destroy_blob, 1 },
            { }
        };

        void init(DllInfo *dll) {
            rhost::exports::add_call_methods(call_methods);
        }
    }
}

