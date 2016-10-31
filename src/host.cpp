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

#include "host.h"
#include "log.h"
#include "msvcrt.h"
#include "eval.h"
#include "util.h"
#include "json.h"
#include "blobs.h"
#include "transport.h"

using namespace std::literals;
using namespace boost::endian;
using namespace rhost::log;
using namespace rhost::util;
using namespace rhost::eval;
using namespace rhost::json;
using namespace rhost::blobs;
using namespace rhost::protocol;

namespace rhost {
    namespace host {
        boost::signals2::signal<void()> callback_started;
        boost::signals2::signal<void()> readconsole_done;
        boost::signals2::signal<void()> disconnected;

        fs::path rdata;
        std::atomic<bool> shutdown_requested = false;

        bool is_r_ready = false;
        std::mutex is_r_ready_lock;
        std::condition_variable is_r_ready_cond;

        std::mutex idle_timer_lock;
        std::chrono::steady_clock::time_point idling_since;

        DWORD main_thread_id;
        std::atomic<bool> is_waiting_for_wm = false;
        bool allow_callbacks = true, allow_intr_in_CallBack = true;

        // Specifies whether the host is currently expecting a response message to some earlier request that it had sent.
        // The host can always receive eval and cancellation requests, and they aren't considered responses. If any other
        // message is received, state must be RESPONSE_EXPECTED; it is then changed to RESPONSE_RECEIVED, and message is
        // saved in response. If state was not RESPONSE_EXPECTED when message was received, it is considered a fatal error.
        enum response_state_t { RESPONSE_UNEXPECTED, RESPONSE_EXPECTED, RESPONSE_RECEIVED } response_state;
        // Most recent message received in response to RESPONSE_EXPECTED.
        message response;
        std::mutex response_mutex;

        // Eval requests queued for execution. When eval begins executing, it is removed from this queue, and placed onto eval_stack.
        std::queue<message> eval_requests;
        std::mutex eval_requests_mutex;

        struct eval_info {
            message_id id;
            bool is_cancelable;

            eval_info(message_id id, bool is_cancelable)
                : id(id), is_cancelable(is_cancelable) {
            }
        };

        // Keeps track of evals that are currently being executed (as opposed to queued - that is tracked by eval_requests).
        // The first item is always dummy eval representing evaluation of input on the last ReadConsole prompt. Following it
        // is the current real top-level eval, and then any nested evals are appended at the end, in order of their nesting. 
        // For example, if an eval request for "x" came in (and it was re-entrant, thus permitting nested evals); and then,
        // while it was executing, an eval request for "y" came in; and then while that was executing, "z" came in, then the
        // stack will look like this:
        //
        //   <dummy> x y z
        // 
        // When cancellation for any eval on the stack is requested, all evals that follow it on the stack are also canceled,
        // since execution will not return to the eval unless all nested evals are terminated. When cancellation of all evals
        // is requested, it is implemented as cancellation of the topmost dummy eval. 
        std::vector<eval_info> eval_stack({ eval_info(0, true) });
        bool canceling_eval; // whether we're currently processing a cancellation request by unwinding the eval stack
        message_id eval_cancel_target; // ID of the eval on the stack that is the cancellation target
        std::mutex eval_stack_mutex;

        blob_id next_blob_id = 1;
        std::map<blob_id, blob> blobs;
        std::mutex blobs_mutex;

        void log_message(const char* prefix, message_id id, message_id request_id, const std::string& name, const picojson::array& args, const blob& blob) {
#ifdef TRACE_JSON
            std::ostringstream str;
            str << prefix << " #" << id << "# " << name;

            if (request_id > 0 && request_id < std::numeric_limits<message_id>::max()) {
                str << " #" << request_id << "#";
            }

            str << " " << picojson::value(args).serialize();

            if (!blob.empty()) {
                str << " <raw (" << blob.size() << " bytes)>";
            }

            logf(log_verbosity::traffic, "%s\n\n", str.str().c_str());
#endif
        }

        void reset_idle_timer() {
            std::lock_guard<std::mutex> lock(idle_timer_lock);
            idling_since = std::chrono::steady_clock::now();
        }

        message_id send_notification(const std::string& name, const picojson::array& args, const blob& blob) {
            assert(name[0] == '!');

            reset_idle_timer();

            message msg(0, name, args, blob);
            transport::send_message(msg);
            return msg.id();
        }

