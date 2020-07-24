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
#include "log.h"
#include "xamlbuilder.h"
#include "host.h"
#include "eval.h"
#include "util.h"
#include "grdevices.h"
#include "exports.h"

using namespace rhost::rapi;

namespace rhost {
    namespace grdevices {
        namespace ide {
            template <int ApiVer>
            class ide_device;

            template <int ApiVer>
            class plot {
                typedef rapi::gd_api<ApiVer> gd_api;
                typedef ide::ide_device<ApiVer> ide_device;
                typedef typename gd_api::DevDesc DevDesc;

            public:
                plot(DevDesc* dd, plot* source_plot = nullptr);
                virtual ~plot();

                boost::uuids::uuid get_id() const;
                void set_pending_render();
                bool has_pending_render() const;
                bool pending_render_timeout_elapsed() const;
                void render(bool save_snapshot);
                void render_empty();
                void render_from_display_list();
                void render_from_snapshot();
                void set_snapshot(const rhost::util::protected_sexp& snapshot);

            private:
                void create_snapshot();
                void remove_snapshot_render_file();

                boost::uuids::uuid _plot_id;
                DevDesc* _device_desc;
                bool _has_pending_render;
                boost::posix_time::ptime _last_pending_render_time;
                double _snapshot_render_width;
                double _snapshot_render_height;
                fs::path _snapshot_render_filename;
                rhost::util::protected_sexp _snapshot;
            };

            template <int ApiVer>
            class plot_history {
                typedef rapi::gd_api<ApiVer> gd_api;
                typedef ide::ide_device<ApiVer> ide_device;
                typedef typename gd_api::DevDesc DevDesc;
                typedef ide::plot<ApiVer> plot;

            public:
                plot_history(DevDesc* dd);

                plot* get_active() const;
                plot* get_plot(const boost::uuids::uuid& plot_id);
                bool select(const boost::uuids::uuid& plot_id);
                void move_next();
                void move_previous();

                void new_page();
                void append(std::unique_ptr<plot> plot);
                void clear();
                void remove(const boost::uuids::uuid& plot_id);

                void resize(double width, double height, double resolution);
                void render_from_snapshot();

                int plot_count() const;
                int active_plot_index() const;

            private:
                typename std::vector<std::unique_ptr<plot>>::iterator _active_plot;
                typename std::vector<std::unique_ptr<plot>> _plots;
                DevDesc* _device_desc;
                bool _replaying;

                class replay_mode {
                public:
                    replay_mode(plot_history& history)
                        : _history(history) {
                        _history._replaying = true;
                    }

                    ~replay_mode() {
                        _history._replaying = false;
                    }
                private:
                    plot_history& _history;
                };
            };

            template <int ApiVer>
            class ide_device : public graphics_device<ApiVer> {
                typedef rapi::gd_api<ApiVer> gd_api;
                typedef typename gd_api::DevDesc DevDesc;
                typedef ide::plot<ApiVer> plot;
                typedef ide::plot_history<ApiVer> plot_history;

            public:
                static std::unique_ptr<ide_device> create(const boost::uuids::uuid& device_id, std::string device_type, double width, double height, double resolution);
                static void copy_device_attributes(DevDesc* source_dd, DevDesc* target_dd);

                ide_device(DevDesc* dd, const boost::uuids::uuid& device_id, std::string device_type, double width, double height, double resolution);
                virtual ~ide_device();

                boost::signals2::signal<void(ide_device*)> closed;

