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

using namespace rhost::util;
using namespace rhost::host;

namespace rhost {
    namespace grdevices {
        namespace vs {
            class plot {
            public:
                plot(pDevDesc dd);

                void set_pending_render();
                bool has_pending_render() const;
                bool pending_render_timeout_elapsed() const;
                void render();
                void render_from_snapshot();

            private:
                boost::uuids::uuid _plot_id;
                pDevDesc _device_desc;
                bool _has_pending_render;
                boost::posix_time::ptime _last_pending_render_time;
                double _snapshot_render_width;
                double _snapshot_render_height;
                std::string _snapshot_render_filename;
                std::string _snapshot_filename;
            };

            class plot_history {
            public:
                plot_history(pDevDesc dd);

                std::shared_ptr<plot> get_active();
                void move_next();
                void move_previous();

                void append(std::shared_ptr<plot> plot);
                void clear();

            private:
                std::vector<std::shared_ptr<plot>> _plots;
                int _active_index;
                pDevDesc _device_desc;
            };

            class delegating_device : public graphics_device {
            public:
                static delegating_device * create(std::string device_type, double width, double height);

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
                double width() { return _width; }
                double height() { return _height; }
                std::string save();
                void send(std::string filename);

                void history_next();
                void history_previous();

            private:
                delegating_device(pDevDesc dd, std::string device_type, double width, double height);
                virtual ~delegating_device();

                pDevDesc get_or_create_delegated_device();
                pDevDesc create_delegated_device();
                void sync_delegated_device();
                void output_and_kill_delegated_device();
                void set_pending_render();

                static pDevDesc create_delegated_device(const std::string& device_type, const std::string& filename, double width, double height);

            private:
                double _width;
                double _height;
                bool _debug;
                pDevDesc _delegated_device;
                std::string _delegated_device_type;
                std::string _delegated_device_filename;
                boost::uuids::uuid _device_id;
                //plot* _active_plot;
                plot_history _history;
            };


            static std::set<delegating_device*> delegating_devices;
            static double default_width = 360;
            static double default_height = 360;

            ///////////////////////////////////////////////////////////////////////
            // Delegating device plot
            ///////////////////////////////////////////////////////////////////////

            plot::plot(pDevDesc dd) {
                _plot_id = boost::uuids::random_generator()();
                _device_desc = dd;
                _has_pending_render = false;
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

                auto xdd = reinterpret_cast<delegating_device*>(_device_desc->deviceSpecific);
                auto path = xdd->save();
                xdd->send(path);

                _snapshot_render_filename = path;
                _snapshot_render_width = xdd->width();
                _snapshot_render_height = xdd->height();
            }

            void plot::render_from_snapshot() {
                // TODO: check if the snapshot render matches requested size, if not, render again from snapshot
                auto xdd = reinterpret_cast<delegating_device*>(_device_desc->deviceSpecific);
                xdd->send(_snapshot_render_filename);
            }

            ///////////////////////////////////////////////////////////////////////
            // Delegating device plot history
            ///////////////////////////////////////////////////////////////////////

            plot_history::plot_history(pDevDesc dd) {
                _device_desc = dd;
                _active_index = -1;
            }

            std::shared_ptr<plot> plot_history::get_active() {
                if (_active_index < 0 || _active_index >= _plots.size()) {
                    return nullptr;
                }
                return _plots.at(_active_index);
            }

            void plot_history::move_next() {
                if (_active_index < (_plots.size() - 1)) {
                    _active_index++;
                }
            }

            void plot_history::move_previous() {
                if (_active_index > 0) {
                    _active_index--;
                }
            }

            void plot_history::append(std::shared_ptr<plot> plot) {
                _plots.push_back(plot);
                _active_index = (int)_plots.size() - 1;
            }

            void plot_history::clear() {
                // TODO
            }

            ///////////////////////////////////////////////////////////////////////
            // Delegating device
            ///////////////////////////////////////////////////////////////////////

