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
#include "blobs.h"

namespace rhost {
    namespace protocol {
        typedef uint64_t message_id;

        struct message_repr {
            boost::endian::little_uint64_buf_t id, request_id;
            char* data;
        };

        class message {
        public:
            static const message_id request_marker = std::numeric_limits<message_id>::max();

            message() :
                _id(0), _request_id(0), _name(0), _json(0), _blob(0) {
            }

            message(message_id request_id, const std::string& name, const picojson::array& json, const std::vector<char>& blob) :
                message(request_id, name, picojson::value(json).serialize(), blob) {
            }

            message(message_id request_id, const std::string& name, const std::string& json, const std::vector<char>& blob);

            static message parse(std::string&& payload);

            static message parse(const std::string& payload) {
                return parse(std::string(payload));
            }

            const std::string& payload() const {
                return _payload;
            }

            message_id id() const {
                return _id;
            }

            message_id request_id() const {
                return _request_id;
            }

            bool is_notification() const {
                return request_id() == 0;
            }

            bool is_request() const {
                return request_id() == request_marker;
            }

            bool is_response() const {
                return !is_notification() && !is_request();
            }

            const char* name() const {
                return &_payload[_name];
            }

            const char* blob_data() const {
                return &_payload[_blob];
            }

            size_t blob_size() const {
                return _payload.size() - _blob;
            }

            blobs::blob blob() const {
                return blobs::blob(blob_data(), blob_data() + blob_size());
            }

            const char* json_text() const {
                return &_payload[_json];
            }

            picojson::array json() const;

        private:

            message_id _id;
            message_id _request_id;
            std::string _payload;

            // The following all point inside _payload. _name and _json are guaranteed
            // to be null-terminated. Blob spans from from _blob to end of _payload.
            ptrdiff_t _name;
            ptrdiff_t _json;
            ptrdiff_t _blob;

            message(message_id id, message_id request_id, std::string&& payload, ptrdiff_t name, ptrdiff_t json, ptrdiff_t blob) :
                _id(id), _request_id(request_id), _payload(std::move(payload)),
                _name(name), _json(json), _blob(blob) {
            }
        };
    }
}