        template<class... Args>
        message_id respond_to_message(const message& request, const blob& blob, Args... args) {
            assert(request.name()[0] == '?');

            reset_idle_timer();

            picojson::array json;
            rhost::util::append(json, args...);

            std::string name = request.name();
            name[0] = ':';

            message msg(request.id(), name, json, blob);
            transport::send_message(msg);
            return msg.id();
        }

        template<class... Args>
        message_id respond_to_message(const message& request, Args... args) {
            static const blob empty;
            return respond_to_message(request, empty, args...);
        }

        bool query_interrupt() {
            std::lock_guard<std::mutex> lock(eval_stack_mutex);
            if (!canceling_eval) {
                return false;
            }

            // If there is a non-cancellable eval on the stack, do not allow to interrupt it or anything nested.
            auto it = std::find_if(eval_stack.begin(), eval_stack.end(), [](auto ei) { return !ei.is_cancelable; });
            return it == eval_stack.end();
        }

        // Unblock any pending with_response call that is waiting in a message loop.
        void unblock_message_loop() {
            // Because PeekMessage can dispatch messages that were sent, which may in turn result 
            // in nested evaluation of R code and nested message loops, sending a single WM_NULL
            // may not be sufficient, so keep sending them until the waiting flag is cleared - 
            // because WM_NULL is no-op, posting extra ones is harmless.
            // However, we need to pause and give the other thread some time to process, otherwise
            // we can flood its WM queue faster than it can process it, and it might never stop
            // pumping events and return to PeekMessage.
            auto delay = 10ms;
            for (; is_waiting_for_wm; std::this_thread::sleep_for(delay)) {
                PostThreadMessage(main_thread_id, WM_NULL, 0, 0);

                // Further guard against overflowing the queue by posting to it too aggressively.
                // If previous wait didn't help, give it a little more time to process next message,
                // up to a reasonable limit.
                if (delay < 5000ms) {
                    delay *= 2;
                }
            }
        }

        void terminate_if_disconnected() {
            // terminate invokes R_Suicide, which may invoke WriteConsole and/or ShowMessage, which will
            // then call terminate again, so we need to prevent infinite recursion here.
            static bool is_terminating;
            if (is_terminating) {
                return;
            }

            if (!transport::is_connected()) {
                is_terminating = true;
                disconnected();
                terminate("Lost connection to client.");
            }
        }

        void shutdown_if_requested() {
            terminate_if_disconnected();

            if (!shutdown_requested) {
                return;
            }

            // Make sure we don't try handle the pending shutdown request more than once.
            static std::atomic<bool> is_shutting_down;
            bool expected = false;
            if (!is_shutting_down.compare_exchange_strong(expected, true)) {
                return;
            }

            if (!rdata.empty()) {
                std::string s = rdata.string();
                logf(log_verbosity::minimal, "Saving workspace to %s...\n", s.c_str());

                bool saved = r_top_level_exec([&] {
                    R_SaveGlobalEnvToFile(s.c_str());
                });

                logf(log_verbosity::minimal, saved ? "Workspace saved successfully.\n" : "Failed to save workspace.\n");
                send_notification("!End", saved);
            }

            terminate("Shutting down by request.");
        }

        void request_shutdown(bool save_rdata) {
            if (!save_rdata) {
                rdata.clear();
            }

            shutdown_requested = true;

            std::thread([] {
                std::this_thread::sleep_for(1min);
                terminate("Timed out while waiting for graceful shutdown to complete; terminating process.");
            }).detach();

            unblock_message_loop();
        }

        void request_shutdown(const message& msg) {
            assert(!strcmp(msg.name(), "!Shutdown"));

            if (shutdown_requested) {
                return;
            }

            auto json = msg.json();
            if (!json[0].is<bool>()) {
                fatal_error("Invalid evaluation request: 1 boolean argument expected");
            }
            auto save_rdata = json[0].get<bool>();

            request_shutdown(save_rdata);
        }

        void idle_timer_thread(std::chrono::seconds idle_timeout) {
            for (;;) {
                std::chrono::steady_clock::time_point idling_since;
                {
                    std::lock_guard<std::mutex> lock(idle_timer_lock);
                    idling_since = host::idling_since;
                }

                auto delta = std::chrono::steady_clock::now() - idling_since;
                if (delta > idle_timeout) {
                    request_shutdown(true);
                    break;
                }

                std::this_thread::sleep_until(idling_since + idle_timeout);
            }
        }

