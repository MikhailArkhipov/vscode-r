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
#include "Rgraphicsapi.h"
#include "host.h"
#include "msvcrt.h"
#include "eval.h"
#include "util.h"
#include "grdevices.h"
#include "exports.h"

namespace rhost {
    namespace grdevices {
        namespace ide {
            class plot {
            public:
                plot(pDevDesc dd);
                virtual ~plot();

                void set_pending_render();
                bool has_pending_render() const;
                bool pending_render_timeout_elapsed() const;
                void render();
                void render_from_display_list();
                void render_from_snapshot(double width, double height);

            private:
                std::string get_snapshot_varname();
                void save_snapshot_variable();
                void replay_snapshot_variable();

            private:
                boost::uuids::uuid _plot_id;
                pDevDesc _device_desc;
                bool _has_pending_render;
                boost::posix_time::ptime _last_pending_render_time;
                double _snapshot_render_width;
                double _snapshot_render_height;
                std::string _snapshot_render_filename;
                std::string _snapshot_varname;
            };

            class plot_history {
            public:
                plot_history(pDevDesc dd);

                plot* get_active() const;
                void move_next();
                void move_previous();

                void new_page();
                void append(std::unique_ptr<plot> plot);
                void clear();

                void resize(double width, double height);
                void render_from_snapshot(double width, double height);

                int plot_count() const;
                int active_plot_index() const;

            private:
                std::vector<std::unique_ptr<plot>>::iterator _active_plot;
                std::vector<std::unique_ptr<plot>> _plots;
                pDevDesc _device_desc;
                bool _replaying;
            };

            class ide_device : public graphics_device {
            public:
                static std::unique_ptr<ide_device> create(std::string device_type, double width, double height);

                ide_device(pDevDesc dd, std::string device_type, double width, double height);
                virtual ~ide_device();

                boost::signals2::signal<void()> closed;

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
                void render_request(bool immediately);
                void resize(double width, double height);
                double width() const { return _width; }
                double height() const { return _height; }
                std::string save();
                void send(std::string filename);

                void history_next();
                void history_previous();

                int plot_count() const;
                int active_plot_index() const;

                void select();
                void output_and_kill_file_device();

            private:
                std::string get_render_file_path();
                pDevDesc get_or_create_file_device();
                pDevDesc create_file_device();
                void sync_file_device();
                void set_pending_render();

                static pDevDesc create_file_device(const std::string& device_type, const std::string& filename, double width, double height);

            private:
                double _width;
                double _height;
                bool _debug;
                pDevDesc _file_device;
                std::string _file_device_type;
                std::string _file_device_filename;
                plot_history _history;
            };


            static ide_device* device_instance = nullptr;
            static boost::uuids::random_generator uuid_generator;
            static double default_width = 360;
            static double default_height = 360;

            ///////////////////////////////////////////////////////////////////////
            // Ide device plot
            ///////////////////////////////////////////////////////////////////////

            plot::plot(pDevDesc dd) :
                _plot_id(uuid_generator()),
                _device_desc(dd),
                _has_pending_render(false) {
            }

            plot::~plot() {
                if (!_snapshot_render_filename.empty()) {
                    remove(_snapshot_render_filename.c_str());
                }
            }

            void plot::set_pending_render() {
                _has_pending_render = true;
                _last_pending_render_time = boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
            }

            bool plot::has_pending_render() const {
                return _has_pending_render;
            }

            bool plot::pending_render_timeout_elapsed() const {
                auto now = boost::posix_time::second_clock::local_time();
                auto elapsed = now - _last_pending_render_time;
                return elapsed.total_milliseconds() >= 50;
            }

            void plot::render() {
                if (!has_pending_render()) {
                    return;
                }

                _has_pending_render = false;

                auto xdd = reinterpret_cast<ide_device*>(_device_desc->deviceSpecific);
                auto path = xdd->save();

                if (!_snapshot_render_filename.empty()) {
                    remove(_snapshot_render_filename.c_str());
                }

                _snapshot_render_filename = path;
                _snapshot_render_width = xdd->width();
                _snapshot_render_height = xdd->height();

                if (_snapshot_varname.empty()) {
                    _snapshot_varname = get_snapshot_varname();
                    save_snapshot_variable();
                }

                xdd->send(path);
            }

            void plot::render_from_display_list() {
                pGEDevDesc ge_dev_desc = Rf_desc2GEDesc(_device_desc);
                GEplayDisplayList(ge_dev_desc);

                render();
            }

