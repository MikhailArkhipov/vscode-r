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

#include "blobs.h"
#include "log.h"

using namespace rhost::util;
using namespace rhost::log;
namespace js = picojson;

namespace rhost {
    namespace raw {
        namespace {
            void blob_error(SEXP sexp, const char* format, ...) {
                SEXP repr_sexp = STRING_ELT(Rf_deparse1line(sexp, R_FALSE), 0);
                Rf_protect(repr_sexp);
                const char* repr = R_CHAR(repr_sexp);

                char buf[0x10000] = {};
                snprintf(buf, sizeof buf, "BLOB serialization failed for input:\n\n%s\n\n", repr);

                size_t len = strlen(buf);
                va_list va;
                va_start(va, format);
                vsnprintf(buf + len, sizeof buf - len, format, va);
                va_end(format);

                Rf_error("%s", buf);
            }

            bool to_blob_internal(SEXP sexp, rhost::util::blob& blob) {
                int type = TYPEOF(sexp);
                size_t length = Rf_length(sexp);

                if ((type == NILSXP) || (length == 0)) {
                    return true;
                }

                if (type == RAWSXP) {
                    Rbyte* data = RAW(sexp);
                    blob.push_back(rhost::util::blob_slice(data, data + length));
                    return true;
                }

                blob_error(sexp, "Unsupported type for blob - must be one of: NULL; RAW.");
                return false;
            }
        }

        bool to_blob(SEXP sexp, rhost::util::blob& blob) {
            bool result = false;
            rhost::util::errors_to_exceptions([&] {result = to_blob_internal(sexp, blob); });
            return result;
        }
    }
}
