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
#include "Rapi.h"
#include "util.h"

namespace rhost {
    namespace host {
        struct message {
            std::string id;
            std::string request_id; // not a response if empty
            std::string name;
            size_t blob_count;
            rhost::util::blob blob;
            picojson::array args;
        };

        class eval_cancel_error : std::exception {
        };

        std::future<void> wait_for_client(const boost::asio::ip::tcp::endpoint& endpoint);
        std::future<void> connect_to_server(const websocketpp::uri& uri);
        void register_callbacks(structRstart& rp);
        void terminate_if_closed();

        extern "C" void ShowMessage(const char* s);
        extern "C" int YesNoCancel(const char* s);
        extern "C" int OkCancel(const char* s);
        extern "C" int YesNo(const char* s);

        extern boost::signals2::signal<void()> callback_started;
        extern boost::signals2::signal<void()> readconsole_done;

        __declspec(noreturn) void propagate_cancellation();

        template <class F>
        auto with_cancellation(F body) -> decltype(body()) {
            terminate_if_closed();
            try {
                return body();
            } catch (const eval_cancel_error&) {
                propagate_cancellation();
            }
        }

        std::string send_notification(const char* name, const rhost::util::blob& blob, const picojson::array& args);
        std::string send_notification(const char* name, const picojson::array& args);

        template<class... Args>
        inline std::string send_notification(const char* name, const Args&... args) {
            picojson::array args_array;
            rhost::util::append(args_array, args...);
            return send_notification(name, args_array);
        }

        template<class... Args>
        inline std::string send_notification(const char* name, const rhost::util::blob& blob, const Args&... args) {
            picojson::array args_array;
            rhost::util::append(args_array, args...);
            return send_notification(name, blob, args_array);
        }

        message send_request_and_get_response(const char* name, const picojson::array& args);

        template<class... Args>
        inline message send_request_and_get_response(const char* name, const Args&... args) {
            picojson::array args_array;
            rhost::util::append(args_array, args...);
            return send_request_and_get_response(name, args_array);
        }

        const std::vector<byte> get_blob(long long id);
        void destroy_blob(long long id);
    }
}