        void create_blob(const message& msg) {
            assert(!strcmp(msg.name(), "?CreateBlob"));
            
            std::lock_guard<std::mutex> lock(blobs_mutex);
            blobs::blob_id id = ++next_blob_id;

            // Check that it never overflows double mantissa, and provide immediate diagnostics if it happens.
            if (id != blobs::blob_id(double(id))) {
                fatal_error("CreateBlob: Blob ID overflow");
            }
            
            // Create a empty blob
            blobs[id] = blobs::blob();

            respond_to_message(msg, static_cast<double>(id));
        }

        blobs::blob_id create_blob(blobs::blob&& blob) {
            std::lock_guard<std::mutex> lock(blobs_mutex);
            blobs::blob_id id = ++next_blob_id;

            // Check that it never overflows double mantissa, and provide immediate diagnostics if it happens.
            if (id != blobs::blob_id(double(id))) {
                fatal_error("Blob ID overflow");
            }

            blobs[id] = std::move(blob);
            return id;
        }

        void compress_data(blob& compressed_blob, void* data, size_t length) {
            fs::path temp_archive = std::tmpnam(nullptr);

            {   // scoping for zip archiver
                // Open ZIP archive file
                int zip_err = ZIP_ER_OK;
                zip_t* archive = zip_open(temp_archive.make_preferred().string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zip_err);
                SCOPE_WARDEN(_zip_close, {
                    if (archive) {
                        zip_close(archive);
                    }
                });

                if (zip_err != ZIP_ER_OK) {
                    fatal_error("Error while creating compressed file.");
                }

                zip_error_t zip_error = {};
                zip_source_t* source = zip_source_buffer(archive, data, length, 0);
                if (zip_error_code_zip(&zip_error) != ZIP_ER_OK || zip_error_code_system(&zip_error) < 0 || !source) {
                    zip_source_free(source);
                    fatal_error("Error while creating compressed file source from buffer.");
                }

                if (zip_file_add(archive, "data", source, ZIP_FL_ENC_GUESS) < ZIP_ER_OK) {
                    zip_source_free(source);
                    fatal_error("Error while adding compressed file source to archive.");
                }

                // NOTE: Do NOT free source after a successful zip_file_add
            }

            append_from_file(compressed_blob, temp_archive.make_preferred().string().c_str());
            fs::remove(temp_archive);
        }

        blobs::blob_id create_compressed_blob(blobs::blob&& blob) {
            blobs::blob compressed_blob;
            compress_data(compressed_blob, blob.data(), blob.size());
            return create_blob(compressed_blob);
        }

        bool get_blob(blobs::blob_id id, blobs::blob& blob) {
            std::lock_guard<std::mutex> lock(blobs_mutex);
            auto& it = blobs.find(id);

            if (it == blobs.end()) {
                return false;
            }

            blob = it->second;
            return true;
        }

        void destroy_blob(blobs::blob_id blob_id) {
            std::lock_guard<std::mutex> lock(blobs_mutex);
            blobs.erase(blob_id);
        }

        void destroy_blobs(const message& msg) {
            assert(!strcmp(msg.name(), "!DestroyBlob"));

            auto json = msg.json();

            std::lock_guard<std::mutex> lock(blobs_mutex);
            for (auto val : json) {
                if (!val.is<double>()) {
                    fatal_error("DestroyBlob: non-numeric blob ID");
                }

                auto id = static_cast<blobs::blob_id>(val.get<double>());
                blobs.erase(id);
            }
        }

        void get_blob_size(const message& msg) {
            assert(!strcmp(msg.name(), "?GetBlobSize"));

            auto json = msg.json();
            if (!json[0].is<double>()) {
                fatal_error("GetBlobSize: non-numeric blob ID");
            }
            auto id = static_cast<blobs::blob_id>(json[0].get<double>());

            std::lock_guard<std::mutex> lock(blobs_mutex);
            auto& it = blobs.find(id);
            if (it == blobs.end()) {
                fatal_error("GetBlobSize: no blob with ID %llu", id);
            }

            respond_to_message(msg, ensure_fits_double(it->second.size()));
        }

        void set_blob_size(const message& msg) {
            assert(!strcmp(msg.name(), "!SetBlobSize"));

            auto json = msg.json();
            if (!json[0].is<double>()) {
                fatal_error("SetBlobSize: non-numeric blob ID");
            }
            auto id = static_cast<blobs::blob_id>(json[0].get<double>());

            if (!json[1].is<double>()) {
                fatal_error("SetBlobSize: non-numeric blob Size");
            }
            auto size = static_cast<size_t>(json[1].get<double>());

            std::lock_guard<std::mutex> lock(blobs_mutex);
            auto& it = blobs.find(id);
            if (it == blobs.end()) {
                fatal_error("SetBlobSize: no blob with ID %llu", id);
            }

            it->second.resize(size);
            respond_to_message(msg, ensure_fits_double(it->second.size()));
        }