            void plot::render_from_snapshot(double width, double height) {
                // Check if we already have a rendered file for the requested size
                if (_snapshot_render_width == width && _snapshot_render_height == height) {
                    auto xdd = reinterpret_cast<ide_device*>(_device_desc->deviceSpecific);
                    xdd->send(_snapshot_render_filename);
                }
                else {
                    replay_snapshot_variable();
                }
            }

            void plot::replay_snapshot_variable() {
                auto xdd = reinterpret_cast<ide_device*>(_device_desc->deviceSpecific);
                xdd->output_and_kill_file_device();

                rhost::util::protected_sexp snapshot(Rf_findVar(Rf_install(_snapshot_varname.c_str()), R_GlobalEnv));
                pGEDevDesc ge_dev_desc = Rf_desc2GEDesc(_device_desc);

                GEplaySnapshot(snapshot.get(), ge_dev_desc);
            }

            void plot::save_snapshot_variable() {
                pGEDevDesc ge_dev_desc = Rf_desc2GEDesc(_device_desc);
                rhost::util::protected_sexp snapshot(GEcreateSnapshot(ge_dev_desc));

                rhost::util::protected_sexp klass(Rf_mkString("recordedplot"));
                Rf_classgets(snapshot.get(), klass.get());

                Rf_defineVar(Rf_install(_snapshot_varname.c_str()), snapshot.get(), R_GlobalEnv);
            }

            std::string plot::get_snapshot_varname() {
                std::string name = std::string(".SavedPlot") + boost::uuids::to_string(_plot_id);
                boost::algorithm::replace_all(name, "-", "");
                return name;
            }

            ///////////////////////////////////////////////////////////////////////
            // Ide device plot history
            ///////////////////////////////////////////////////////////////////////

            plot_history::plot_history(pDevDesc dd) :
                _device_desc(dd),
                _replaying(false) {
                _active_plot = _plots.begin();
            }

            plot* plot_history::get_active() const {
                if (_active_plot == _plots.end()) {
                    return nullptr;
                }

                return (*_active_plot).get();
            }

            void plot_history::move_next() {
                auto end = _plots.end();
                if (_active_plot != end) {
                    auto next = std::next(_active_plot);
                    if (next != end) {
                        _active_plot = next;
                    }
                }
            }

            void plot_history::move_previous() {
                if (_active_plot != _plots.begin()) {
                    _active_plot--;
                }
            }

            void plot_history::new_page() {
                if (!_replaying) {
                    pGEDevDesc ge_dev_desc = Rf_desc2GEDesc(_device_desc);
                    auto snapshot = ge_dev_desc->savedSnapshot;
                    append(std::make_unique<plot>(_device_desc));
                    // TODO: save snapshot
                }
            }

            void plot_history::append(std::unique_ptr<plot> p) {
                _plots.push_back(std::move(p));
                _active_plot = std::prev(_plots.end(), 1);
            }

            void plot_history::clear() {
                // TODO
            }

            void plot_history::resize(double width, double height) {
                if (_active_plot == std::prev(_plots.end(), 1)) {
                    _replaying = true;
                    auto plot = _active_plot->get();
                    plot->render_from_display_list();
                    _replaying = false;
                }
                else {
                    render_from_snapshot(width, height);
                }
            }

            void plot_history::render_from_snapshot(double width, double height) {
                _replaying = true;
                auto plot = _active_plot->get();
                if (plot != nullptr) {
                    plot->render_from_snapshot(width, height);
                }
                _replaying = false;
            }

            int plot_history::plot_count() const {
                return (int)_plots.size();
            }

            int plot_history::active_plot_index() const {
                if (_active_plot == _plots.end()) {
                    return -1;
                }

                int index = static_cast<int>(_active_plot - _plots.begin());
                return index;
            }

            ///////////////////////////////////////////////////////////////////////
            // Ide device
            ///////////////////////////////////////////////////////////////////////

