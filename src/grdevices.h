#pragma once

#include "stdafx.h"
#include "Rgraphicsapi.h"
#include "util.h"
#include "grdevices.h"

namespace rhost {
    namespace grdevices {
        ///////////////////////////////////////////////////////////////////
        // Base class for graphics devices
        ///////////////////////////////////////////////////////////////////
        class graphics_device {
        public:
            pDevDesc device_desc;

            virtual void activate() = 0;
            virtual void circle(double x, double y, double r, pGEcontext gc) = 0;
            virtual void clip(double x0, double x1, double y0, double y1) = 0;
            virtual void close() = 0;
            virtual void deactivate() = 0;
            virtual Rboolean locator(double *x, double *y) = 0;
            virtual void line(double x1, double y1, double x2, double y2, const pGEcontext gc) = 0;
            virtual void metric_info(int c, const pGEcontext gc, double* ascent, double* descent, double* width) = 0;
            virtual void mode(int mode) = 0;
            virtual void new_page(const pGEcontext gc) = 0;
            virtual void polygon(int n, double *x, double *y, const pGEcontext gc) = 0;
            virtual void polyline(int n, double *x, double *y, const pGEcontext gc) = 0;
            virtual void rect(double x0, double y0, double x1, double y1, const pGEcontext gc) = 0;
            virtual void path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc) = 0;
            virtual void raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc) = 0;
            virtual SEXP cap() = 0;
            virtual void size(double *left, double *right, double *bottom, double *top) = 0;
            virtual double str_width(const char *str, const pGEcontext gc) = 0;
            virtual void text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) = 0;
            virtual void on_exit() = 0;
            virtual Rboolean new_frame_confirm() = 0;
            virtual void text_utf8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) = 0;
            virtual double str_width_utf8(const char *str, const pGEcontext gc) = 0;
            virtual void event_helper(int code) = 0;
            virtual int hold_flush(int level) = 0;

        protected:
            graphics_device(pDevDesc dd) :
                device_desc(dd)
            {
                device_desc->activate = [](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->activate();
                    });
                };
                device_desc->circle = [](double x, double y, double r, pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->circle(x, y, r, gc);
                    });
                };
                device_desc->clip = [](double x0, double x1, double y0, double y1, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->clip(x0, x1, y0, y1);
                    });
                };
                device_desc->close = [](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        reinterpret_cast<graphics_device*>(dd->deviceSpecific)->close();
                        delete reinterpret_cast<graphics_device*>(dd->deviceSpecific);
                    });
                };
                device_desc->deactivate = [](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->deactivate();
                    });
                };
                device_desc->locator = [](double *x, double *y, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->locator(x, y);
                    });
                };
                device_desc->line = [](double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->line(x1, y1, x2, y2, gc);
                    });
                };
                device_desc->metricInfo = [](int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->metric_info(c, gc, ascent, descent, width);
                    });
                };
                device_desc->mode = [](int mode, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->mode(mode);
                    });
                };
                device_desc->newPage = [](const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->new_page(gc);
                    });
                };
                device_desc->polygon = [](int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->polygon(n, x, y, gc);
                    });
                };
                device_desc->polyline = [](int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->polyline(n, x, y, gc);
                    });
                };
                device_desc->rect = [](double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->rect(x0, y0, x1, y1, gc);
                    });
                };
                device_desc->path = [](double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->path(x, y, npoly, nper, winding, gc);
                    });
                };
                device_desc->raster = [](unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->raster(raster, w, h, x, y, width, height, rot, interpolate, gc);
                    });
                };
                device_desc->cap = [](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->cap();
                    });
                };
                device_desc->size = [](double *left, double *right, double *bottom, double *top, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->size(left, right, bottom, top);
                    });
                };
                device_desc->strWidth = [](const char *str, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->str_width(str, gc);
                    });
                };
                device_desc->text = [](double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->text(x, y, str, rot, hadj, gc);
                    });
                };
                device_desc->onExit = [](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->on_exit();
                    });
                };
                device_desc->newFrameConfirm = [](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->new_frame_confirm();
                    });
                };
                device_desc->textUTF8 = [](double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->text_utf8(x, y, str, rot, hadj, gc);
                    });
                };
                device_desc->strWidthUTF8 = [](const char *str, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->str_width_utf8(str, gc);
                    });
                };
                device_desc->eventHelper = [](pDevDesc dd, int code) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->event_helper(code);
                    });
                };
                device_desc->holdflush = [](pDevDesc dd, int level) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(dd->deviceSpecific)->hold_flush(level);
                    });
                };
            }

            virtual ~graphics_device()
            {
            }
        };
    }
}