        void read_blob(const message& msg) {
            assert(!strcmp(msg.name(), "?ReadBlob"));

            auto json = msg.json();
            if (!json[0].is<double>()) {
                fatal_error("ReadBlob: non-numeric blob ID");
            }
            auto id = static_cast<blobs::blob_id>(json[0].get<double>());

            if (!json[1].is<double>()) {
                fatal_error("ReadBlob: non-numeric position");
            }
            long long pos = static_cast<long long>(json[1].get<double>());
            if (pos < 0) {
                fatal_error("ReadBlob: position cannot be < 0");
            }

            if (!json[2].is<double>()) {
                fatal_error("ReadBlob: non-numeric byte count");
            }
            long long count = static_cast<long long>(json[2].get<double>());

            if (count < -1) {
                fatal_error("ReadBlob: byte count cannot be < -1");
            }

            std::lock_guard<std::mutex> lock(blobs_mutex);
            auto& it = blobs.find(id);
            if (it == blobs.end()) {
                fatal_error("ReadBlob: no blob with ID %llu", id);
            }

            if (pos >= static_cast<long long>(it->second.size()) && count >= 0) {
                // .net stream read requires an empty/zero sized read to identify end-of-stream.
                blobs::blob empty;
                respond_to_message(msg, empty);
                return;
            }

            if (pos == 0 && count == -1) {
                // Read all
                respond_to_message(msg, it->second);
                return;
            } 
            
            // Read at position and count
            size_t size = pos;
            size += count;
            if (count == -1 || size > it->second.size()) {
                count = it->second.size() - pos;
            }

            blobs::blob::const_iterator begin = it->second.begin() + pos;
            blobs::blob::const_iterator end = begin + count;

            blobs::blob part(begin, end);
            respond_to_message(msg, part);
        }

        void write_blob(const message& msg) {
            assert(!strcmp(msg.name(), "?WriteBlob"));

            auto json = msg.json();
            if (!json[0].is<double>()) {
                fatal_error("WriteBlob: non-numeric blob ID");
            }
            auto id = static_cast<blobs::blob_id>(json[0].get<double>());

            if (!json[1].is<double>()) {
                fatal_error("ReadBlob: non-numeric position");
            }
            long long pos = static_cast<long long>(json[1].get<double>());

            std::lock_guard<std::mutex> lock(blobs_mutex);
            auto& it = blobs.find(id);
            if (it == blobs.end()) {
                fatal_error("WriteBlob: no blob with ID %llu", id);
            }

            if (pos == -1 || pos == it->second.size()) {
                // append to the end of the blob
                auto blob = msg.blob();
                it->second.insert(it->second.end(), blob.begin(), blob.end());
            } else {
                // write/over-write at position
                auto blob = msg.blob();
                size_t size = pos;
                size += blob.size();
                if (it->second.size() < size) {
                    it->second.resize(size);
                }

                std::copy(blob.begin(), blob.end(), it->second.begin() + pos);
            }
            
            respond_to_message(msg, ensure_fits_double(it->second.size()));
        }

