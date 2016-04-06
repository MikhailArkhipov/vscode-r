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

#include "json.h"
#include "log.h"

using namespace rhost::util;
using namespace rhost::log;
namespace js = picojson;

namespace rhost {
    namespace json {
        namespace {
            void json_error(SEXP sexp, const char* format, ...) {
                SEXP repr_sexp = STRING_ELT(Rf_deparse1line(sexp, R_FALSE), 0);
                Rf_protect(repr_sexp);
                const char* repr = R_CHAR(repr_sexp);

                char buf[0x10000] = {};
                snprintf(buf, sizeof buf, "JSON serialization failed for input:\n\n%s\n\n", repr);

                size_t len = strlen(buf);
                va_list va;
                va_start(va, format);
                vsnprintf(buf + len, sizeof buf - len, format, va);
                va_end(format);

                Rf_error("%s", buf);
            }

            void at_most_one(SEXP sexp) {
                if (Rf_length(sexp) != 1) {
                    json_error(sexp, "Vector must have 0 or 1 elements.");
                }
            }

            void list_to_array(SEXP sexp, js::array& result) {
                R_len_t count = Rf_length(sexp);
                result.reserve(count);

                for (R_len_t i = 0; i < count; ++i) {
                    result.push_back(js::value());
                    auto& elem = result.back();

                    SEXP elem_sexp = VECTOR_ELT(sexp, i);
                    if (!to_json(elem_sexp, elem)) {
                        result.pop_back();
                    }
                }
            }

            void list_to_object(SEXP sexp, SEXP names, js::object& result) {
                R_len_t count = Rf_length(sexp);
                if (Rf_length(names) != count) {
                    json_error(sexp, "All elements in list must be named, but there are fewer names than elements.");
                }

                for (R_len_t i = 0; i < count; ++i) {
                    SEXP name_sexp = STRING_ELT(names, i);
                    if (name_sexp == R_NaString) {
                        json_error(sexp, "All elements in list must be named, but [[%d]] is not.", i + 1);
                    }

                    const char* name = R_CHAR(name_sexp);

                    auto insert_result = result.insert(std::make_pair(name, js::value()));
                    if (!insert_result.second) {
                        json_error(sexp, "Duplicate name '%s' in list.", name);
                    }

                    auto iter = insert_result.first;
                    auto& elem = iter->second;

                    SEXP elem_sexp = VECTOR_ELT(sexp, i);
                    if (!to_json(elem_sexp, elem)) {
                        result.erase(iter);
                    }
                }
            }

            void env_to_object(SEXP sexp, js::object& result) {
                SEXP names = R_lsInternal3(sexp, R_TRUE, R_FALSE);
                R_len_t count = Rf_length(names);

                for (R_len_t i = 0; i < count; ++i) {
                    SEXP name_sexp = STRING_ELT(names, i);
                    const char* name = R_CHAR(name_sexp);

                    auto insert_result = result.insert(std::make_pair(name, js::value()));
                    if (!insert_result.second) {
                        json_error(sexp, "Duplicate name '%s' in environment.", name);
                    }

                    auto iter = insert_result.first;
                    auto& elem = iter->second;

                    SEXP sym_sexp = Rf_installChar(name_sexp);
                    SEXP elem_sexp = Rf_findVar(sym_sexp, sexp);
                    if (!to_json(elem_sexp, elem)) {
                        result.erase(iter);
                    }
                }
            }
        }

        bool to_json(SEXP sexp, js::value& result) {
            int type = TYPEOF(sexp);

            switch (type) {
            case NILSXP:
                result = js::value();
                return true;

            case VECSXP: {
                SEXP names = Rf_getAttrib(sexp, R_NamesSymbol);
                if (Rf_isNull(names)) {
                    result = js::value(js::array());
                    list_to_array(sexp, result.get<js::array>());
                } else {
                    result = js::value(js::object());
                    list_to_object(sexp, names, result.get<js::object>());
                }
                return true;
            }

            case ENVSXP:
                result = js::value(js::object());
                env_to_object(sexp, result.get<js::object>());
                return true;
            }

            if (Rf_length(sexp) == 0) {
                result = js::value();
                return true;
            }

            switch (type) {
            case LGLSXP: {
                at_most_one(sexp);
                int x = *LOGICAL(sexp);
                result = x == R_NaInt ? js::value() : js::value(x != 0);
                break;
            }

            case INTSXP: {
                at_most_one(sexp);
                int x = *INTEGER(sexp);
                result = x == R_NaInt ? js::value() : js::value(static_cast<double>(x));
                break;
            }

            case REALSXP: {
                at_most_one(sexp);
                double x = *REAL(sexp);
                if (R_IsNA(x)) {
                    result = js::value();
                } else {
                    if (isinf(x) || isnan(x)) {
                        json_error(sexp, "+Inf, -Inf and NaN cannot be serialized.");
                    }
                    result = js::value(x);
                }
                break;
            }

            case STRSXP: {
                at_most_one(sexp);
                SEXP x = STRING_ELT(sexp, 0);
                if (x == R_NaString) {
                    result = js::value();
                } else {
                    const void* vmax = vmaxget();
                    const char* s = Rf_translateCharUTF8(x);
                    result = s ? js::value(s) : js::value();
                    vmaxset(vmax);
                }
                break;
            }

            default:
                json_error(sexp, "Unsupported type - must be one of: NULL; logical, integer, real, character vector; list; environment.");
            }

            return result != js::value();
        }
    }
}
