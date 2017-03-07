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
#include "util.h"
#include "msvcrt.h"

namespace po = boost::program_options;

namespace rhost {
    namespace util {
        // Taken from R gnuwin32\console.c. Converts string that is partially
        // ANSI and partially UTF-8 to Unicode. UTF-8 fragment is bounded by
        // 02 FF FE at the start and by 03 FF FE at the end.
        size_t RString2Unicode(wchar_t *wc, char *s, size_t n) {
            char UTF8in[4] = "\002\377\376";
            char UTF8out[4] = "\003\377\376";

            size_t nc = 0;
            char *pb, *pe;

            if ((pb = strchr(s, UTF8in[0])) && *(pb + 1) == UTF8in[1] && *(pb + 2) == UTF8in[2]) {
                *pb = '\0';

                nc += RHOST_mbstowcs(wc, s, n);
                pb += 3; pe = pb;

                while (*pe &&
                    !((pe = strchr(pe, UTF8out[0])) && *(pe + 1) == UTF8out[1] &&
                        *(pe + 2) == UTF8out[2])) {
                    pe++;
                }
                if (!*pe) {
                    return nc;
                }

                *pe = '\0';
                /* convert string starting at pb from UTF-8 */
                nc += Rf_utf8towcs(wc + nc, pb, (pe - pb));
                pe += 3;
                nc += RString2Unicode(wc + nc, pe, n - nc);
            } else {
                nc = RHOST_mbstowcs(wc, s, n);
            }
            return nc;
        }

        std::string Rchar_to_utf8(const char* buf, size_t len) {
            // Convert 8-bit characters to Unicode via Windows CP. This guarantees
            // if locale for non-Unicode programs is set correctly, user can type in
            // their language. This does NOT guarantee that all languages can be used
            // since R is not Unicode app. If host app is Unicode, it must perform
            // checks if text being passed here is convertible to Unicode.
            std::wstring ws;
            size_t cch = strlen(buf);
            if (cch > 0) {
                ws.resize(cch);
                RString2Unicode(&ws[0], (char*)buf, len);
            }
            // Now convert Unicode to UTF-8 for passing over to the host.
            return boost::locale::conv::utf_to_utf<char>(ws);
        }

        std::string from_utf8(const std::string& u8s) {
            // Convert UTF-8 string that is coming from the host to Unicode.
            auto ws = boost::locale::conv::utf_to_utf<wchar_t>(u8s);

            // Now convert to MBCS. Do it manually since WideCharToMultiByte
            // requires specific code page and fails if character
            // cannot be converted. Instead, we are going to encode 
            // unconvertible characters into "\uABCD" form. This allows
            // us to preserve characters written in non-default OS CP.
            // Max 8 bytes per character which should fit both UTF-8 and \uABCD
            std::string converted(8 * ws.length(), '\0');
            size_t j = 0;
            for (size_t i = 0; i < ws.length(); i++)
            {
                char mbcharbuf[8];
                int mbcch = RHOST_wctomb(mbcharbuf, ws[i]);

                bool escape;
                if (mbcch == -1) {
                    escape = true;
                } else {
                    // wctomb will try to do an inexact mapping if exact is unavailable - for example,
                    // if the native encoding is Latin-1, and input is "Δ" (Greek delta), it will be
                    // converted to Latin "D", and success will be reported. For such cases, we want to
                    // do "\u..." escaping instead, to preserve the original letter exactly. To detect
                    // that, convert the result back, and see if it matches the original. 
                    wchar_t wc;
                    escape = RHOST_mbtowc(&wc, mbcharbuf, mbcch) == -1 || wc != ws[i];
                }

                if (escape) {
                    // Character could not be converted, encode it
                    sprintf(&converted[j], "\\u%04x", ws[i]);
                    j += 6;
                } else {
                    memcpy(&converted[j], mbcharbuf, mbcch);
                    j += mbcch;
                }
            }
            converted[j] = '\0';
            converted.resize(j);
            return converted;
        }

        fs::path path_from_string_elt(SEXP string_elt) {
#ifdef _WIN32
            return fs::path(Rf_wtransChar(string_elt));
#else
            return fs::path(Rf_translateCharUTF8(string_elt));
#endif
        }
    }
}