        void handle_eval(const message& msg) {
            assert(msg.name()[0] == '?' && msg.name()[1] == '=');

            auto args = msg.json();
            if (args.size() != 1 || !args[0].is<std::string>()) {
                fatal_error("Invalid evaluation request #%llu#: must have form [expr].", msg.id());
            }

            SCOPE_WARDEN_RESTORE(allow_callbacks);
            allow_callbacks = false;

            const auto& expr = from_utf8(args[0].get<std::string>());
            log::logf(log_verbosity::traffic, "#%llu# = %s\n\n", msg.id(), expr.c_str());

            SEXP env = nullptr;
            bool is_cancelable = false, new_env = false, no_result = false, raw_response = false;

            for (const char* p = msg.name() + 2; *p; ++p) {
                switch (char c = *p) {
                case 'B':
                case 'E':
                    if (env != nullptr) {
                        fatal_error("'%s': multiple environment flags specified.", msg.name());
                    }
                    env = (c == 'B') ? R_BaseEnv : R_EmptyEnv;
                    break;
                case 'N':
                    new_env = true;
                    break;
                case '@':
                    allow_callbacks = true;
                    break;
                case '/':
                    is_cancelable = true;
                    break;
                case '0':
                    no_result = true;
                    break;
                case 'r':
                    raw_response = true;
                    break;
                default:
                    fatal_error("'%s': unrecognized flag '%c'.", msg.name(), c);
                }
            }

            if (!env) {
                env = R_GlobalEnv;
            }

            r_eval_result<protected_sexp> result = {};
            ParseStatus ps;
            {
                // We must not register this eval as a potential cancellation target before it gets a chance to establish
                // the restart context; otherwise, there is a possibility that a cancellation request will arrive during
                // that interval, and abort the outer eval instead. Similarly, we must remove this eval from the eval stack
                // before the restart context is torn down, so that untimely cancellation request for the outer eval doesn't
                // cancel his one.

                bool was_before_invoked = false;
                auto before = [&] {
                    std::lock_guard<std::mutex> lock(eval_stack_mutex);
                    eval_stack.push_back(eval_info(msg.id(), is_cancelable));
                    was_before_invoked = true;
                };

                bool was_after_invoked = false;
                auto after = [&] {
                    std::lock_guard<std::mutex> lock(eval_stack_mutex);

                    if (was_before_invoked) {
                        assert(!eval_stack.empty());
                        assert(eval_stack.end()[-1].id == msg.id());
                    }

                    if (canceling_eval && msg.id() == eval_cancel_target) {
                        // If we were unwinding the stack for cancellation purposes, and this eval was the target
                        // of the cancellation, then we're done and should stop unwinding. Otherwise, we should 
                        // continue unwinding after reporting the result of the evaluation, which we'll do at the
                        // end of handle_eval if this flag is still set.
                        canceling_eval = false;
                    }

                    if (was_before_invoked) {
                        eval_stack.pop_back();
                    }

                    was_after_invoked = true;
                };

                protected_sexp eval_env(new_env ? Rf_NewEnvironment(R_NilValue, R_NilValue, env) : env);

                auto results = r_try_eval(expr, eval_env.get(), ps, before, after);
                if (!results.empty()) {
                    result = results.back();
                }

                // If eval was canceled, the "after" block was never executed (since it is normally run within the eval
                // context, and so cancelation unwinds it along with everything else in that context), so we need to run
                // it manually afterwards. Note that there's no potential race with newly arriving cancellation requests
                // in this case, since we're already servicing one for this eval (or some parent eval).
                if (!was_after_invoked) {
                    after();
                }

                allow_intr_in_CallBack = true;
            }

            picojson::value parse_status;
            switch (ps) {
            case PARSE_NULL:
                parse_status = picojson::value("NULL");
                break;
            case PARSE_OK:
                parse_status = picojson::value("OK");
                break;
            case PARSE_INCOMPLETE:
                parse_status = picojson::value("INCOMPLETE");
                break;
            case PARSE_ERROR:
                parse_status = picojson::value("ERROR");
                break;
            case PARSE_EOF:
                parse_status = picojson::value("EOF");
                break;
            default:
                parse_status = picojson::value(double(ps));
                break;
            }

            picojson::value error, value;
            blob blob;
            if (result.has_error) {
                error = picojson::value(Rchar_to_utf8(result.error));
            }
            if (result.has_value && !no_result) {
                try {
                    if (raw_response) {
                        errors_to_exceptions([&] { to_blob(result.value.get(), blob); });
                    } else {
                        errors_to_exceptions([&] { to_json(result.value.get(), value); });
                    }
                } catch (r_error& err) {
                    fatal_error("%s", err.what());
                }
            }

#ifdef TRACE_JSON
            indent_log(+1);
#endif
            if (result.is_canceled) {
                respond_to_message(msg, picojson::value());
            } else {
                respond_to_message(msg, blob, parse_status, error, value);
            }
#ifdef TRACE_JSON
            indent_log(-1);
#endif

            // If cancellation hasn't finished yet, continue unwinding the context stack. We don't want to call
            // Rf_onintr here, because this would skip over all the local object destructors in this function,
            // as well as the callback that invoked it. Instead, throw an exception and let C++ do unwinding the
            // normal way, and callback will then catch it at the very end, and invoke Rf_onintr just before it 
            // would've normally returned to R; see with_cancellation.
            if (query_interrupt()) {
                throw eval_cancel_error();
            }
        }

