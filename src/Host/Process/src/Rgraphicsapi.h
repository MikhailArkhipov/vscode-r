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

// This file combines various bits and pieces of R API in a single header file.
//
// The main reason for its existence is that R headers themselves are not immediately usable
// after checkout: some parts expect R to be at least partially built to generate config.h
// etc, or rely on various macros defined in R makefiles. By moving all this code here and
// cleaning it up, it is readily usable.
//
// The following R header files were used as a basis to produce this file:
// - R_ext/Arith.h
// - R_ext/GraphicsDevice.h
// - R_ext/GraphicsEngine.h

#pragma once

#include <stdio.h>
#include <stdarg.h>

#include "r_api.h"

#define R_32_GE_version 10
#define R_33_GE_version 11
#define R_34_GE_version 12

#define R_RGB(r,g,b)        ((r)|((g)<<8)|((b)<<16)|0xFF000000)
#define R_RGBA(r,g,b,a)     ((r)|((g)<<8)|((b)<<16)|((a)<<24))
#define R_RED(col)          (((col))&255)
#define R_GREEN(col)        (((col)>>8)&255)
#define R_BLUE(col)         (((col)>>16)&255)
#define R_ALPHA(col)        (((col)>>24)&255)
#define R_OPAQUE(col)       (R_ALPHA(col) == 255)
#define R_TRANSPARENT(col)  (R_ALPHA(col) == 0)

#define LTY_BLANK           -1
#define LTY_SOLID           0
#define LTY_DASHED          4 + (4<<4)
#define LTY_DOTTED          1 + (3<<4)
#define LTY_DOTDASH         1 + (3<<4) + (4<<8) + (3<<12)
#define LTY_LONGDASH        7 + (3<<4)
#define LTY_TWODASH         2 + (2<<4) + (6<<8) + (2<<12)

#define NA_INTEGER          R_NaInt

