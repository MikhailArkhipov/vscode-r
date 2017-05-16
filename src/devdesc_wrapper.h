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

#include "r_api.h"

typedef void* RHOST_VOIDP;
typedef void(*dd_activate)(const pDevDesc);
typedef void(*dd_circle)(double x, double y, double r, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_clip)(double x0, double x1, double y0, double y1, pDevDesc dd);
typedef void(*dd_close)(pDevDesc dd);
typedef void(*dd_deactivate)(pDevDesc);
typedef Rboolean(*dd_locator)(double *x, double *y, pDevDesc dd);
typedef void(*dd_line)(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_metricInfo)(int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dd);
typedef void(*dd_mode)(int mode, pDevDesc dd);
typedef void(*dd_newPage)(const pGEcontext gc, pDevDesc dd);
typedef void(*dd_polygon)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_polyline)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_rect)(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_path)(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_raster)(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
typedef SEXP(*dd_cap)(pDevDesc dd);
typedef void(*dd_size)(double *left, double *right, double *bottom, double *top, pDevDesc dd);
typedef double(*dd_strWidth)(const char *str, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_text)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
typedef void(*dd_onExit)(pDevDesc dd);
typedef SEXP(*dd_getEvent)(SEXP, const char *);
typedef Rboolean(*dd_newFrameConfirm)(pDevDesc dd);
typedef void(*dd_eventHelper)(pDevDesc dd, int code);
typedef int(*dd_holdflush)(pDevDesc dd, int level);
typedef void(*dd_textUTF8)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
typedef double(*dd_strWidthUTF8)(const char *str, const pGEcontext gc, pDevDesc dd);

#define RHOST_DEV_WRAPPER_DECL_GET(member_type, member) \
            member_type get_##member(); \

#define RHOST_DEV_WRAPPER_DECL_SET(member_type, member) \
            void set_##member(member_type); \

#define RHOST_DEV_WRAPPER_DECL_GET_ARRAY(member_type, member) \
            member_type get_##member(int); \

#define RHOST_DEV_WRAPPER_DECL_SET_ARRAY(member_type, member) \
            void set_##member(int, member_type); \

#define RHOST_DEV_WRAPPER_DECL_GETSET(member_type, member) \
RHOST_DEV_WRAPPER_DECL_GET(member_type, member) \
RHOST_DEV_WRAPPER_DECL_SET(member_type, member) \

#define RHOST_DEV_WRAPPER_DECL_GETSET_ARRAY(member_type, member) \
RHOST_DEV_WRAPPER_DECL_GET_ARRAY(member_type, member) \
RHOST_DEV_WRAPPER_DECL_SET_ARRAY(member_type, member) \

#define RHOST_DEV_DEREF(ver, member) _u.dd_##ver->member
#define RHOST_DEV_DEREF_ARRAY(ver, member) _u.dd_##ver->member[index]

#define RHOST_DEV_GET_CASE(ver, member) \
    case ver: return RHOST_DEV_DEREF(ver, member); \

#define RHOST_DEV_GET_CASE_ARRAY(ver, member) \
    case ver: return RHOST_DEV_DEREF_ARRAY(ver, member); \

#define RHOST_DEV_SET_CASE(ver, member) \
    case ver: RHOST_DEV_DEREF(ver, member) = value; break; \

#define RHOST_DEV_SET_CASE_ARRAY(ver, member) \
    case ver: RHOST_DEV_DEREF_ARRAY(ver, member) = value; break; \

#define RHOST_DEV_GET_CASE_COMMON(member) \
    RHOST_DEV_GET_CASE(10, member) \
    RHOST_DEV_GET_CASE(11, member) \
    RHOST_DEV_GET_CASE(12, member) \

#define RHOST_DEV_SET_CASE_COMMON(member) \
    RHOST_DEV_SET_CASE(10, member) \
    RHOST_DEV_SET_CASE(11, member) \
    RHOST_DEV_SET_CASE(12, member) \

#define RHOST_DEV_GET_CASE_COMMON_ARRAY(member) \
    RHOST_DEV_GET_CASE_ARRAY(10, member) \
    RHOST_DEV_GET_CASE_ARRAY(11, member) \
    RHOST_DEV_GET_CASE_ARRAY(12, member) \

#define RHOST_DEV_SET_CASE_COMMON_ARRAY(member) \
    RHOST_DEV_SET_CASE_ARRAY(10, member) \
    RHOST_DEV_SET_CASE_ARRAY(11, member) \
    RHOST_DEV_SET_CASE_ARRAY(12, member) \

#define RHOST_DEV_VERSION_ASSERT(v) assert(v >= 10 && v <= 12)

#define RHOST_DEV_GET_CASE_V12(member) \
    RHOST_DEV_GET_CASE(12, member) \

#define RHOST_DEV_SET_CASE_V12(member) \
    RHOST_DEV_SET_CASE(12, member) \

#define RHOST_DEV_WRAPPER_DEFINE_GET(macro, member_type, member) \
member_type devdesc_wrapper::get_##member() { \
    switch(_ver) { \
    macro(member) \
    default: RHOST_DEV_VERSION_ASSERT(_ver); return static_cast<member_type>(0); \
    } \
} \

#define RHOST_DEV_WRAPPER_DEFINE_SET(macro, member_type, member) \
void devdesc_wrapper::set_##member(member_type value) { \
    switch(_ver) { \
    macro(member) \
    default: RHOST_DEV_VERSION_ASSERT(_ver); break; \
    } \
} \