        void handle_cancel(const std::string& name, const message& msg) {
            assert(name == "!/" || name == "!//");
            auto args = msg.json();

            message_id eval_id;
            if (name == "!//") {
                if (!args.empty()) {
                    fatal_error("Incorrect number or type of arguments to '!//'.");
                }
                eval_id = 0;
            } else {
                if (args.size() != 1 || !args[0].is<double>()) {
                    fatal_error("Incorrect number or type of arguments to '!/'.");
                }
                eval_id = static_cast<message_id>(args[0].get<double>());
            }

            std::lock_guard<std::mutex> lock(eval_stack_mutex);

            for (auto eval_info : eval_stack) {
                auto& id = eval_info.id;

                if (canceling_eval && id == eval_cancel_target) {
                    // If we're already in the process of cancelling some eval, and that one is below the
                    // one that we're been asked to cancel in the stack, then we don't need to do anything.
                    break;
                }

                if (id == eval_id) {
                    canceling_eval = true;
                    eval_cancel_target = id;
                    break;
                }
            }

            if (canceling_eval) {
                // Spin the loop in send_request_and_get_response so that it gets a chance to run cancel checks.
                unblock_message_loop();
            } else {
                // If we didn't find the target eval in the stack, it must have completed already, and we've
                // got a belated cancelation request for it, which we can simply ignore.
            }
        }

        void propagate_cancellation() {
            // Prevent CallBack from doing anything if it's called from within Rf_onintr again.
            allow_intr_in_CallBack = false;

            interrupt_eval();

            assert(!"Rf_onintr should never return.");
            throw;
        }

        void handle_pending_evals() {
            for (;;) {
                message msg;
                {
                    std::lock_guard<std::mutex> lock(eval_requests_mutex);
                    if (eval_requests.empty()) {
                        break;
                    } else {
                        msg = eval_requests.front();
                        eval_requests.pop();
                    }
                }

                handle_eval(msg);
            }
        }

        inline message send_request_and_get_response(const std::string& name, const picojson::array& args) {
            assert(name[0] == '?');

            response_state_t old_response_state;
            {
                std::lock_guard<std::mutex> lock(response_mutex);
                old_response_state = response_state;
                response_state = RESPONSE_EXPECTED;
            }

            message request(message::request_marker, name, args, blob());
            transport::send_message(request);
            auto id = request.id();

            shutdown_if_requested();

            indent_log(+1);
            SCOPE_WARDEN(dedent_log, { indent_log(-1); });

            for (;;) {
                message msg;
                for (;;) {
                    {
                        handle_pending_evals();

                        std::lock_guard<std::mutex> lock(response_mutex);
                        if (response_state == RESPONSE_UNEXPECTED) {
                            assert(false);
                            fatal_error("Invalid response state transition: went from RESPONSE_EXPECTED to RESPONSE_UNEXPECTED.");
                        }
                        if (response_state == RESPONSE_RECEIVED) {
                            msg = response;
                            response_state = old_response_state;
                            break;
                        }
                    }

                    shutdown_if_requested();

                    // R_ProcessEvents may invoke CallBack. If there is a pending cancellation request, we do
                    // not want CallBack to call Rf_onintr as it normally does, since it would unwind the stack
                    // using longjmp, which will skip destructors for all our local variables. Instead, make
                    // CallBack a no-op until event processing is done, and then do a manual cancellation check.
                    allow_intr_in_CallBack = false;

                    R_ToplevelExec([](void*) {
                        // Errors can happen during event processing (from GUI windows such as graphs), and
                        // we don't want them to bubble up here, so run these in a fresh execution context.
                        is_waiting_for_wm = true;
                        R_WaitEvent();
                        is_waiting_for_wm = false;
                        R_ProcessEvents();
                    }, nullptr);

                    // In case anything in R_WaitEvent failed and unwound the context before we could reset.
                    is_waiting_for_wm = false;

                    allow_intr_in_CallBack = true;

                    shutdown_if_requested();

                    if (query_interrupt()) {
                        throw eval_cancel_error();
                    }
                }

                assert(msg.is_response());

                if (msg.request_id() != id) {
                    fatal_error("Received response [%llu,'%s'], while awaiting response for [%llu,'%s'].",
                        msg.request_id(), msg.name(), id, name.c_str());
                } else if (strcmp(msg.name() + 1, name.c_str() + 1) != 0) {
                    fatal_error("Response to [%llu,'%s'] has mismatched name '%s'.",
                        id, name.c_str(), msg.name());
                }

                return msg;
            }
        }

        picojson::array get_context() {
            picojson::array context;
            for (RCNTXT* ctxt = R_GlobalContext; ctxt != nullptr; ctxt = ctxt->nextcontext) {
                context.push_back(picojson::value(double(ctxt->callflag)));
            }
            return context;
        }