            std::unique_ptr<ide_device> ide_device::create(std::string device_type, double width, double height) {
                pDevDesc dd = static_cast<pDevDesc>(rhost::msvcrt::calloc(1, sizeof(DevDesc)));

                auto xdd = std::make_unique<ide_device>(dd, device_type, width, height);

                pDevDesc file_dd = xdd->create_file_device();

                xdd.get()->_file_device = file_dd;

                // Our ide device needs to look like an instance of the 
                // file device, so we copy most of its attributes
                // (don't overwrite the callbacks that are already assigned).
                dd->left = file_dd->left;
                dd->right = file_dd->right;
                dd->bottom = file_dd->bottom;
                dd->top = file_dd->top;

                dd->clipLeft = file_dd->clipLeft;
                dd->clipRight = file_dd->clipRight;
                dd->clipBottom = file_dd->clipBottom;
                dd->clipTop = file_dd->clipTop;

                dd->xCharOffset = file_dd->xCharOffset;
                dd->yCharOffset = file_dd->yCharOffset;
                dd->yLineBias = file_dd->yLineBias;

                dd->ipr[0] = file_dd->ipr[0];
                dd->ipr[1] = file_dd->ipr[1];
                dd->cra[0] = file_dd->cra[0];
                dd->cra[1] = file_dd->cra[1];
                dd->gamma = file_dd->gamma;

                dd->canClip = file_dd->canClip;
                dd->canChangeGamma = file_dd->canChangeGamma;
                dd->canHAdj = file_dd->canHAdj;

                dd->startps = file_dd->startps;
                dd->startcol = file_dd->startcol;
                dd->startfill = file_dd->startfill;
                dd->startlty = file_dd->startlty;
                dd->startfont = file_dd->startfont;
                dd->startgamma = file_dd->startgamma;

                dd->hasTextUTF8 = file_dd->hasTextUTF8;
                dd->wantSymbolUTF8 = file_dd->wantSymbolUTF8;
                dd->useRotatedTextInContour = file_dd->useRotatedTextInContour;

                dd->haveTransparency = file_dd->haveTransparency;
                dd->haveTransparentBg = file_dd->haveTransparentBg;
                dd->haveRaster = file_dd->haveRaster;
                dd->haveCapture = file_dd->haveCapture;
                dd->haveLocator = file_dd->haveLocator;

                dd->displayListOn = R_TRUE;
                dd->canGenMouseDown = R_FALSE;
                dd->canGenMouseMove = R_FALSE;
                dd->canGenMouseUp = R_FALSE;
                dd->canGenKeybd = R_FALSE;

                dd->deviceSpecific = xdd.get();

                return xdd;
            }

            void ide_device::activate() {
            }

            void ide_device::circle(double x, double y, double r, pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->circle != nullptr) {
                    dev->circle(x, y, r, gc, dev);
                }
            }

            void ide_device::clip(double x0, double x1, double y0, double y1) {
                auto dev = get_or_create_file_device();
                if (dev->clip != nullptr) {
                    dev->clip(x0, x1, y0, y1, dev);
                }
            }

            void ide_device::close() {
                // send an 'empty' plot to ide to clear the plot window
                send("");

                closed();
                delete this;
            }

            void ide_device::deactivate() {
            }

            Rboolean ide_device::locator(double *x, double *y) {
                *x = 0;
                *y = 0;
                return R_FALSE;
            }

            void ide_device::line(double x1, double y1, double x2, double y2, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->line != nullptr) {
                    dev->line(x1, y1, x2, y2, gc, dev);
                }
            }

            void ide_device::metric_info(int c, const pGEcontext gc, double* ascent, double* descent, double* width) {
                *ascent = 0;
                *descent = 0;
                *width = 0;

                auto dev = get_or_create_file_device();
                if (dev->metricInfo != nullptr) {
                    dev->metricInfo(c, gc, ascent, descent, width, dev);
                }
            }

            void ide_device::mode(int mode) {
                auto dev = get_or_create_file_device();
                if (dev->mode != nullptr) {
                    dev->mode(mode, dev);
                }

                set_pending_render();
            }

            void ide_device::new_page(const pGEcontext gc) {
                _history.new_page();

                auto dev = get_or_create_file_device();
                if (dev->newPage != nullptr) {
                    dev->newPage(gc, dev);
                }
            }