            delegating_device * delegating_device::create(std::string device_type, double width, double height) {
                pDevDesc dd = static_cast<pDevDesc>(rhost::msvcrt::malloc(sizeof(DevDesc)));
                memset(dd, 0, sizeof(DevDesc));

                delegating_device * xdd = new delegating_device(dd, device_type, width, height);
                dd->deviceSpecific = xdd;

                pDevDesc delegated_dd = xdd->create_delegated_device();

                xdd->_delegated_device = delegated_dd;

                // Our delegating device needs to look like an instance of the 
                // delegated device, so we copy all of its attributes
                dd->left = delegated_dd->left;
                dd->right = delegated_dd->right;
                dd->bottom = delegated_dd->bottom;
                dd->top = delegated_dd->top;

                dd->clipLeft = delegated_dd->clipLeft;
                dd->clipRight = delegated_dd->clipRight;
                dd->clipBottom = delegated_dd->clipBottom;
                dd->clipTop = delegated_dd->clipTop;

                dd->xCharOffset = delegated_dd->xCharOffset;
                dd->yCharOffset = delegated_dd->yCharOffset;
                dd->yLineBias = delegated_dd->yLineBias;

                dd->ipr[0] = delegated_dd->ipr[0];
                dd->ipr[1] = delegated_dd->ipr[1];
                dd->cra[0] = delegated_dd->cra[0];
                dd->cra[1] = delegated_dd->cra[1];
                dd->gamma = delegated_dd->gamma;

                dd->canClip = delegated_dd->canClip;
                dd->canChangeGamma = delegated_dd->canChangeGamma;
                dd->canHAdj = delegated_dd->canHAdj;

                dd->startps = delegated_dd->startps;
                dd->startcol = delegated_dd->startcol;
                dd->startfill = delegated_dd->startfill;
                dd->startlty = delegated_dd->startlty;
                dd->startfont = delegated_dd->startfont;
                dd->startgamma = delegated_dd->startgamma;

                dd->hasTextUTF8 = delegated_dd->hasTextUTF8;
                dd->wantSymbolUTF8 = delegated_dd->wantSymbolUTF8;
                dd->useRotatedTextInContour = delegated_dd->useRotatedTextInContour;

                dd->haveTransparency = delegated_dd->haveTransparency;
                dd->haveTransparentBg = delegated_dd->haveTransparentBg;
                dd->haveRaster = delegated_dd->haveRaster;
                dd->haveCapture = delegated_dd->haveCapture;
                dd->haveLocator = delegated_dd->haveLocator;

                dd->displayListOn = R_TRUE;
                dd->canGenMouseDown = R_FALSE;
                dd->canGenMouseMove = R_FALSE;
                dd->canGenMouseUp = R_FALSE;
                dd->canGenKeybd = R_FALSE;

                return xdd;
            }

            void delegating_device::activate() {
            }

            void delegating_device::circle(double x, double y, double r, pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->circle != nullptr) {
                    dev->circle(x, y, r, gc, dev);
                }
            }

            void delegating_device::clip(double x0, double x1, double y0, double y1) {
                auto dev = get_or_create_delegated_device();
                if (dev->clip != nullptr) {
                    dev->clip(x0, x1, y0, y1, dev);
                }
            }

            void delegating_device::close() {
                delegating_devices.erase(this);
            }

            void delegating_device::deactivate() {
            }

            Rboolean delegating_device::locator(double *x, double *y) {
                *x = 0;
                *y = 0;
                return R_FALSE;
            }

            void delegating_device::line(double x1, double y1, double x2, double y2, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->line != nullptr) {
                    dev->line(x1, y1, x2, y2, gc, dev);
                }
            }

            void delegating_device::metric_info(int c, const pGEcontext gc, double* ascent, double* descent, double* width) {
                *ascent = 0;
                *descent = 0;
                *width = 0;

                auto dev = get_or_create_delegated_device();
                if (dev->metricInfo != nullptr) {
                    dev->metricInfo(c, gc, ascent, descent, width, dev);
                }
            }

            void delegating_device::mode(int mode) {
                auto dev = get_or_create_delegated_device();
                if (dev->mode != nullptr) {
                    dev->mode(mode, dev);
                }

                set_pending_render();
            }

            void delegating_device::new_page(const pGEcontext gc) {
                //_active_plot = new plot(device_desc);
                _history.append(std::shared_ptr<plot>(new plot(device_desc)));

                auto dev = get_or_create_delegated_device();
                if (dev->newPage != nullptr) {
                    dev->newPage(gc, dev);
                }
            }