#define RHOST_DEV_WRAPPER_DEFINE_GET_ARRAY(macro, member_type, member) \
member_type devdesc_wrapper::get_##member(int index) { \
    switch(_ver) { \
    macro(member) \
    default: RHOST_DEV_VERSION_ASSERT(_ver); return static_cast<member_type>(0); \
    } \
} \

#define RHOST_DEV_WRAPPER_DEFINE_SET_ARRAY(macro, member_type, member) \
void devdesc_wrapper::set_##member(int index, member_type value) { \
    switch(_ver) { \
    macro(member) \
    default: RHOST_DEV_VERSION_ASSERT(_ver); break; \
    } \
} \

#define RHOST_DEV_WRAPPER_DEFINE_GETSET_COMMON_ARRAY(member_type, member) \
RHOST_DEV_WRAPPER_DEFINE_GET_ARRAY(RHOST_DEV_GET_CASE_COMMON_ARRAY, member_type, member) \
RHOST_DEV_WRAPPER_DEFINE_SET_ARRAY(RHOST_DEV_SET_CASE_COMMON_ARRAY, member_type, member) \

#define RHOST_DEV_WRAPPER_DEFINE_GETSET_COMMON(member_type, member) \
RHOST_DEV_WRAPPER_DEFINE_GET(RHOST_DEV_GET_CASE_COMMON, member_type, member) \
RHOST_DEV_WRAPPER_DEFINE_SET(RHOST_DEV_SET_CASE_COMMON, member_type, member) \

#define RHOST_DEV_WRAPPER_DEFINE_GETSET_V12(member_type, member) \
RHOST_DEV_WRAPPER_DEFINE_GET(RHOST_DEV_GET_CASE_V12, member_type, member) \
RHOST_DEV_WRAPPER_DEFINE_SET(RHOST_DEV_SET_CASE_V12, member_type, member) \

#define RHOST_DEV_DESC_V11(macro) \
macro(double, left) \
macro(double, right) \
macro(double, bottom) \
macro(double, top) \
macro(double, clipLeft) \
macro(double, clipRight) \
macro(double, clipBottom) \
macro(double, clipTop) \
macro(double, xCharOffset) \
macro(double, yCharOffset) \
macro(double, yLineBias) \
macro(double, gamma) \
macro(Rboolean, canClip) \
macro(Rboolean, canChangeGamma) \
macro(int, canHAdj) \
macro(double, startps) \
macro(int, startcol) \
macro(int, startfill) \
macro(int, startlty) \
macro(int, startfont) \
macro(double, startgamma) \
macro(RHOST_VOIDP, deviceSpecific) \
macro(Rboolean, displayListOn) \
macro(Rboolean, canGenMouseDown) \
macro(Rboolean, canGenMouseMove) \
macro(Rboolean, canGenMouseUp) \
macro(Rboolean, canGenKeybd) \
macro(Rboolean, gettingEvent) \
macro(Rboolean, hasTextUTF8) \
macro(Rboolean, wantSymbolUTF8) \
macro(Rboolean, useRotatedTextInContour) \
macro(SEXP, eventEnv) \
macro(int, haveTransparency) \
macro(int, haveTransparentBg) \
macro(int, haveRaster) \
macro(int, haveCapture) \
macro(int, haveLocator) \
macro(dd_activate, activate) \
macro(dd_circle, circle) \
macro(dd_clip, clip) \
macro(dd_close, close) \
macro(dd_deactivate, deactivate) \
macro(dd_locator, locator) \
macro(dd_line, line) \
macro(dd_metricInfo, metricInfo) \
macro(dd_mode, mode) \
macro(dd_newPage, newPage) \
macro(dd_polygon, polygon) \
macro(dd_polyline, polyline) \
macro(dd_rect, rect) \
macro(dd_path, path) \
macro(dd_raster, raster) \
macro(dd_cap, cap) \
macro(dd_size, size) \
macro(dd_strWidth, strWidth) \
macro(dd_text, text) \
macro(dd_onExit, onExit) \
macro(dd_getEvent, getEvent) \
macro(dd_newFrameConfirm, newFrameConfirm) \
macro(dd_textUTF8, textUTF8) \
macro(dd_strWidthUTF8, strWidthUTF8) \
macro(dd_eventHelper, eventHelper) \
macro(dd_holdflush, holdflush) \

#define RHOST_DEV_DESC_V11_ARRAY(macro) \
macro(double, ipr) \
macro(double, cra) \

#define RHOST_DEV_DESC_V12(macro) \
macro(Rboolean, canGenIdle) \

namespace rhost {
    namespace grdevices {
        class devdesc_wrapper
        {
        private:
            union {
                pDevDesc10 dd_10;
                pDevDesc11 dd_11;
                pDevDesc12 dd_12;
            } _u;
            int _ver;
        public:
            devdesc_wrapper();
            devdesc_wrapper(pDevDesc dd);

            static void* allocate();
            operator pDevDesc();
            inline pDevDesc get_pDevDesc() { return operator pDevDesc(); }

            // V11 is compatible with V10 so no need to add V10 specifically
            RHOST_DEV_DESC_V11(RHOST_DEV_WRAPPER_DECL_GETSET);
            RHOST_DEV_DESC_V11_ARRAY(RHOST_DEV_WRAPPER_DECL_GETSET_ARRAY);
            RHOST_DEV_DESC_V12(RHOST_DEV_WRAPPER_DECL_GETSET);
        };

    }
}
