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

namespace po = boost::program_options;

namespace rhost {
    namespace util {
#ifdef USE_BOOST_LOCALE
        const std::locale& single_byte_locale() {
            static auto locale = [] {
                boost::locale::generator gen;
                return gen.generate("");
            } ();
            return locale;
        }

        std::string to_utf8(const char* buf, size_t len) {
            std::locale loc = single_byte_locale();
            auto& codecvt_wchar = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
            std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert(&codecvt_wchar);
            auto ws = convert.from_bytes(buf, buf + len);

            std::wstring_convert<std::codecvt_utf8<wchar_t>> codecvt_utf8;
            return codecvt_utf8.to_bytes(ws);
        }

        std::string from_utf8(const std::string& u8s) {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> codecvt_utf8;
            auto ws = codecvt_utf8.from_bytes(u8s);

            std::locale loc = single_byte_locale();
            auto& codecvt_wchar = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
            std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> convert(&codecvt_wchar);
            return convert.to_bytes(ws);
        }
#endif
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
                nc += mbstowcs(wc, s, n);
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
                nc = mbstowcs(wc, s, n);
            }
            return nc;
        }

        std::string to_utf8(const char* buf, size_t len) {
            // Convert 8-bit characters to Unicode via Windows CP. This guarantees
            // if locale for non-Unicode programs is set correctly, user can type in
            // their language. This does NOT guarantee that all languages can be used
            // since R is not Unicode app. If host app is Unicode, it must perform
            // checks if text being passed here is convertable to Unicode.
            std::wstring ws;
            size_t cch = strlen(buf);
            if (cch > 0) {
                ws.resize(cch);
                RString2Unicode(&ws[0], (char*)buf, len);
            }
            // Now convert Unicode to UTF-8 for passing over to the host.
            std::wstring_convert<std::codecvt_utf8<wchar_t>> codecvt_utf8;
            return codecvt_utf8.to_bytes(ws.c_str());
        }

        std::string from_utf8(const std::string& u8s) {
            // Convert UTF-8 string that is coming from the host to Unicode.
            std::wstring_convert<std::codecvt_utf8<wchar_t>> codecvt_utf8;
            auto ws = codecvt_utf8.from_bytes(u8s);

            // Now convert to MBCS. Do it manually since WideCharToMultiByte
            // requires specific code page and fails if character
            // cannot be converted. Instead, we are going to encode 
            // unconvertable characters into "\uABCD" form. This allows
            // us to preserve characters written in non-default OS CP.
            std::string converted;
            // Max 8 bytes per character which should fit both UTF-8 and \uABCD
            converted.resize(8 * ws.length());
            size_t j = 0;
            for (size_t i = 0; i < ws.length(); i++)
            {
                char mbcharbuf[8];
                int mbcch = wctomb(mbcharbuf, ws[i]);
                if (mbcch == -1) {
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
    }
}

namespace boost {
    namespace asio {
        namespace ip {
            void validate(boost::any& v, const std::vector<std::string> values, boost::asio::ip::tcp::endpoint*, int) {
                po::validators::check_first_occurrence(v);
                auto& s = po::validators::get_single_string(values);

                std::string host;
                uint16_t port;
                size_t colon = s.find(':');
                if (colon == s.npos) {
                    host = s;
                    port = 5118;
                } else {
                    host = s.substr(0, colon);
                    auto port_str = s.substr(colon + 1);
                    try {
                        port = std::stoi(port_str);
                    } catch (std::invalid_argument&) {
                        throw po::validation_error(po::validation_error::invalid_option_value);
                    } catch (std::out_of_range&) {
                        throw po::validation_error(po::validation_error::invalid_option_value);
                    }
                }

                boost::asio::ip::address address;
                try {
                    address = boost::asio::ip::address::from_string(host);
                } catch (boost::system::system_error&) {
                    throw po::validation_error(po::validation_error::invalid_option_value);
                }

                v = boost::any(boost::asio::ip::tcp::endpoint(address, port));
            }
        }
    }
}

namespace websocketpp {
    void validate(boost::any& v, const std::vector<std::string> values, websocketpp::uri*, int) {
        po::validators::check_first_occurrence(v);
        auto& s = po::validators::get_single_string(values);

        websocketpp::uri uri(s);
        if (!uri.get_valid()) {
            throw po::validation_error(po::validation_error::invalid_option_value);
        }

        v = boost::any(uri);
    }
}
