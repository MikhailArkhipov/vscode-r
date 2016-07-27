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

#include "message.h"
#include "log.h"

namespace rhost {
    namespace protocol {
        namespace {
            void log_payload(const std::string& payload) {
                std::ostringstream str;
                str << "\n\n<message (" << payload.size() << " bytes):\n";
                for (unsigned char ch : payload) {
                    str << std::hex << std::setw(2) << std::setfill('0') << unsigned(ch) << ' ';
                }
                str << ">";

                log::logf("%s\n\n", str.str().c_str());
                log::flush_log();
            }
        }

        message message::parse(std::string&& payload) {
            using namespace boost::endian;
            using namespace rhost::log;

            if (payload.size() < sizeof(message_repr)) {
                log_payload(payload);
                fatal_error("Malformed message header - missing IDs");
            }
            auto& repr = *reinterpret_cast<const message_repr*>(&payload[0]);

            const char* start = &payload.front();
            const char* end = &payload.back() + 1;
            const char* p = repr.data;

            if (p >= end) {
                log_payload(payload);
                fatal_error("Malformed message header - missing name");
            }
            const char* name = p;
            p = reinterpret_cast<const char*>(memchr(p, '\0', end - p));
            if (!p) {
                log_payload(payload);
                fatal_error("Malformed message header - missing name terminator");
            }

            if (++p >= end) {
                log_payload(payload);
                fatal_error("Malformed message body - missing JSON");
            }
            const char* json = p;
            p = reinterpret_cast<const char*>(memchr(p, '\0', end - p));
            if (!p) {
                log_payload(payload);
                fatal_error("Malformed message body - missing JSON terminator");
            }

            const char* blob = ++p;

            return message(repr.id.value(), repr.request_id.value(), std::move(payload), name - start, json - start, blob - start);
        }

        picojson::array message::json() const {
            picojson::value result;

            std::string err = picojson::parse(result, json_text());
            if (!err.empty()) {
                log_payload(_payload);
                log::fatal_error("Malformed JSON payload - %s: %s", err.c_str(), _json);
            }
            if (!result.is<picojson::array>()) {
                log_payload(_payload);
                log::fatal_error("JSON payload must be an array, but got %s", _json);
            }

            return result.get<picojson::array>();
        }
    }
}