        extern "C" void CallBack() {
            shutdown_if_requested();

            reset_idle_timer();

            // Called periodically by R_ProcessEvents and Rf_eval. This is where we check for various
            // cancellation requests and issue an interrupt (Rf_onintr) if one is applicable in the
            // current context.
            callback_started();

            // Rf_onintr may end up calling CallBack before it returns. We don't want to recursively
            // call it again, so do nothing and let the next eligible callback handle things.
            if (!allow_intr_in_CallBack) {
                return;
            }

            if (query_interrupt()) {
                allow_intr_in_CallBack = false;
                interrupt_eval();
                // Note that allow_intr_in_CallBack is not reset to false here. This is because Rf_onintr
                // does not return (it unwinds via longjmp), and therefore any code here wouldn't run.
                // Instead, we reset the flag where the control will end up after unwinding - either
                // immediately after r_try_eval returns, or else (if we unwound R's own REPL eval) at
                // the beginning of the next ReadConsole.
                assert(!"Rf_onintr should never return.");
            }

            // Process any pending eval requests if reentrancy is allowed.
            if (allow_callbacks) {
                handle_pending_evals();
            }
        }

        extern "C" int R_ReadConsole(const char* prompt, char* buf, int len, int addToHistory) {
            return with_cancellation([&] {
                // The moment we get the first ReadConsole from R is when it's ready to process our requests.
                // Until then, attempts to do things (especially to eval arbitrary code) can fail because
                // the standard library is not fully loaded yet.
                {
                    std::lock_guard<std::mutex> lock(is_r_ready_lock);
                    is_r_ready = true;
                    is_r_ready_cond.notify_all();
                }

                if (!allow_intr_in_CallBack) {
                    // If we got here, this means that we've just processed a cancellation request that had
                    // unwound the context stack all the way to the bottom, cancelling all the active evals;
                    // otherwise, handle_eval would have allow_intr_in_CallBack set to true immediately after
                    // the targeted eval had returned. Mark everything cancellation-related as done.
                    assert(eval_stack.size() == 1);
                    canceling_eval = false;
                    allow_intr_in_CallBack = true;

                    // Notify client that cancellation has completed. When a specific eval is being canceled,
                    // there will be a corresponding (error) response to the original '?=' message indicating
                    // completion, but for top-level canellation we need a special message.
                    send_notification("!CanceledAll");
                }

                bool is_browser = false;
                for (RCNTXT* ctxt = R_GlobalContext; ctxt != nullptr; ctxt = ctxt->nextcontext) {
                    if (ctxt->callflag & CTXT_BROWSER) {
                        is_browser = true;
                        break;
                    }
                }

                if (!allow_callbacks && len >= 3) {
                    if (is_browser) {
                        // If this is a Browse prompt, raising an error is not a proper way to reject it -
                        // it will simply start an infinite loop with every new error producing such prompt.
                        // Instead, just tell the interpreter to continue execution.
                        buf[0] = 'c';
                        buf[1] = '\n';
                        buf[2] = '\0';
                        return 1;
                    }

                    Rf_error("ReadConsole: blocking callback not allowed during evaluation.");
                }

                // Check for and perform auto-stepping on the current instruction if necessary.
                if (is_browser && R_Srcref && R_Srcref != R_NilValue) {
                    static SEXP auto_step_over_symbol = Rf_install("Microsoft.R.Host::auto_step_over");
                    int auto_step_over = Rf_asLogical(Rf_getAttrib(R_Srcref, auto_step_over_symbol));
                    if (auto_step_over && auto_step_over != R_NaInt) {
                        buf[0] = 'n';
                        buf[1] = '\n';
                        buf[2] = '\0';
                        return 1;
                    }
                }

                readconsole_done();

                for (std::string retry_reason;;) {
                    auto msg = send_request_and_get_response(
                        "?>", get_context(), double(len), addToHistory != 0,
                        retry_reason.empty() ? picojson::value() : picojson::value(retry_reason),
                        to_utf8_json(prompt));

                    auto args = msg.json();
                    if (args.size() != 1) {
                        fatal_error("ReadConsole: response must have a single argument.");
                    }

                    const auto& arg = args[0];
                    if (arg.is<picojson::null>()) {
                        return 0;
                    }

                    if (!arg.is<std::string>()) {
                        fatal_error("ReadConsole: response argument must be string or null.");
                    }

                    auto s = from_utf8(arg.get<std::string>());
                    if (s.size() >= len) {
                        retry_reason = "BUFFER_OVERFLOW";
                        continue;
                    }

                    strcpy_s(buf, len, s.c_str());
                    return 1;
                }
            });
        }

