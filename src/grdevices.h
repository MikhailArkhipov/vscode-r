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
#include "util.h"
#include "grdevices.h"
#include "devdesc_wrapper.h"

namespace rhost {
    namespace grdevices {
        ///////////////////////////////////////////////////////////////////
        // Base class for graphics devices
        ///////////////////////////////////////////////////////////////////
        class graphics_device {
        public:
            devdesc_wrapper dev;

            virtual ~graphics_device()
            {
            }

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
                dev(dd)
            {
                dev.set_activate([](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->activate();
                    });
                });
                dev.set_circle([](double x, double y, double r, pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->circle(x, y, r, gc);
                    });
                });
                dev.set_clip([](double x0, double x1, double y0, double y1, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->clip(x0, x1, y0, y1);
                    });
                });
                dev.set_close([](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->close();
                    });
                });
                dev.set_deactivate([](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->deactivate();
                    });
                });
                dev.set_locator([](double *x, double *y, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->locator(x, y);
                    });
                });
                dev.set_line([](double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->line(x1, y1, x2, y2, gc);
                    });
                });
                dev.set_metricInfo([](int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->metric_info(c, gc, ascent, descent, width);
                    });
                });
                dev.set_mode([](int mode, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->mode(mode);
                    });
                });
                dev.set_newPage([](const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->new_page(gc);
                    });
                });
                dev.set_polygon([](int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->polygon(n, x, y, gc);
                    });
                });
                dev.set_polyline([](int n, double *x, double *y, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->polyline(n, x, y, gc);
                    });
                });
                dev.set_rect([](double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->rect(x0, y0, x1, y1, gc);
                    });
                });
                dev.set_path([](double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->path(x, y, npoly, nper, winding, gc);
                    });
                });
                dev.set_raster([](unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->raster(raster, w, h, x, y, width, height, rot, interpolate, gc);
                    });
                });
                dev.set_cap([](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->cap();
                    });
                });
                dev.set_size([](double *left, double *right, double *bottom, double *top, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->size(left, right, bottom, top);
                    });
                });
                dev.set_strWidth([](const char *str, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->str_width(str, gc);
                    });
                });
                dev.set_text([](double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->text(x, y, str, rot, hadj, gc);
                    });
                });
                dev.set_onExit([](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->on_exit();
                    });
                });
                dev.set_newFrameConfirm([](pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->new_frame_confirm();
                    });
                });
                dev.set_textUTF8([](double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->text_utf8(x, y, str, rot, hadj, gc);
                    });
                });
                dev.set_strWidthUTF8([](const char *str, const pGEcontext gc, pDevDesc dd) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->str_width_utf8(str, gc);
                    });
                });
                dev.set_eventHelper([](pDevDesc dd, int code) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->event_helper(code);
                    });
                });
                dev.set_holdflush([](pDevDesc dd, int level) {
                    return rhost::util::exceptions_to_errors([&] {
                        return reinterpret_cast<graphics_device*>(devdesc_wrapper(dd).get_deviceSpecific())->hold_flush(level);
                    });
                });
            }
        };
    }
}