            void delegating_device::polygon(int n, double *x, double *y, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->polygon != nullptr) {
                    dev->polygon(n, x, y, gc, dev);
                }
            }

            void delegating_device::polyline(int n, double *x, double *y, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->polyline != nullptr) {
                    dev->polyline(n, x, y, gc, dev);
                }
            }

            void delegating_device::rect(double x0, double y0, double x1, double y1, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->rect != nullptr) {
                    dev->rect(x0, y0, x1, y1, gc, dev);
                }
            }

            void delegating_device::path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->path) {
                    dev->path(x, y, npoly, nper, winding, gc, dev);
                }
            }

            void delegating_device::raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->raster != nullptr) {
                    dev->raster(raster, w, h, x, y, width, height, rot, interpolate, gc, dev);
                }
            }

            SEXP delegating_device::cap() {
                return R_NilValue;
            }

            void delegating_device::size(double *left, double *right, double *bottom, double *top) {
                auto dev = get_or_create_delegated_device();
                if (dev->size != nullptr) {
                    dev->size(left, right, bottom, top, dev);
                }
            }

            double delegating_device::str_width(const char *str, const pGEcontext gc) {
                double width = 0;

                auto dev = get_or_create_delegated_device();
                if (dev->strWidth != nullptr) {
                    width = dev->strWidth(str, gc, dev);
                }
                return width;
            }

            void delegating_device::text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->text != nullptr) {
                    dev->text(x, y, str, rot, hadj, gc, dev);
                }
            }

            void delegating_device::on_exit() {
            }

            Rboolean delegating_device::new_frame_confirm() {
                return R_FALSE;
            }

            void delegating_device::text_utf8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                auto dev = get_or_create_delegated_device();
                if (dev->textUTF8 != nullptr) {
                    dev->textUTF8(x, y, str, rot, hadj, gc, dev);
                }
            }

            double delegating_device::str_width_utf8(const char *str, const pGEcontext gc) {
                double width = 0;

                auto dev = get_or_create_delegated_device();
                if (dev->strWidthUTF8 != nullptr) {
                    width = dev->strWidthUTF8(str, gc, dev);
                }

                return width;
            }

            void delegating_device::event_helper(int code) {
            }

            int delegating_device::hold_flush(int level) {
                return 0;
            }

            void delegating_device::render_request(bool immediately) {
                //if (_active_plot != nullptr && _active_plot->has_pending_render()) {
                //    if (immediately || _active_plot->pending_render_timeout_elapsed()) {
                //        _active_plot->render();
                //    }
                //}

                auto plot = _history.get_active();
                if (plot != nullptr) {
                    if (plot->has_pending_render()) {
                        if (immediately || plot->pending_render_timeout_elapsed()) {
                            plot->render();
                        }
                    }
                }
            }

            void delegating_device::resize(double width, double height) {
                output_and_kill_delegated_device();

                _width = width;
                _height = height;

                _delegated_device = create_delegated_device();

                pGEDevDesc ge_dev_desc = Rf_desc2GEDesc(device_desc);
                GEplayDisplayList(ge_dev_desc);

                render_request(true);
            }

            std::string delegating_device::save() {
                auto path = _delegated_device_filename;
                sync_delegated_device();
                output_and_kill_delegated_device();
                return path;
            }

            void delegating_device::send(std::string path) {
                printf("VSDGD: plot='%s'\n", path.c_str());
                with_cancellation([&] {
                    send_message("Plot", to_utf8(path));
                });
            }

            delegating_device::delegating_device(pDevDesc dd, std::string device_type, double width, double height) :
                graphics_device(dd),
                _width(width),
                _height(height),
                _debug(false),
                _history(dd),
                //_active_plot(nullptr),
                _delegated_device(nullptr),
                _delegated_device_type(device_type),
                _delegated_device_filename(""),
                _device_id(boost::uuids::random_generator()()) {
            }

            delegating_device::~delegating_device() {
                //delete _active_plot;
            }

            pDevDesc delegating_device::get_or_create_delegated_device() {
                if (_delegated_device == nullptr) {
                    _delegated_device = create_delegated_device();
                    sync_delegated_device();
                }
                return _delegated_device;
            }

            pDevDesc delegating_device::create_delegated_device() {
                _delegated_device_filename = rhost::util::get_temp_file_path() + std::string(".") + _delegated_device_type;
                return create_delegated_device(_delegated_device_type, _delegated_device_filename, _width, _height);
            }

            void delegating_device::sync_delegated_device() {
                int delegated_device_num = Rf_ndevNumber(_delegated_device);
                int delegating_device_num = Rf_ndevNumber(device_desc);

                Rf_selectDevice(delegated_device_num);
                GEcopyDisplayList(delegating_device_num);
                Rf_selectDevice(delegating_device_num);
            }

            void delegating_device::output_and_kill_delegated_device() {
                // The device number is not constant, so get the current number
                int delegated_device_num = Rf_ndevNumber(_delegated_device);

                // Killing the device will call close, which will save the file to disk
                pGEDevDesc ge_dev_desc = GEgetDevice(delegated_device_num);
                GEkillDevice(ge_dev_desc);

                // Blank our state, next call to graphics primitive (if any) will create 
                // a new delegated device on demand
                _delegated_device = nullptr;
                _delegated_device_filename = "";
            }

            void delegating_device::set_pending_render() {
                //if (_active_plot != nullptr) {
                //    _active_plot->set_pending_render();
                //}

                auto plot = _history.get_active();
                if (plot != nullptr) {
                    plot->set_pending_render();
                }
            }

            void delegating_device::history_next() {
                _history.move_next();
                auto plot = _history.get_active();
                if (plot != nullptr) {
                    plot->render_from_snapshot();
                }
            }

            void delegating_device::history_previous() {
                _history.move_previous();
                auto plot = _history.get_active();
                if (plot != nullptr) {
                    plot->render_from_snapshot();
                }
            }

            pDevDesc delegating_device::create_delegated_device(const std::string& device_type, const std::string& filename, double width, double height) {
                std::string fixed_filename(filename);
                boost::algorithm::replace_all(fixed_filename, "\\", "/");

                auto expr = boost::format("%1%(filename='%2%', width=%3%, height=%4%)") % device_type % fixed_filename % width % height;

                // Create the delegated device via the public R API
                ParseStatus ps;
                auto before = [&] {};
                auto after = [&] {};
                auto result = rhost::eval::r_try_eval_str(expr.str(), R_GlobalEnv, ps, before, after);
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
                // We check every delegating device to give them opportunity to save to disk
                // There appears to be no way of querying the type of device, so we maintain
                // our own list.
                auto now = boost::posix_time::second_clock::local_time();
                for (auto it = delegating_devices.begin(); it != delegating_devices.end(); ++it) {
                    (*it)->render_request(immediately);
                }
            }

            static int get_delegating_device_num() {
                int device_num = 0;

                int active_device_num = Rf_curDevice();
                for (auto it = delegating_devices.begin(); it != delegating_devices.end(); ++it) {
                    int this_device_num = Rf_ndevNumber((*it)->device_desc);
                    if (this_device_num == active_device_num) {
                        device_num = this_device_num;
                    }
                }

                return device_num;
            }

            static delegating_device* get_delegating_device() {
                int device_num = get_delegating_device_num();
                if (device_num != 0) {
                    pGEDevDesc ge_dev_desc = GEgetDevice(device_num);
                    pDevDesc dd = ge_dev_desc->dev;
                    return reinterpret_cast<delegating_device*>(dd->deviceSpecific);
                }

                return nullptr;
            }

            ///////////////////////////////////////////////////////////////////////
            // Exported R routines
            ///////////////////////////////////////////////////////////////////////

            extern "C" SEXP visualstudio_graphicsdevice_new(SEXP args) {
                R_GE_checkVersionOrDie(R_GE_version);

                R_CheckDeviceAvailable();
                BEGIN_SUSPEND_INTERRUPTS{
                    auto dev = delegating_device::create("png", default_width, default_height);
                    pGEDevDesc gdd = GEcreateDevDesc(dev->device_desc);
                    GEaddDevice2(gdd, "visualstudio");
                    delegating_devices.insert(dev);
                } END_SUSPEND_INTERRUPTS;

                return R_NilValue;
            }

            extern "C" SEXP visualstudio_graphicsdevice_resize(SEXP args) {
                args = CDR(args);
                SEXP width = CAR(args);
                args = CDR(args);
                SEXP height = CAR(args);

                double *w = REAL(width);
                double *h = REAL(height);

                default_width = *w;
                default_height = *h;

                int device_num = get_delegating_device_num();
                if (device_num != 0) {
                    pGEDevDesc ge_dev_desc = GEgetDevice(device_num);
                    pDevDesc dd = ge_dev_desc->dev;
                    reinterpret_cast<delegating_device*>(dd->deviceSpecific)->resize(*w, *h);
                }

                return R_NilValue;
            }

            extern "C" SEXP visualstudio_graphicsdevice_next_plot(SEXP args) {
                auto device = get_delegating_device();
                if (device != nullptr) {
                    device->history_next();
                }
                return R_NilValue;
            }

            extern "C" SEXP visualstudio_graphicsdevice_previous_plot(SEXP args) {
                auto device = get_delegating_device();
                if (device != nullptr) {
                    device->history_previous();
                }
                return R_NilValue;
            }

            static R_ExternalMethodDef external_methods[] = {
                { "rtvs::External.visualstudio_graphicsdevice_new", (DL_FUNC)&visualstudio_graphicsdevice_new, 0 },
                { "rtvs::External.visualstudio_graphicsdevice_resize", (DL_FUNC)&visualstudio_graphicsdevice_resize, 2 },
                { "rtvs::External.visualstudio_graphicsdevice_next_plot", (DL_FUNC)&visualstudio_graphicsdevice_next_plot, 0 },
                { "rtvs::External.visualstudio_graphicsdevice_previous_plot", (DL_FUNC)&visualstudio_graphicsdevice_previous_plot, 0 },
                { }
            };

            void init(DllInfo *dll) {
                rhost::exports::add_external_methods(external_methods);

                rhost::host::callback_started.connect(boost::bind(process_pending_render, false));
                rhost::host::readconsole_done.connect(boost::bind(process_pending_render, true));
            }
        }
    }
}