            void ide_device::polygon(int n, double *x, double *y, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->polygon != nullptr) {
                    dev->polygon(n, x, y, gc, dev);
                }
            }

            void ide_device::polyline(int n, double *x, double *y, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->polyline != nullptr) {
                    dev->polyline(n, x, y, gc, dev);
                }
            }

            void ide_device::rect(double x0, double y0, double x1, double y1, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->rect != nullptr) {
                    dev->rect(x0, y0, x1, y1, gc, dev);
                }
            }

            void ide_device::path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->path) {
                    dev->path(x, y, npoly, nper, winding, gc, dev);
                }
            }

            void ide_device::raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->raster != nullptr) {
                    dev->raster(raster, w, h, x, y, width, height, rot, interpolate, gc, dev);
                }
            }

            SEXP ide_device::cap() {
                return R_NilValue;
            }

            void ide_device::size(double *left, double *right, double *bottom, double *top) {
                auto dev = get_or_create_file_device();
                if (dev->size != nullptr) {
                    dev->size(left, right, bottom, top, dev);
                }
            }

            double ide_device::str_width(const char *str, const pGEcontext gc) {
                double width = 0;

                auto dev = get_or_create_file_device();
                if (dev->strWidth != nullptr) {
                    width = dev->strWidth(str, gc, dev);
                }
                return width;
            }

            void ide_device::text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->text != nullptr) {
                    dev->text(x, y, str, rot, hadj, gc, dev);
                }
            }

            void ide_device::on_exit() {
            }

            Rboolean ide_device::new_frame_confirm() {
                return R_FALSE;
            }

            void ide_device::text_utf8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                auto dev = get_or_create_file_device();
                if (dev->textUTF8 != nullptr) {
                    dev->textUTF8(x, y, str, rot, hadj, gc, dev);
                }
            }

            double ide_device::str_width_utf8(const char *str, const pGEcontext gc) {
                double width = 0;

                auto dev = get_or_create_file_device();
                if (dev->strWidthUTF8 != nullptr) {
                    width = dev->strWidthUTF8(str, gc, dev);
                }

                return width;
            }

            void ide_device::event_helper(int code) {
            }

            int ide_device::hold_flush(int level) {
                return 0;
            }

            void ide_device::render_request(bool immediately) {
                auto plot = _history.get_active();
                if (plot != nullptr) {
                    if (plot->has_pending_render()) {
                        if (immediately || plot->pending_render_timeout_elapsed()) {
                            plot->render();
                        }
                    }
                }
            }

            void ide_device::select() {
                auto num = Rf_ndevNumber(device_instance->device_desc);
                Rf_selectDevice(num);
            }

            void ide_device::resize(double width, double height) {
                output_and_kill_file_device();

                _width = width;
                _height = height;

                _file_device = create_file_device();

                _history.resize(width, height);
            }

            std::string ide_device::save() {
                auto path = _file_device_filename;
                sync_file_device();
                output_and_kill_file_device();
                return path;
            }

            void ide_device::send(std::string path) {
                rhost::log::logf("ide_device: plot='%s'\n", path.c_str());
                rhost::host::with_cancellation([&] {
                    rhost::host::send_message("Plot", rhost::util::to_utf8(path));
                });
            }

            ide_device::ide_device(pDevDesc dd, std::string device_type, double width, double height) :
                graphics_device(dd),
                _width(width),
                _height(height),
                _debug(false),
                _history(dd),
                _file_device(nullptr),
                _file_device_type(device_type) {
            }

            ide_device::~ide_device() {
            }

            std::string ide_device::get_render_file_path() {
                // TODO: change filename to use a uuid
                // TODO: decide if we really want them in the temp folder
                char folderpath[1024];
                char filepath[1024];
                GetTempPathA(1024, folderpath);
                GetTempFileNameA(folderpath, "rt", 0, filepath);

                return std::string(filepath) + std::string(".") + _file_device_type;
            }

            pDevDesc ide_device::get_or_create_file_device() {
                if (_file_device == nullptr) {
                    _file_device = create_file_device();
                    sync_file_device();
                }
                return _file_device;
            }

            pDevDesc ide_device::create_file_device() {
                _file_device_filename = get_render_file_path();
                return create_file_device(_file_device_type, _file_device_filename, _width, _height);
            }

            void ide_device::sync_file_device() {
                int file_device_num = Rf_ndevNumber(_file_device);
                int ide_device_num = Rf_ndevNumber(device_desc);

                Rf_selectDevice(file_device_num);
                GEcopyDisplayList(ide_device_num);
                Rf_selectDevice(ide_device_num);
            }

            void ide_device::output_and_kill_file_device() {
                // The device number is not constant, so get the current number
                int file_device_num = Rf_ndevNumber(_file_device);

                // Killing the device will call close, which will save the file to disk
                pGEDevDesc ge_dev_desc = GEgetDevice(file_device_num);
                GEkillDevice(ge_dev_desc);

                // Blank our state, next call to graphics primitive (if any) will create 
                // a new file device on demand
                _file_device = nullptr;
                _file_device_filename = "";
            }

            void ide_device::set_pending_render() {
                auto plot = _history.get_active();
                if (plot != nullptr) {
                    plot->set_pending_render();
                }
            }

            void ide_device::history_next() {
                _history.move_next();
                _history.render_from_snapshot(_width, _height);
            }

            void ide_device::history_previous() {
                _history.move_previous();
                _history.render_from_snapshot(_width, _height);
            }

            int ide_device::plot_count() const {
                return _history.plot_count();
            }

            int ide_device::active_plot_index() const {
                return _history.active_plot_index();
            }

            pDevDesc ide_device::create_file_device(const std::string& device_type, const std::string& filename, double width, double height) {
                std::string fixed_filename(filename);
                boost::algorithm::replace_all(fixed_filename, "\\", "/");

                auto expr = boost::format("%1%(filename='%2%', width=%3%, height=%4%)") % device_type % fixed_filename % width % height;

                // Create the file device via the public R API
                ParseStatus ps;
                auto result = rhost::eval::r_try_eval_str(expr.str(), R_GlobalEnv, ps);
                if (result.has_error) {
                    // TODO: throw exception
                }

                // Retrieve the device descriptor of the current device (the one created above)
                int device_num = Rf_curDevice();
                pGEDevDesc ge_dev_desc = GEgetDevice(device_num);
                pDevDesc dev_desc = ge_dev_desc->dev;
                return dev_desc;
            }

            static void process_pending_render(bool immediately) {
                if (device_instance != nullptr) {
                    device_instance->render_request(immediately);
                }
            }

            ///////////////////////////////////////////////////////////////////////
            // Exported R routines
            ///////////////////////////////////////////////////////////////////////

            extern "C" SEXP ide_graphicsdevice_new(SEXP args) {
                R_GE_checkVersionOrDie(R_GE_version);

                if (device_instance != nullptr) {
                    // TODO: issue some error
                    return R_NilValue;
                }

                R_CheckDeviceAvailable();
                BEGIN_SUSPEND_INTERRUPTS{
                    auto dev = ide_device::create("png", default_width, default_height);
                pGEDevDesc gdd = GEcreateDevDesc(dev->device_desc);
                GEaddDevice2(gdd, "ide");
                // Owner is DevDesc::deviceSpecific, and is released in close()
                dev->closed.connect([&] { device_instance = nullptr; });
                device_instance = dev.release();
                } END_SUSPEND_INTERRUPTS;

                return R_NilValue;
            }

            extern "C" SEXP ide_graphicsdevice_resize(SEXP args) {
                args = CDR(args);
                SEXP width = CAR(args);
                args = CDR(args);
                SEXP height = CAR(args);

                double *w = REAL(width);
                double *h = REAL(height);

                default_width = *w;
                default_height = *h;

                if (device_instance != nullptr) {
                    device_instance->select();
                    device_instance->resize(*w, *h);
                }

                return R_NilValue;
            }

            extern "C" SEXP ide_graphicsdevice_next_plot(SEXP args) {
                if (device_instance != nullptr) {
                    device_instance->select();
                    device_instance->history_next();
                }

                return R_NilValue;
            }

            extern "C" SEXP ide_graphicsdevice_previous_plot(SEXP args) {
                if (device_instance != nullptr) {
                    device_instance->select();
                    device_instance->history_previous();
                }

                return R_NilValue;
            }

            extern "C" SEXP ide_graphicsdevice_history_info(SEXP args) {
                // zero-based index active plot, number of plots
                auto value = Rf_allocVector(INTSXP, 2);
                if (device_instance != nullptr) {
                    INTEGER(value)[0] = device_instance->active_plot_index();
                    INTEGER(value)[1] = device_instance->plot_count();
                }
                else {
                    INTEGER(value)[0] = -1;
                    INTEGER(value)[1] = 0;
                }

                return value;
            }

            static R_ExternalMethodDef external_methods[] = {
                { "Microsoft.R.Host::External.ide_graphicsdevice_new", (DL_FUNC)&ide_graphicsdevice_new, 0 },
                { "Microsoft.R.Host::External.ide_graphicsdevice_resize", (DL_FUNC)&ide_graphicsdevice_resize, 2 },
                { "Microsoft.R.Host::External.ide_graphicsdevice_next_plot", (DL_FUNC)&ide_graphicsdevice_next_plot, 0 },
                { "Microsoft.R.Host::External.ide_graphicsdevice_previous_plot", (DL_FUNC)&ide_graphicsdevice_previous_plot, 0 },
                { "Microsoft.R.Host::External.ide_graphicsdevice_history_info", (DL_FUNC)&ide_graphicsdevice_history_info, 0 },
                {}
            };

            void init(DllInfo *dll) {
                rhost::exports::add_external_methods(external_methods);

                rhost::host::callback_started.connect([] { process_pending_render(false); });
                rhost::host::readconsole_done.connect([] { process_pending_render(true); });
            }
        }
    }
}