            public:
                virtual void activate();
                virtual void circle(double x, double y, double r, pGEcontext gc);
                virtual void clip(double x0, double x1, double y0, double y1);
                virtual void close();
                virtual void deactivate();
                virtual Rboolean locator(double *x, double *y);
                virtual void line(double x1, double y1, double x2, double y2, const pGEcontext gc);
                virtual void metric_info(int c, const pGEcontext gc, double* ascent, double* descent, double* width);
                virtual void mode(int mode);
                virtual void new_page(const pGEcontext gc);
                virtual void polygon(int n, double *x, double *y, const pGEcontext gc);
                virtual void polyline(int n, double *x, double *y, const pGEcontext gc);
                virtual void rect(double x0, double y0, double x1, double y1, const pGEcontext gc);
                virtual void path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc);
                virtual void raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc);
                virtual SEXP cap();
                virtual void size(double *left, double *right, double *bottom, double *top);
                virtual double str_width(const char *str, const pGEcontext gc);
                virtual void text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc);
                virtual void on_exit();
                virtual Rboolean new_frame_confirm();
                virtual void text_utf8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc);
                virtual double str_width_utf8(const char *str, const pGEcontext gc);
                virtual void event_helper(int code);
                virtual int hold_flush(int level);

            public:
                static std::vector<ide_device*> devices;

                boost::uuids::uuid get_id() const;
                plot* get_plot(const boost::uuids::uuid& plot_id);
                plot* copy_plot_from(ide_device *source_device, const boost::uuids::uuid& source_plot_id);
                void render_request(bool immediately);
                void resize(double width, double height, double resolution);
                double width() const { return _width; }
                double height() const { return _height; }
                fs::path save();
                fs::path save_empty();
                void send_clear();
                void send(const boost::uuids::uuid& plot_id, const fs::path& filename);

                bool history_select(const boost::uuids::uuid& plot_id, bool force_render);
                void history_next();
                void history_previous();
                void history_clear();
                void history_remove(const boost::uuids::uuid& plot_id);

                int plot_count() const;
                int active_plot_index() const;
                plot* active_plot() const;

                void select();
                void delete_file_device();
                void output_and_kill_file_device();

                static void process_pending_render(bool immediately);
                static ide_device* find_device_by_num(int device_num);
                static ide_device* find_device_by_id(const boost::uuids::uuid& device_id);

            private:
                fs::path get_render_file_path();
                DevDesc* get_or_create_file_device();
                DevDesc* create_file_device();
                void sync_file_device();
                void set_pending_render();

                static void init_devdesc(DevDesc* dd);
                static DevDesc* create_file_device(const std::string& device_type, const fs::path& filename, double width, double height, double resolution);

            private:
                boost::uuids::uuid _device_id;
                double _width;
                double _height;
                double _resolution;
                bool _debug;
                DevDesc* _file_device;
                std::string _file_device_type;
                fs::path _file_device_filename;
                plot_history _history;
            };

            class current_device_restorer {
            private:
                int _device_num;
            public:
                current_device_restorer() {
                    rhost::util::errors_to_exceptions([&] {
                        _device_num = Rf_curDevice();
                    });
                }

                ~current_device_restorer() {
                    rhost::util::errors_to_exceptions([&] {
                        Rf_selectDevice(_device_num);
                    });
                }
            };

            static boost::uuids::random_generator uuid_generator;

            template <int ApiVer>
            plot<ApiVer>::plot(DevDesc* dd, plot* source_plot) :
                _plot_id(uuid_generator()),
                _device_desc(dd),
                _snapshot(source_plot ? source_plot->_snapshot : nullptr),
                _has_pending_render(false) {
            }

            template <int ApiVer>
            plot<ApiVer>::~plot() {
                remove_snapshot_render_file();
            }

            template <int ApiVer>
            void plot<ApiVer>::remove_snapshot_render_file() {
                if (!_snapshot_render_filename.empty()) {
                    try {
                        fs::remove(_snapshot_render_filename);
                    }
                    catch (const fs::filesystem_error&) {
                    }
                }
            }

            template <int ApiVer>
            boost::uuids::uuid plot<ApiVer>::get_id() const {
                return _plot_id;
            }

            template <int ApiVer>
            void plot<ApiVer>::set_pending_render() {
                _has_pending_render = true;
                _last_pending_render_time = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
            }

            template <int ApiVer>
            bool plot<ApiVer>::has_pending_render() const {
                return _has_pending_render;
            }

            template <int ApiVer>
            bool plot<ApiVer>::pending_render_timeout_elapsed() const {
                auto now = boost::posix_time::second_clock::local_time();
                auto elapsed = now - _last_pending_render_time;
                return elapsed.total_milliseconds() >= 50;
            }

            template <int ApiVer>
            void plot<ApiVer>::render_empty() {
                auto xdd = reinterpret_cast<ide_device*>(_device_desc->deviceSpecific);
                auto path = xdd->save_empty();

                remove_snapshot_render_file();

                _snapshot_render_filename = path;
                xdd->send(_plot_id, path);
            }

            template <int ApiVer>
            void plot<ApiVer>::render(bool save_snapshot) {
                if (!has_pending_render()) {
                    return;
                }

                _has_pending_render = false;

                auto xdd = reinterpret_cast<ide_device*>(_device_desc->deviceSpecific);
                auto path = xdd->save();

                remove_snapshot_render_file();

                if (path.empty()) {
                    return;
                }

                _snapshot_render_filename = path;
                _snapshot_render_width = xdd->width();
                _snapshot_render_height = xdd->height();

                if (save_snapshot) {
                    create_snapshot();
                }

                xdd->send(_plot_id, path);
            }

            template <int ApiVer>
            void plot<ApiVer>::render_from_display_list() {
                rhost::util::errors_to_exceptions([&] {
                    pGEDevDesc ge_dev_desc = gd_api::Rf_desc2GEDesc(_device_desc);
                    gd_api::GEplayDisplayList(ge_dev_desc);
                });

                render(true);
            }

            template <int ApiVer>
            void plot<ApiVer>::render_from_snapshot() {
                auto xdd = reinterpret_cast<ide_device*>(_device_desc->deviceSpecific);
                xdd->output_and_kill_file_device();

                try {
                    rhost::util::errors_to_exceptions([&] {
                        auto snapshot = _snapshot.get();
                        if (snapshot != nullptr && snapshot != R_UnboundValue && snapshot != R_NilValue) {
                            pGEDevDesc ge_dev_desc = gd_api::Rf_desc2GEDesc(_device_desc);
                            gd_api::GEplaySnapshot(snapshot, ge_dev_desc);
                        }
                        else {
                            render_empty();
                        }
                    });
                }
                catch (rhost::util::r_error&) {
                    render_empty();
                }
            }

            template <int ApiVer>
            void plot<ApiVer>::set_snapshot(const rhost::util::protected_sexp& snapshot) {
                rhost::util::errors_to_exceptions([&] {
                    SEXP klass = Rf_protect(Rf_mkString("recordedplot"));
                    Rf_classgets(snapshot.get(), klass);

                    _snapshot = Rf_duplicate(snapshot.get());

                    Rf_unprotect(1);
                });
            }

            template <int ApiVer>
            void plot<ApiVer>::create_snapshot() {
                rhost::util::errors_to_exceptions([&] {
                    pGEDevDesc ge_dev_desc = gd_api::Rf_desc2GEDesc(_device_desc);
                    _snapshot = gd_api::GEcreateSnapshot(ge_dev_desc);
                });
            }

            ///////////////////////////////////////////////////////////////////////
            // Ide device plot history
            ///////////////////////////////////////////////////////////////////////

            template <int ApiVer>
            plot_history<ApiVer>::plot_history(DevDesc* dd) :
                _device_desc(dd),
                _replaying(false) {
                _active_plot = _plots.begin();
            }

            template <int ApiVer>
            auto plot_history<ApiVer>::get_active() const -> plot* {
                if (_active_plot == _plots.end()) {
                    return nullptr;
                }

                return (*_active_plot).get();
            }

            template <int ApiVer>
            auto plot_history<ApiVer>::get_plot(const boost::uuids::uuid& plot_id) -> plot* {
                auto plot = find_if(_plots.begin(), _plots.end(), [&](auto& p) { return p->get_id() == plot_id; });
                return (plot != _plots.end()) ? (*plot).get() : nullptr;
            }

            template <int ApiVer>
            bool plot_history<ApiVer>::select(const boost::uuids::uuid& plot_id) {
                if (_active_plot != _plots.end() && (*_active_plot)->get_id() == plot_id) {
                    return false;
                }

                auto plot = find_if(_plots.begin(), _plots.end(), [&](auto& p) { return p->get_id() == plot_id; });
                if (plot != _plots.end()) {
                    _active_plot = plot;
                    return true;
                }

                return false;
            }

            template <int ApiVer>
            void plot_history<ApiVer>::move_next() {
                auto end = _plots.end();
                if (_active_plot != end) {
                    auto next = std::next(_active_plot);
                    if (next != end) {
                        _active_plot = next;
                    }
                }
            }

            template <int ApiVer>
            void plot_history<ApiVer>::move_previous() {
                if (_active_plot != _plots.begin()) {
                    _active_plot--;
                }
            }

            template <int ApiVer>
            void plot_history<ApiVer>::new_page() {
                if (!_replaying) {
                    auto previous_plot = get_active();
                    if (previous_plot != nullptr) {
                        pGEDevDesc ge_dev_desc = gd_api::Rf_desc2GEDesc(_device_desc);
                        util::protected_sexp snapshot(ge_dev_desc->savedSnapshot);
                        if (previous_plot->has_pending_render()) {
                            previous_plot->set_snapshot(snapshot);
                            previous_plot->render(false);
                        }
                    }

                    // Create a plot object for this new page
                    append(std::make_unique<plot>(_device_desc));
                }
            }

            template <int ApiVer>
            void plot_history<ApiVer>::append(std::unique_ptr<plot> p) {
                _plots.push_back(std::move(p));
                _active_plot = std::prev(_plots.end(), 1);
            }

            template <int ApiVer>
            void plot_history<ApiVer>::clear() {
                _plots.clear();
                _active_plot = _plots.begin();
            }

            template <int ApiVer>
            void plot_history<ApiVer>::remove(const boost::uuids::uuid& plot_id) {
                auto plot = find_if(_plots.begin(), _plots.end(), [&](auto& p) { return p->get_id() == plot_id; });
                if (plot != _plots.end()) {
                    _active_plot = _plots.erase(plot);
                    // erase() returns an iterator that points to end() when removing the last item
                    // so adjust it to point to the new last item, if one is available
                    if (_active_plot == _plots.end() && _plots.size() > 0) {
                        _active_plot--;
                    }
                }
            }

            template <int ApiVer>
            void plot_history<ApiVer>::resize(double width, double height, double resolution) {
                auto plot = get_active();
                if (plot != nullptr) {
                    if (plot->has_pending_render()) {
                        auto replay = replay_mode(*this);
                        auto plot = _active_plot->get();
                        plot->render_from_display_list();
                    }
                    else {
                        render_from_snapshot();
                    }
                }
            }

            template <int ApiVer>
            void plot_history<ApiVer>::render_from_snapshot() {
                auto replay = replay_mode(*this);
                auto plot = _active_plot->get();
                if (plot != nullptr) {
                    plot->render_from_snapshot();
                }
            }

            template <int ApiVer>
            int plot_history<ApiVer>::plot_count() const {
                return (int)_plots.size();
            }

            template <int ApiVer>
            int plot_history<ApiVer>::active_plot_index() const {
                if (_active_plot == _plots.end()) {
                    return -1;
                }

                int index = static_cast<int>(_active_plot - _plots.begin());
                return index;
            }

            ///////////////////////////////////////////////////////////////////////
            // Ide device
            ///////////////////////////////////////////////////////////////////////

            template <int ApiVer>
            std::vector<ide_device<ApiVer>*> ide_device<ApiVer>::devices;

            template <>
            void ide_device<10>::init_devdesc(DevDesc* dd) {
                dd->displayListOn = R_TRUE;
                dd->canGenMouseDown = R_FALSE;
                dd->canGenMouseMove = R_FALSE;
                dd->canGenMouseUp = R_FALSE;
                dd->canGenKeybd = R_FALSE;
            }

            template <>
            void ide_device<11>::init_devdesc(DevDesc* dd) {
                dd->displayListOn = R_TRUE;
                dd->canGenMouseDown = R_FALSE;
                dd->canGenMouseMove = R_FALSE;
                dd->canGenMouseUp = R_FALSE;
                dd->canGenKeybd = R_FALSE;
            }

            template <>
            void ide_device<12>::init_devdesc(DevDesc* dd) {
                dd->displayListOn = R_TRUE;
                dd->canGenMouseDown = R_FALSE;
                dd->canGenMouseMove = R_FALSE;
                dd->canGenMouseUp = R_FALSE;
                dd->canGenKeybd = R_FALSE;
                dd->canGenIdle = R_FALSE;
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::create(const boost::uuids::uuid& device_id, std::string device_type, double width, double height, double resolution) -> std::unique_ptr<ide_device> {
                auto dd = static_cast<DevDesc*>(calloc(1, sizeof(DevDesc)));
                auto xdd = std::make_unique<ide_device>(dd, device_id, device_type, width, height, resolution);

                DevDesc* file_dd = xdd->create_file_device();
                xdd->_file_device = file_dd;

                // Our ide device needs to look like an instance of the 
                // file device, so we copy most of its attributes
                // (don't overwrite the callbacks that are already assigned).
                copy_device_attributes(file_dd, dd);

                init_devdesc(dd);
                dd->deviceSpecific = xdd.get();

                return xdd;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::copy_device_attributes(DevDesc* source_dd, DevDesc* target_dd) {
                target_dd->left = source_dd->left;
                target_dd->right = source_dd->right;
                target_dd->bottom = source_dd->bottom;
                target_dd->top = source_dd->top;

                target_dd->clipLeft = source_dd->clipLeft;
                target_dd->clipRight = source_dd->clipRight;
                target_dd->clipBottom = source_dd->clipBottom;
                target_dd->clipTop = source_dd->clipTop;

                target_dd->xCharOffset = source_dd->xCharOffset;
                target_dd->yCharOffset = source_dd->yCharOffset;
                target_dd->yLineBias = source_dd->yLineBias;

                target_dd->ipr[0] = source_dd->ipr[0];
                target_dd->ipr[1] = source_dd->ipr[1];
                target_dd->cra[0] = source_dd->cra[0];
                target_dd->cra[1] = source_dd->cra[1];
                target_dd->gamma = source_dd->gamma;

                target_dd->canClip = source_dd->canClip;
                target_dd->canChangeGamma = source_dd->canChangeGamma;
                target_dd->canHAdj = source_dd->canHAdj;

                target_dd->startps = source_dd->startps;
                target_dd->startcol = source_dd->startcol;
                target_dd->startfill = source_dd->startfill;
                target_dd->startlty = source_dd->startlty;
                target_dd->startfont = source_dd->startfont;
                target_dd->startgamma = source_dd->startgamma;

                target_dd->hasTextUTF8 = source_dd->hasTextUTF8;
                target_dd->wantSymbolUTF8 = source_dd->wantSymbolUTF8;
                target_dd->useRotatedTextInContour = source_dd->useRotatedTextInContour;

                target_dd->haveTransparency = source_dd->haveTransparency;
                target_dd->haveTransparentBg = source_dd->haveTransparentBg;
                target_dd->haveRaster = source_dd->haveRaster;
                target_dd->haveCapture = source_dd->haveCapture;
                target_dd->haveLocator = source_dd->haveLocator;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::activate() {
            }

            template <int ApiVer>
            void ide_device<ApiVer>::circle(double x, double y, double r, pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->circle != nullptr) {
                    dev->circle(x, y, r, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::clip(double x0, double x1, double y0, double y1) {
                auto dev = get_or_create_file_device();
                if (dev->clip != nullptr) {
                    dev->clip(x0, x1, y0, y1, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::close() {
                auto device_name(boost::uuids::to_string(_device_id));
                rhost::host::with_cancellation([&] {
                    rhost::host::send_notification(
                        "!PlotDeviceDestroy",
                        rhost::util::Rchar_to_utf8(device_name)
                    );
                });

                delete_file_device();

                closed(this);
                delete this;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::deactivate() {
            }

            template <int ApiVer>
            Rboolean ide_device<ApiVer>::locator(double *x, double *y) {
                *x = 0;
                *y = 0;
                Rboolean clicked = R_FALSE;

                rhost::host::with_cancellation([&] {
                    auto device_name(boost::uuids::to_string(_device_id));
                    auto msg = rhost::host::send_request_and_get_response("?Locator", rhost::util::to_utf8_json(device_name.c_str()));
                    auto args = msg.json();
                    if (args.size() != 3 || !args[0].template is<bool>() || !args[1].template is<double>() || !args[2].template is<double>()) {
                        rhost::log::fatal_error("Locator response is malformed. It must have 3 elements: bool, double, double.");
                    }

                    auto& result_clicked = args[0].template get<bool>();
                    auto& result_x = args[1].template get<double>();
                    auto& result_y = args[2].template get<double>();
                    *x = result_x;
                    *y = result_y;
                    clicked = result_clicked ? R_TRUE : R_FALSE;
                });

                return clicked;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::line(double x1, double y1, double x2, double y2, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->line != nullptr) {
                    dev->line(x1, y1, x2, y2, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::metric_info(int c, const pGEcontext gc, double* ascent, double* descent, double* width) {
                *ascent = 0;
                *descent = 0;
                *width = 0;

                auto dev = get_or_create_file_device();
                if (dev->metricInfo != nullptr) {
                    dev->metricInfo(c, gc, ascent, descent, width, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::mode(int mode) {
                auto dev = get_or_create_file_device();
                if (dev->mode != nullptr) {
                    dev->mode(mode, dev);
                }

                set_pending_render();
            }

            template <int ApiVer>
            void ide_device<ApiVer>::new_page(const pGEcontext gc) {
                _history.new_page();

                auto dev = get_or_create_file_device();
                if (dev->newPage != nullptr) {
                    dev->newPage(gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::polygon(int n, double *x, double *y, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->polygon != nullptr) {
                    dev->polygon(n, x, y, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::polyline(int n, double *x, double *y, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->polyline != nullptr) {
                    dev->polyline(n, x, y, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::rect(double x0, double y0, double x1, double y1, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->rect != nullptr) {
                    dev->rect(x0, y0, x1, y1, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->path) {
                    dev->path(x, y, npoly, nper, winding, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->raster != nullptr) {
                    dev->raster(raster, w, h, x, y, width, height, rot, interpolate, gc, dev);
                }
            }


            template <int ApiVer>
            SEXP ide_device<ApiVer>::cap() {
                return R_NilValue;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::size(double *left, double *right, double *bottom, double *top) {
                auto dev = get_or_create_file_device();
                if (dev->size != nullptr) {
                    dev->size(left, right, bottom, top, dev);
                }
            }

            template <int ApiVer>
            double ide_device<ApiVer>::str_width(const char *str, const pGEcontext gc) {
                double width = 0;

                auto dev = get_or_create_file_device();
                if (dev->strWidth != nullptr) {
                    width = dev->strWidth(str, gc, dev);
                }
                return width;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->text != nullptr) {
                    dev->text(x, y, str, rot, hadj, gc, dev);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::on_exit() {
            }

            template <int ApiVer>
            Rboolean ide_device<ApiVer>::new_frame_confirm() {
                return R_FALSE;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::text_utf8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->textUTF8 != nullptr) {
                    dev->textUTF8(x, y, str, rot, hadj, gc, dev);
                }
            }

            template <int ApiVer>
            double ide_device<ApiVer>::str_width_utf8(const char *str, const pGEcontext gc) {
                double width = 0;

                auto dev = get_or_create_file_device();
                if (dev->strWidthUTF8 != nullptr) {
                    width = dev->strWidthUTF8(str, gc, dev);
                }

                return width;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::event_helper(int code) {
            }

            template <int ApiVer>
            int ide_device<ApiVer>::hold_flush(int level) {
                return 0;
            }

            template <int ApiVer>
            boost::uuids::uuid ide_device<ApiVer>::get_id() const {
                return _device_id;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::render_request(bool immediately) {
                auto plot = _history.get_active();
                if (plot != nullptr) {
                    if (plot->has_pending_render()) {
                        if (immediately || plot->pending_render_timeout_elapsed()) {
                            plot->render(true);
                        }
                    }
                }
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::get_plot(const boost::uuids::uuid& plot_id) -> plot* {
                return _history.get_plot(plot_id);
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::copy_plot_from(ide_device *source_device, const boost::uuids::uuid& source_plot_id) -> plot* {
                auto source_plot = source_device->get_plot(source_plot_id);
                if (source_plot == nullptr) {
                    return nullptr;
                }

                auto target_plot(std::make_unique<plot>(this->device_desc, source_plot));
                auto plot = target_plot.get();

                _history.append(std::move(target_plot));

                return plot;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::select() {
                rhost::util::errors_to_exceptions([&] {
                    auto num = gd_api::Rf_ndevNumber(this->device_desc);
                    Rf_selectDevice(num);
                });
            }

            template <int ApiVer>
            void ide_device<ApiVer>::resize(double width, double height, double resolution) {
                output_and_kill_file_device();

                _width = width;
                _height = height;
                _resolution = resolution;

                // Recreate the file device to obtain its new attributes,
                // based on the new width/height/resolution.
                _file_device = create_file_device();

                // Update the ide device with attributes based
                // on the new width/height/resolution, so that 
                // future plots on this device are calculated correctly.
                // https://github.com/Microsoft/RTVS/issues/2017
                copy_device_attributes(_file_device, this->device_desc);

                delete_file_device();

                _history.resize(width, height, resolution);
            }

            template <int ApiVer>
            fs::path ide_device<ApiVer>::save() {
                auto path = _file_device_filename;
                sync_file_device();
                output_and_kill_file_device();
                return path;
            }

            template <int ApiVer>
            fs::path ide_device<ApiVer>::save_empty() {
                auto path = get_render_file_path();
                std::ofstream empty_file(path.make_preferred().string());
                empty_file.close();
                return path;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::send_clear() {
                // send an 'empty' plot to ide to clear the plot window
                send(boost::uuids::uuid(), fs::path(""));
            }

            template <int ApiVer>
            void ide_device<ApiVer>::send(const boost::uuids::uuid& plot_id, const fs::path& filename) {
                auto path_copy(filename);
                auto plot_name(boost::uuids::to_string(plot_id));
                auto device_name(boost::uuids::to_string(_device_id));
                rhost::host::with_cancellation([&] {
                    std::string file_path = path_copy.make_preferred().string();
                    blobs::blob plot_image_data;

                    if (!file_path.empty()) {
                        blobs::append_from_file(plot_image_data, file_path);
                    }

                    int device_num = -1;
                    rhost::util::errors_to_exceptions([&] {
                        device_num = gd_api::Rf_ndevNumber(this->device_desc);
                    });

                    rhost::host::send_notification(
                        "!Plot",
                        plot_image_data,
                        rhost::util::Rchar_to_utf8(device_name),
                        rhost::util::Rchar_to_utf8(plot_name),
                        rhost::util::Rchar_to_utf8(file_path),
                        static_cast<double>(device_num + 1),
                        static_cast<double>(active_plot_index()),
                        static_cast<double>(plot_count())
                    );
                });
            }

            template <int ApiVer>
            ide_device<ApiVer>::ide_device(DevDesc* dd, const boost::uuids::uuid& device_id, std::string device_type, double width, double height, double resolution) :
                graphics_device<ApiVer>(dd),
                _device_id(device_id),
                _width(width),
                _height(height),
                _resolution(resolution),
                _debug(false),
                _history(dd),
                _file_device(nullptr),
                _file_device_type(device_type) {
            }

            template <int ApiVer>
            ide_device<ApiVer>::~ide_device() {
            }

            template <int ApiVer>
            fs::path ide_device<ApiVer>::get_render_file_path() {
                auto file_path = fs::temp_directory_path();
                auto file_name = std::string("rhost-ide-plot-") + boost::uuids::to_string(uuid_generator()) + std::string(".") + _file_device_type;
                file_path /= file_name;
                return file_path;
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::get_or_create_file_device() -> DevDesc* {
                if (_file_device == nullptr) {
                    _file_device = create_file_device();
                    sync_file_device();
                }
                return _file_device;
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::create_file_device() -> DevDesc* {
                _file_device_filename = get_render_file_path();
                return create_file_device(_file_device_type, _file_device_filename, _width, _height, _resolution);
            }

            template <int ApiVer>
            void ide_device<ApiVer>::sync_file_device() {
                if (_file_device == nullptr) {
                    return;
                }

                try {
                    rhost::util::errors_to_exceptions([&] {
                        int file_device_num = gd_api::Rf_ndevNumber(_file_device);
                        int ide_device_num = gd_api::Rf_ndevNumber(this->device_desc);

                        Rf_selectDevice(file_device_num);
                        GEcopyDisplayList(ide_device_num);
                        Rf_selectDevice(ide_device_num);
                    });
                }
                catch (rhost::util::r_error&) {
                    auto path = save_empty();
                    send(boost::uuids::uuid(), path);
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::delete_file_device() {
                // Kill the temporary file device and delete the file it opened on disk
                auto file_device_filename = _file_device_filename;
                output_and_kill_file_device();
                if (!file_device_filename.empty()) {
                    try {
                        fs::remove(file_device_filename);
                    }
                    catch (const fs::filesystem_error&) {
                    }
                }
            }

            template <int ApiVer>
            void ide_device<ApiVer>::output_and_kill_file_device() {
                rhost::util::errors_to_exceptions([&] {
                    // The device number is not constant, so get the current number
                    int file_device_num = gd_api::Rf_ndevNumber(_file_device);

                    // Killing the device will call close, which will save the file to disk
                    pGEDevDesc ge_dev_desc = gd_api::GEgetDevice(file_device_num);
                    gd_api::GEkillDevice(ge_dev_desc);
                });

                // Blank our state, next call to graphics primitive (if any) will create 
                // a new file device on demand
                _file_device = nullptr;
                _file_device_filename = fs::path();
            }

            template <int ApiVer>
            void ide_device<ApiVer>::set_pending_render() {
                auto plot = _history.get_active();
                if (plot != nullptr) {
                    plot->set_pending_render();
                }
            }

            template <int ApiVer>
            bool ide_device<ApiVer>::history_select(const boost::uuids::uuid& plot_id, bool force_render) {
                if (_history.select(plot_id) || force_render) {
                    _history.render_from_snapshot();
                    return true;
                }
                return false;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::history_next() {
                _history.move_next();
                _history.render_from_snapshot();
            }

            template <int ApiVer>
            void ide_device<ApiVer>::history_previous() {
                _history.move_previous();
                _history.render_from_snapshot();
            }

            template <int ApiVer>
            void ide_device<ApiVer>::history_clear() {
                _history.clear();
                send_clear();
            }

            template <int ApiVer>
            void ide_device<ApiVer>::history_remove(const boost::uuids::uuid& plot_id) {
                _history.remove(plot_id);
                if (_history.plot_count() > 0) {
                    _history.render_from_snapshot();
                }
                else {
                    send_clear();
                }
            }

            template <int ApiVer>
            int ide_device<ApiVer>::plot_count() const {
                return _history.plot_count();
            }

            template <int ApiVer>
            int ide_device<ApiVer>::active_plot_index() const {
                return _history.active_plot_index();
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::active_plot() const -> plot* {
                return _history.get_active();
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::create_file_device(const std::string& device_type, const fs::path& filename, double width, double height, double resolution) -> DevDesc* {
                auto expr = boost::format("%1%(filename='%2%', width=%3%, height=%4%, res=%5%)") % device_type % filename.generic_string() % width % height % resolution;

                // Create the file device via the public R API
                ParseStatus ps;
                auto result = rhost::eval::r_try_eval_str(expr.str(), R_GlobalEnv, ps);
                if (result.has_error) {
                    throw rhost::util::r_error(result.error.c_str());
                }

                DevDesc* dev_desc = nullptr;

                // Retrieve the device descriptor of the current device (the one created above)
                rhost::util::errors_to_exceptions([&] {
                    int device_num = Rf_curDevice();
                    pGEDevDesc ge_dev_desc = gd_api::GEgetDevice(device_num);
                    dev_desc = (DevDesc*)ge_dev_desc->dev;
                });

                return dev_desc;
            }

            template <int ApiVer>
            void ide_device<ApiVer>::process_pending_render(bool immediately) {
                for (auto dev : devices) {
                    dev->render_request(immediately);
                }
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::find_device_by_num(int device_num) -> ide_device* {
                auto dev = find_if(devices.begin(), devices.end(), [&](auto& d) {
                    return gd_api::Rf_ndevNumber(d->device_desc) == (device_num - 1);
                });

                return (dev != devices.end()) ? *dev : nullptr;
            }

            template <int ApiVer>
            auto ide_device<ApiVer>::find_device_by_id(const boost::uuids::uuid& device_id) -> ide_device* {
                auto dev = find_if(devices.begin(), devices.end(), [&](auto& d) {
                    return d->get_id() == device_id;
                });

                return (dev != devices.end()) ? *dev : nullptr;
            }

            template <int ApiVer>
            struct external_methods_impl {
                typedef rapi::gd_api<ApiVer> gd_api;
                typedef ide::ide_device<ApiVer> ide_device;

                static SEXP ide_graphicsdevice_new(SEXP args) {
                    int ver = R_GE_getVersion();
                    if (ver < R_32_GE_version || ver > R_34_GE_version) {
                        Rf_error("Graphics API version %d is not supported.", ver);
                    }

                    return rhost::util::exceptions_to_errors([&] {
                        R_CheckDeviceAvailable();
                        BEGIN_SUSPEND_INTERRUPTS{
                            double width;
                            double height;
                            double resolution;
                            boost::uuids::uuid device_id = uuid_generator();
                            auto device_name(boost::uuids::to_string(device_id));

                            rhost::host::with_cancellation([&] {
                                auto msg = rhost::host::send_request_and_get_response("?PlotDeviceCreate", rhost::util::to_utf8_json(device_name.c_str()));
                                auto args = msg.json();
                                if (args.size() != 3 || !args[0].is<double>() || !args[1].is<double>() || !args[2].is<double>()) {
                                    rhost::log::fatal_error("PlotDeviceCreate response is malformed. It must have 3 elements: double, double, double.");
                                }

                                width = args[0].get<double>();
                                height = args[1].get<double>();
                                resolution = args[2].get<double>();
                            });

                            auto dev = ide_device::create(device_id, "png", width, height, resolution);
                            pGEDevDesc gdd = gd_api::GEcreateDevDesc(dev->device_desc);
                            gd_api::GEaddDevice2(gdd, "ide");

                            // Owner is DevDesc::deviceSpecific, and is released in close()
                            dev->closed.connect([&](ide_device* o) {
                                ide_device::devices.erase(std::find(ide_device::devices.begin(), ide_device::devices.end(), o));
                            });
                            ide_device::devices.push_back(dev.release());
                        } END_SUSPEND_INTERRUPTS;

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_resize(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);
                    args = CDR(args);
                    SEXP param2 = CAR(args);
                    args = CDR(args);
                    SEXP param3 = CAR(args);
                    args = CDR(args);
                    SEXP param4 = CAR(args);

                    return rhost::util::exceptions_to_errors([&] {
                        auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));
                        double width = *REAL(param2);
                        double height = *REAL(param3);
                        double resolution = *REAL(param4);

                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            current_device_restorer device_restorer;

                            dev->select();
                            dev->resize(width, height, resolution);
                            dev->render_request(true);
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_next_plot(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            current_device_restorer device_restorer;

                            dev->select();
                            dev->history_next();
                            dev->render_request(true);
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_previous_plot(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            current_device_restorer device_restorer;

                            dev->select();
                            dev->history_previous();
                            dev->render_request(true);
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_clear_plots(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            current_device_restorer device_restorer;

                            dev->select();
                            dev->history_clear();
                            dev->render_request(true);
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_remove_plot(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);
                    args = CDR(args);
                    SEXP param2 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));
                    auto plot_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param2, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            current_device_restorer device_restorer;

                            dev->select();
                            dev->history_remove(plot_id);
                            dev->render_request(true);
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_copy_plot(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);
                    args = CDR(args);
                    SEXP param2 = CAR(args);
                    args = CDR(args);
                    SEXP param3 = CAR(args);

                    auto source_device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));
                    auto source_plot_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param2, 0)));
                    auto target_device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param3, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        current_device_restorer device_restorer;

                        auto source_dev = ide_device::find_device_by_id(source_device_id);
                        if (source_dev == nullptr) {
                            throw rhost::util::r_error("Source device could not be found.");
                        }

                        auto target_dev = ide_device::find_device_by_id(target_device_id);
                        if (target_dev == nullptr) {
                            throw rhost::util::r_error("Destination device could not be found.");
                        }

                        auto target_plot = target_dev->copy_plot_from(source_dev, source_plot_id);
                        if (target_plot == nullptr) {
                            throw rhost::util::r_error("Could not copy plot.");
                        }

                        target_dev->select();
                        if (target_dev->history_select(target_plot->get_id(), true)) {
                            target_dev->render_request(true);
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_select_plot(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);
                    args = CDR(args);
                    SEXP param2 = CAR(args);
                    args = CDR(args);
                    SEXP param3 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));
                    auto plot_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param2, 0)));
                    auto force_render = *LOGICAL(param3);

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            current_device_restorer device_restorer;

                            dev->select();
                            if (dev->history_select(plot_id, force_render != 0)) {
                                dev->render_request(true);
                            }
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_get_device_id(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);

                    int device_num = *INTEGER(param1);

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_num(device_num);
                        if (dev != nullptr) {
                            auto device_name(boost::uuids::to_string(dev->get_id()));

                            SEXP device_id = Rf_mkCharCE(device_name.c_str(), CE_UTF8);
                            Rf_protect(device_id);
                            SEXP result = Rf_allocVector(STRSXP, 1);
                            SET_STRING_ELT(result, 0, device_id);
                            Rf_unprotect(1);
                            return result;
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_get_device_num(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            auto result = Rf_allocVector(INTSXP, 1);
                            int num = gd_api::Rf_ndevNumber(dev->device_desc) + 1;
                            *INTEGER(result) = num;
                            return result;
                        }

                        return R_NilValue;
                    });
                }

                static SEXP ide_graphicsdevice_get_active_plot_id(SEXP args) {
                    args = CDR(args);
                    SEXP param1 = CAR(args);

                    auto device_id = boost::lexical_cast<boost::uuids::uuid>(R_CHAR(STRING_ELT(param1, 0)));

                    return rhost::util::exceptions_to_errors([&] {
                        auto dev = ide_device::find_device_by_id(device_id);
                        if (dev != nullptr) {
                            auto plot = dev->active_plot();
                            if (plot != nullptr) {
                                auto plot_name(boost::uuids::to_string(plot->get_id()));

                                SEXP plot_id = Rf_mkCharCE(plot_name.c_str(), CE_UTF8);
                                Rf_protect(plot_id);
                                SEXP result = Rf_allocVector(STRSXP, 1);
                                SET_STRING_ELT(result, 0, plot_id);
                                Rf_unprotect(1);
                                return result;
                            }
                        }

                        return R_NilValue;
                    });
                }

                static R_ExternalMethodDef external_methods[];
            };

            template <int ApiVer>
            R_ExternalMethodDef external_methods_impl<ApiVer>::external_methods[] = {
                {"Microsoft.R.Host::External.ide_graphicsdevice_new", (DL_FUNC)&ide_graphicsdevice_new, 0},
                {"Microsoft.R.Host::External.ide_graphicsdevice_resize", (DL_FUNC)&ide_graphicsdevice_resize, 4},
                {"Microsoft.R.Host::External.ide_graphicsdevice_next_plot", (DL_FUNC)&ide_graphicsdevice_next_plot, 1},
                {"Microsoft.R.Host::External.ide_graphicsdevice_previous_plot", (DL_FUNC)&ide_graphicsdevice_previous_plot, 1},
                {"Microsoft.R.Host::External.ide_graphicsdevice_clear_plots", (DL_FUNC)&ide_graphicsdevice_clear_plots, 1},
                {"Microsoft.R.Host::External.ide_graphicsdevice_remove_plot", (DL_FUNC)&ide_graphicsdevice_remove_plot, 2},
                {"Microsoft.R.Host::External.ide_graphicsdevice_copy_plot", (DL_FUNC)&ide_graphicsdevice_copy_plot, 3},
                {"Microsoft.R.Host::External.ide_graphicsdevice_select_plot", (DL_FUNC)&ide_graphicsdevice_select_plot, 3},
                {"Microsoft.R.Host::External.ide_graphicsdevice_get_device_id", (DL_FUNC)&ide_graphicsdevice_get_device_id, 1},
                {"Microsoft.R.Host::External.ide_graphicsdevice_get_device_num", (DL_FUNC)&ide_graphicsdevice_get_device_num, 1},
                {"Microsoft.R.Host::External.ide_graphicsdevice_get_active_plot_id", (DL_FUNC)&ide_graphicsdevice_get_active_plot_id, 1},
                {}
            };

            void init(DllInfo *dll) {
                R_ExternalMethodDef* external_methods;
                void (*process_pending_render)(bool immediately);

                switch (int ver = R_GE_getVersion()) {
                case 10:
                    external_methods = external_methods_impl<10>::external_methods;
                    process_pending_render = ide_device<10>::process_pending_render;
                    break;
                case 11:
                    external_methods = external_methods_impl<11>::external_methods;
                    process_pending_render = ide_device<11>::process_pending_render;
                    break;
                case 12:
                    external_methods = external_methods_impl<12>::external_methods;
                    process_pending_render = ide_device<12>::process_pending_render;
                    break;
                default:
                    log::fatal_error("Unsupported GD API version %d", ver);
                }

                rhost::exports::add_external_methods(external_methods);

                rhost::host::callback_started.connect([=] {
                    process_pending_render(false);
                });
                rhost::host::readconsole_done.connect([=] {
                    process_pending_render(true);
                });
            }
        }
    }
}