extern "C" {
    int R_GE_getVersion(void);

    void R_GE_checkVersionOrDie(int version);

    typedef enum {
        GE_ROUND_CAP = 1,
        GE_BUTT_CAP = 2,
        GE_SQUARE_CAP = 3
    } R_GE_lineend;

    typedef enum {
        GE_ROUND_JOIN = 1,
        GE_MITRE_JOIN = 2,
        GE_BEVEL_JOIN = 3
    } R_GE_linejoin;

    typedef struct {
        int col;
        int fill;
        double gamma;
        double lwd;
        int lty;
        R_GE_lineend lend;
        R_GE_linejoin ljoin;
        double lmitre;
        double cex;
        double ps;
        double lineheight;
        int fontface;
        char fontfamily[201];
    } R_GE_gcontext;

    typedef R_GE_gcontext* pGEcontext;

    typedef struct _DevDesc11 DevDesc10; // V11 is compatible with V10
    typedef struct _DevDesc11 DevDesc11;
    typedef struct _DevDesc12 DevDesc12;
    typedef DevDesc11* pDevDesc10; // V11 is compatible with V10
    typedef DevDesc11* pDevDesc11;
    typedef DevDesc12* pDevDesc12;
    typedef DevDesc12* pDevDesc;

    struct _DevDesc11 {
        double left;
        double right;
        double bottom;
        double top;
        double clipLeft;
        double clipRight;
        double clipBottom;
        double clipTop;
        double xCharOffset;
        double yCharOffset;
        double yLineBias;
        double ipr[2];
        double cra[2];
        double gamma;
        Rboolean canClip;
        Rboolean canChangeGamma;
        int canHAdj;
        double startps;
        int startcol;
        int startfill;
        int startlty;
        int startfont;
        double startgamma;
        void *deviceSpecific;
        Rboolean displayListOn;
        Rboolean canGenMouseDown;
        Rboolean canGenMouseMove;
        Rboolean canGenMouseUp;
        Rboolean canGenKeybd;
        Rboolean gettingEvent;
        void(*activate)(const pDevDesc);
        void(*circle)(double x, double y, double r, const pGEcontext gc, pDevDesc dd);
        void(*clip)(double x0, double x1, double y0, double y1, pDevDesc dd);
        void(*close)(pDevDesc dd);
        void(*deactivate)(pDevDesc);
        Rboolean(*locator)(double *x, double *y, pDevDesc dd);
        void(*line)(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd);
        void(*metricInfo)(int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dd);
        void(*mode)(int mode, pDevDesc dd);
        void(*newPage)(const pGEcontext gc, pDevDesc dd);
        void(*polygon)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
        void(*polyline)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
        void(*rect)(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd);
        void(*path)(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd);
        void(*raster)(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
        SEXP(*cap)(pDevDesc dd);
        void(*size)(double *left, double *right, double *bottom, double *top, pDevDesc dd);
        double(*strWidth)(const char *str, const pGEcontext gc, pDevDesc dd);
        void(*text)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
        void(*onExit)(pDevDesc dd);
        SEXP(*getEvent)(SEXP, const char *);
        Rboolean(*newFrameConfirm)(pDevDesc dd);
        Rboolean hasTextUTF8;
        void(*textUTF8)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
        double(*strWidthUTF8)(const char *str, const pGEcontext gc, pDevDesc dd);
        Rboolean wantSymbolUTF8;
        Rboolean useRotatedTextInContour;
        SEXP eventEnv;
        void(*eventHelper)(pDevDesc dd, int code);
        int(*holdflush)(pDevDesc dd, int level);
        int haveTransparency;
        int haveTransparentBg;
        int haveRaster;
        int haveCapture, haveLocator;
        char reserved[64];
    };

    struct _DevDesc12 {
        double left;
        double right;
        double bottom;
        double top;
        double clipLeft;
        double clipRight;
        double clipBottom;
        double clipTop;
        double xCharOffset;
        double yCharOffset;
        double yLineBias;
        double ipr[2];
        double cra[2];
        double gamma;
        Rboolean canClip;
        Rboolean canChangeGamma;
        int canHAdj;
        double startps;
        int startcol;
        int startfill;
        int startlty;
        int startfont;
        double startgamma;
        void *deviceSpecific;
        Rboolean displayListOn;
        Rboolean canGenMouseDown;
        Rboolean canGenMouseMove;
        Rboolean canGenMouseUp;
        Rboolean canGenKeybd;
        Rboolean canGenIdle;
        Rboolean gettingEvent;
        void(*activate)(const pDevDesc);
        void(*circle)(double x, double y, double r, const pGEcontext gc, pDevDesc dd);
        void(*clip)(double x0, double x1, double y0, double y1, pDevDesc dd);
        void(*close)(pDevDesc dd);
        void(*deactivate)(pDevDesc);
        Rboolean(*locator)(double *x, double *y, pDevDesc dd);
        void(*line)(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd);
        void(*metricInfo)(int c, const pGEcontext gc, double* ascent, double* descent, double* width, pDevDesc dd);
        void(*mode)(int mode, pDevDesc dd);
        void(*newPage)(const pGEcontext gc, pDevDesc dd);
        void(*polygon)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
        void(*polyline)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
        void(*rect)(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd);
        void(*path)(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd);
        void(*raster)(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
        SEXP(*cap)(pDevDesc dd);
        void(*size)(double *left, double *right, double *bottom, double *top, pDevDesc dd);
        double(*strWidth)(const char *str, const pGEcontext gc, pDevDesc dd);
        void(*text)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
        void(*onExit)(pDevDesc dd);
        SEXP(*getEvent)(SEXP, const char *);
        Rboolean(*newFrameConfirm)(pDevDesc dd);
        Rboolean hasTextUTF8;
        void(*textUTF8)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
        double(*strWidthUTF8)(const char *str, const pGEcontext gc, pDevDesc dd);
        Rboolean wantSymbolUTF8;
        Rboolean useRotatedTextInContour;
        SEXP eventEnv;
        void(*eventHelper)(pDevDesc dd, int code);
        int(*holdflush)(pDevDesc dd, int level);
        int haveTransparency;
        int haveTransparentBg;
        int haveRaster;
        int haveCapture, haveLocator;
        char reserved[64];
    };

#ifndef BEGIN_SUSPEND_INTERRUPTS
#define BEGIN_SUSPEND_INTERRUPTS do { \
    Rboolean __oldsusp__ = R_interrupts_suspended; \
    R_interrupts_suspended = R_TRUE;
#define END_SUSPEND_INTERRUPTS R_interrupts_suspended = __oldsusp__; \
    if (R_interrupts_pending && ! R_interrupts_suspended) \
        Rf_onintr(); \
} while(0)
    extern __declspec(dllimport) Rboolean R_interrupts_suspended;
    extern __declspec(dllimport) int R_interrupts_pending;
    extern void Rf_onintr(void);
    extern __declspec(dllimport) Rboolean mbcslocale;
#endif

#define MAX_GRAPHICS_SYSTEMS 24

    typedef enum {
        GE_InitState = 0,
        GE_FinaliseState = 1,
        GE_SaveState = 2,
        GE_RestoreState = 6,
        GE_CopyState = 3,
        GE_SaveSnapshotState = 4,
        GE_RestoreSnapshotState = 5,
        GE_CheckPlot = 7,
        GE_ScalePS = 8
    } GEevent;

    typedef struct _GEDevDesc GEDevDesc;

    typedef SEXP(*GEcallback)(GEevent, GEDevDesc *, SEXP);

    typedef struct {
        void *systemSpecific;
        GEcallback callback;
    } GESystemDesc;

    struct _GEDevDesc {
        pDevDesc dev;
        Rboolean displayListOn;
        SEXP displayList;
        SEXP DLlastElt;
        SEXP savedSnapshot;
        Rboolean dirty;
        Rboolean recordGraphics;
        GESystemDesc *gesd[MAX_GRAPHICS_SYSTEMS];
        Rboolean ask;
    };

    typedef GEDevDesc* pGEDevDesc;

    extern void GEaddDevice2(pGEDevDesc, const char *);
    extern void GEaddDevice2f(pGEDevDesc, const char *, const char *);
    extern void GEkillDevice(pGEDevDesc);
    extern pGEDevDesc GEcreateDevDesc(pDevDesc dev);
    extern pGEDevDesc GEgetDevice(int);

    extern void R_CheckDeviceAvailable(void);
    extern Rboolean R_CheckDeviceAvailableBool(void);

    extern pGEDevDesc GEcurrentDevice(void);
    extern Rboolean GEdeviceDirty(pGEDevDesc dd);
    extern void GEdirtyDevice(pGEDevDesc dd);
    extern Rboolean GEcheckState(pGEDevDesc dd);
    extern Rboolean GErecording(SEXP call, pGEDevDesc dd);
    extern void GErecordGraphicOperation(SEXP op, SEXP args, pGEDevDesc dd);
    extern void GEinitDisplayList(pGEDevDesc dd);
    extern void GEplayDisplayList(pGEDevDesc dd);
    extern void GEcopyDisplayList(int fromDevice);
    extern SEXP GEcreateSnapshot(pGEDevDesc dd);
    extern void GEplaySnapshot(SEXP snapshot, pGEDevDesc dd);
    extern void GEonExit(void);
    extern void GEnullDevice(void);

    extern int Rf_curDevice(void);
    extern int Rf_selectDevice(int);
    extern int Rf_ndevNumber(pDevDesc);
    extern int Rf_NumDevices(void);
    extern pGEDevDesc Rf_desc2GEDesc(pDevDesc dd);
}
