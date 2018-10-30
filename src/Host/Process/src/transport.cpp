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
#include "transport.h"

using namespace rhost::protocol;

namespace rhost {
    namespace transport {
        namespace {
            const int pipeSize = 4096;
#ifdef _WIN32
            typedef int ssize_t;
            const char devNull[] = "NUL";
#else
            const char devNull[] = "/dev/null";
#endif

            std::atomic<bool> connected;
            FILE *input, *output;
            std::mutex output_lock;

            void log_message(const char* prefix, message_id id, message_id request_id, const char* name, const char* json, const blobs::blob& blob) {
#ifdef TRACE_JSON
                std::ostringstream str;
                str << prefix << " #" << id << "# " << name;

                if (request_id > 0 && request_id < std::numeric_limits<message_id>::max()) {
                    str << " #" << request_id << "#";
                }

                str << " " << json;

                if (!blob.empty()) {
                    str << " <raw (" << blob.size() << " bytes)>";
                }

                log::logf(log::log_verbosity::traffic, "%s\n\n", str.str().c_str());
#endif
            }

            void disconnect() {
                if (connected.exchange(false)) {
                    disconnected();
                }
            }

            void receive_worker() {
                for (;;) {
                    boost::endian::little_uint32_buf_t msg_size;
                    if (fread(&msg_size, sizeof msg_size, 1, input) != 1) {
                        break;
                    }

                    std::string payload(msg_size.value(), '\0');
                    if (!payload.empty()) {
                        if (fread(&payload[0], payload.size(), 1, input) != 1) {
                            break;
                        }
                    }

                    auto msg = message::parse(payload);
                    log_message("==>", msg.id(), msg.request_id(), msg.name(), msg.json_text(), msg.blob());
                    message_received(msg);
                }

                disconnect();
            }

            void read_stream_to_message(int fdr, const std::string& message_name) {
                char line[pipeSize];
                size_t len = pipeSize;
                ssize_t nread;

                for(;;) {
#ifdef _WIN32
                    nread = _read(fdr, line, pipeSize);
#else
                    nread = read(fdr, line, pipeSize);
#endif

                    if (nread == -1) {
                        break;
                    }

                    if (nread > 0) {
                        picojson::array json;
                        json.push_back(picojson::value(std::string(line, nread)));
                        message msg(0, message_name, json, blobs::blob());
                        send_message(msg);
                    }
                }
            }

            void stdouterr_to_message() {
                int stdout_fd[2] = {}, stderr_fd[2] = {};
#ifdef _WIN32
                _pipe(stdout_fd, pipeSize, _O_TEXT);
                _pipe(stderr_fd, pipeSize, _O_TEXT);
#else
                pipe(stdout_fd);
                pipe(stderr_fd);
#endif

                dup2(stdout_fd[1], fileno(stdout));
                dup2(stderr_fd[1], fileno(stderr));

                std::thread([stdout_fd]() {
                    read_stream_to_message(stdout_fd[0], "!");
                }).detach();

                std::thread([stderr_fd]() {
                    read_stream_to_message(stderr_fd[0], "!!");
                }).detach();
            }
        }

        boost::signals2::signal<void(const protocol::message&)> message_received;

        boost::signals2::signal<void()> disconnected;

        void initialize() {
            assert(!input && !output);

#ifdef _WIN32
            setmode(fileno(stdin), _O_BINARY);
            setmode(fileno(stdout), _O_BINARY);
#endif

            // Duplicate and stash away handles for original stdin & stdout.
            input = fdopen(dup(fileno(stdin)), "rb");
            setvbuf(input, NULL, _IONBF, 0);
            output = fdopen(dup(fileno(stdout)), "wb");
            setvbuf(output, NULL, _IONBF, 0);

            // Redirect stdin null device, so that it will not interfere with the protocol.
            freopen(devNull, "rb", stdin);

            // This is needed to redirect any code that writes to stdout or stderr directly (instead of 
            // via R_WriteConsole) will be sent to client as a message.
            stdouterr_to_message();

            connected = true;
            std::thread(receive_worker).detach();
        }

        void send_message(const message& msg) {
            assert(output);

            log_message("<==", msg.id(), msg.request_id(), msg.name(), msg.json_text(), msg.blob());

            if (!connected) {
                return;
            }

            auto& payload = msg.payload();
            boost::endian::little_uint32_buf_t msg_size(static_cast<uint32_t>(payload.size()));

            std::lock_guard<std::mutex> lock(output_lock);
            if (fwrite(&msg_size, sizeof msg_size, 1, output) == 1) {
                if (fwrite(payload.data(), payload.size(), 1, output) == 1) {
                    fflush(output);
                    return;
                }
            }

            disconnect();
        }

        bool is_connected() {
            return connected;
        }
    }
}