        extern "C" void WriteConsoleEx(const char* buf, int len, int otype) {
            with_cancellation([&] {
                send_notification((otype ? "!!" : "!"), to_utf8_json(buf));
            });
        }

        extern "C" void Busy(int which) {
            with_cancellation([&] {
                send_notification(which ? "!+" : "!-");
            });
        }

        void message_received(const message& incoming) {
            reset_idle_timer();

            // If R is not ready yet, wait until it is before processing any incoming requests
            // to avoid racing with R initialization code.
            {
                std::unique_lock<std::mutex> lock(is_r_ready_lock);
                is_r_ready_cond.wait(lock, [] { return is_r_ready; });
            }

            std::string name = incoming.name();
            if (name == "!Shutdown") {
                request_shutdown(incoming);
            } else if (name == "!/" || name == "!//") {
                return handle_cancel(name, incoming);
            } else if (name == "?CreateBlob") {
                return create_blob(incoming);
            } else if (name == "?GetBlobSize") {
                return get_blob_size(incoming);
            } else if (name == "!SetBlobSize") {
                return set_blob_size(incoming);
            } else if (name == "?ReadBlob") {
                return read_blob(incoming);
            } else if (name == "?WriteBlob") {
                return write_blob(incoming);
            } else if (name == "!DestroyBlob") {
                return destroy_blobs(incoming);
            } else if (name.size() >= 2 && name[0] == '?' && name[1] == '=') {
                std::lock_guard<std::mutex> lock(eval_requests_mutex);
                eval_requests.push(incoming);
                unblock_message_loop();
                return;
            } else if (incoming.is_response()) {
                std::lock_guard<std::mutex> lock(response_mutex);
                assert(response_state != RESPONSE_RECEIVED);
                if (response_state == RESPONSE_UNEXPECTED) {
                    fatal_error("Unexpected incoming client response.");
                }

                response = std::move(incoming);
                response_state = RESPONSE_RECEIVED;
                unblock_message_loop();
                return;
            } else {
                fatal_error("Unrecognized message.");
            }
        }

        void initialize(structRstart& rp, const fs::path& rdata, std::chrono::seconds idle_timeout) {
            host::rdata = rdata;

            main_thread_id = GetCurrentThreadId();

            transport::message_received.connect(message_received);
            transport::disconnected.connect(unblock_message_loop);

            rp.ReadConsole = R_ReadConsole;
            rp.WriteConsoleEx = WriteConsoleEx;
            rp.CallBack = CallBack;
            rp.ShowMessage = ShowMessage;
            rp.YesNoCancel = YesNoCancel;
            rp.Busy = Busy;

            send_notification("!Microsoft.R.Host", 1.0, getDLLVersion());

            if (idle_timeout > 0s) {
                logf(log_verbosity::minimal, "Host process will shut down after %lld seconds of inactivity.\n", idle_timeout.count());
                std::thread([&] { idle_timer_thread(idle_timeout); }).detach();
            }
        }

        extern "C" void ShowMessage(const char* s) {
            with_cancellation([&] {
                send_notification("!ShowMessage", to_utf8_json(s));
            });
        }

        int ShowMessageBox(const char* s, const char* cmd) {
            return with_cancellation([&] {
                if (!allow_callbacks) {
                    Rf_error("ShowMessageBox: blocking callback not allowed during evaluation.");
                }

                auto msg = send_request_and_get_response(cmd, get_context(), to_utf8_json(s));
                auto args = msg.json();
                if (args.size() != 1 || !args[0].is<std::string>()) {
                    fatal_error("ShowMessageBox: response argument must be a string.");
                }

                auto& r = args[0].get<std::string>();
                if (r == "N") {
                    return -1; // graphapp.h => NO
                } else if (r == "C") {
                    return 0; // graphapp.h => CANCEL
                } else if (r == "Y") {
                    return 1; // graphapp.h => YES
                } else if (r == "O") {
                    return 1; // graphapp.h => YES
                } else {
                    fatal_error("ShowMessageBox: response argument must be 'Y', 'N' or 'C'.");
                }
            });
        }

        extern "C" int YesNoCancel(const char* s) {
            return ShowMessageBox(s, "?YesNoCancel");
        }

        extern "C" int YesNo(const char* s) {
            return ShowMessageBox(s, "?YesNo");
        }

        extern "C" int OkCancel(const char* s) {
            return ShowMessageBox(s, "?OkCancel");
        }
    }
}
