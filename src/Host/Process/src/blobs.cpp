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
#include "host.h"

using namespace rhost::util;
using namespace rhost::log;
namespace js = picojson;

namespace rhost {
    namespace blobs {
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
                va_end(va);

                Rf_error("%s", buf);
            }

            bool to_blob_internal(SEXP sexp, std::vector<char>& blob) {
                int type = TYPEOF(sexp);
                size_t length = Rf_length(sexp);

                if (type == NILSXP) {
                    return false;
                }

                if (type == RAWSXP) {
                    Rbyte* data = RAW(sexp);
                    blob.assign(data, data + length);
                    return true;
                }

                blob_error(sexp, "Unsupported type for blob - must be one of: NULL; RAW.");
                return false;
            }
        }

        bool to_blob(SEXP sexp, std::vector<char>& blob) {
            bool result = false;
            rhost::util::errors_to_exceptions([&] {result = to_blob_internal(sexp, blob); });
            return result;
        }

        void append_from_file(blob& blob, const char* path) {
            FILE* fp = nullptr;
            SCOPE_WARDEN(close_file, {
                if (fp) {
                    fclose(fp);
                }
            });

            fp = fopen(path, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                size_t len = ftell(fp);

                if (len) {
                    size_t offset = blob.size();
                    blob.resize(offset + len);

                    fseek(fp, 0, SEEK_SET);
                    size_t read = fread(&blob[offset], 1, len, fp);
                    if (read != len) {
                        throw std::runtime_error("Error reading file");
                    }
                }
            }
        }

        void save_to_file(blob_id id, fs::path& file_path) {
            auto data = rhost::host::get_blob(id);

            fs::path parent = file_path.parent_path();
            if (!fs::exists(parent)) {
                fs::create_directories(parent);
            }

            FILE *f = std::fopen(file_path.make_preferred().string().c_str(), "wb");
            if (!f) {
                throw std::runtime_error("Error saving blob to file.");
            }

            SCOPE_WARDEN(blob_file, {
                std::fclose(f);
            });

            size_t sz = std::fwrite(data.data(), sizeof(char), data.size(), f);
            if (sz != data.size()) {
                throw std::runtime_error("Error while writing blob to file.");
            }
        }
    }
}
