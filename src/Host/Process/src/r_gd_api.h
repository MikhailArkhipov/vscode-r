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

namespace rhost {
    namespace rapi {
        // Takes type T, and returns the same type, except that any references to type 
        // From are replaced with type To recursively. E.g. if the input type is:
        //    From (*)(int, From*);
        // it becomes:
        //    To (*)(int, To*);
        template<class From, class To, class T>
        struct subst {
            typedef T type;
        };

        template<class From, class To>
        struct subst<From, To, From> {
            typedef To type;
        };

        template<class From, class To, class T>
        struct subst<From, To, const T> {
            typedef const typename subst<From, To, T>::type type;
        };

        template<class From, class To, class T>
        struct subst<From, To, T*> {
            typedef typename subst<From, To, T>::type* type;
        };

        template<class From, class To, class T, size_t N>
        struct subst<From, To, T[N]> {
            typedef typename subst<From, To, T>::type type[N];
        };

        template<class From, class To, class Result>
        struct subst<From, To, Result(*)()> {
            typedef typename subst<From, To, Result>::type(*type)();
        };

        template<class From, class To, class Result, class Arg, class... Args>
        class subst<From, To, Result(*)(Arg, Args...)> {
            template<class... SubbedArgs>
            static auto dummy(Result(*f)(SubbedArgs...))->Result(*)(typename subst<From, To, Arg>::type, SubbedArgs...);
        public:
            typedef decltype(dummy(static_cast<typename subst<From, To, Result(*)(Args...)>::type>(nullptr))) type;
        };

#define RHOST_GD_MEMBER_DECL(x) static subst<::DevDesc, DevDesc, decltype(::x)*>::type x;

#define RHOST_DEVDESC_MEMBER(x) subst<::DevDesc, DevDesc, decltype(::DevDesc::x)>::type x;

#define RHOST_DEVDESC struct DevDesc { \
    RHOST_DEVDESC_V10_MEMBER(left) \
    RHOST_DEVDESC_V10_MEMBER(right) \
    RHOST_DEVDESC_V10_MEMBER(bottom) \
    RHOST_DEVDESC_V10_MEMBER(top) \
    RHOST_DEVDESC_V10_MEMBER(clipLeft) \
    RHOST_DEVDESC_V10_MEMBER(clipRight) \
    RHOST_DEVDESC_V10_MEMBER(clipBottom) \
    RHOST_DEVDESC_V10_MEMBER(clipTop) \
    RHOST_DEVDESC_V10_MEMBER(xCharOffset) \
    RHOST_DEVDESC_V10_MEMBER(yCharOffset) \
    RHOST_DEVDESC_V10_MEMBER(yLineBias) \
    RHOST_DEVDESC_V10_MEMBER(ipr) \
    RHOST_DEVDESC_V10_MEMBER(cra) \
    RHOST_DEVDESC_V10_MEMBER(gamma) \
    RHOST_DEVDESC_V10_MEMBER(canClip) \
    RHOST_DEVDESC_V10_MEMBER(canChangeGamma) \
    RHOST_DEVDESC_V10_MEMBER(canHAdj) \
    RHOST_DEVDESC_V10_MEMBER(startps) \
    RHOST_DEVDESC_V10_MEMBER(startcol) \
    RHOST_DEVDESC_V10_MEMBER(startfill) \
    RHOST_DEVDESC_V10_MEMBER(startlty) \
    RHOST_DEVDESC_V10_MEMBER(startfont) \
    RHOST_DEVDESC_V10_MEMBER(startgamma) \
    RHOST_DEVDESC_V10_MEMBER(deviceSpecific) \
    RHOST_DEVDESC_V10_MEMBER(displayListOn) \
    RHOST_DEVDESC_V10_MEMBER(canGenMouseDown) \
    RHOST_DEVDESC_V10_MEMBER(canGenMouseMove) \
    RHOST_DEVDESC_V10_MEMBER(canGenMouseUp) \
    RHOST_DEVDESC_V10_MEMBER(canGenKeybd) \
    RHOST_DEVDESC_V12_MEMBER(canGenIdle) \
    RHOST_DEVDESC_V10_MEMBER(gettingEvent) \
    RHOST_DEVDESC_V10_MEMBER(activate) \
    RHOST_DEVDESC_V10_MEMBER(circle) \
    RHOST_DEVDESC_V10_MEMBER(clip) \
    RHOST_DEVDESC_V10_MEMBER(close) \
    RHOST_DEVDESC_V10_MEMBER(deactivate) \
    RHOST_DEVDESC_V10_MEMBER(locator) \
    RHOST_DEVDESC_V10_MEMBER(line) \
    RHOST_DEVDESC_V10_MEMBER(metricInfo) \
    RHOST_DEVDESC_V10_MEMBER(mode) \
    RHOST_DEVDESC_V10_MEMBER(newPage) \
    RHOST_DEVDESC_V10_MEMBER(polygon) \
    RHOST_DEVDESC_V10_MEMBER(polyline) \
    RHOST_DEVDESC_V10_MEMBER(rect) \
    RHOST_DEVDESC_V10_MEMBER(path) \
    RHOST_DEVDESC_V10_MEMBER(raster) \
    RHOST_DEVDESC_V10_MEMBER(cap) \
    RHOST_DEVDESC_V10_MEMBER(size) \
    RHOST_DEVDESC_V10_MEMBER(strWidth) \
    RHOST_DEVDESC_V10_MEMBER(text) \
    RHOST_DEVDESC_V10_MEMBER(onExit) \
    RHOST_DEVDESC_V10_MEMBER(getEvent) \
    RHOST_DEVDESC_V10_MEMBER(newFrameConfirm) \
    RHOST_DEVDESC_V10_MEMBER(hasTextUTF8) \
    RHOST_DEVDESC_V10_MEMBER(textUTF8) \
    RHOST_DEVDESC_V10_MEMBER(strWidthUTF8) \
    RHOST_DEVDESC_V10_MEMBER(wantSymbolUTF8) \
    RHOST_DEVDESC_V10_MEMBER(useRotatedTextInContour) \
    RHOST_DEVDESC_V10_MEMBER(eventEnv) \
    RHOST_DEVDESC_V10_MEMBER(eventHelper) \
    RHOST_DEVDESC_V10_MEMBER(holdflush) \
    RHOST_DEVDESC_V10_MEMBER(haveTransparency) \
    RHOST_DEVDESC_V10_MEMBER(haveTransparentBg) \
    RHOST_DEVDESC_V10_MEMBER(haveRaster) \
    RHOST_DEVDESC_V10_MEMBER(haveCapture) \
    RHOST_DEVDESC_V10_MEMBER(haveLocator) \
    RHOST_DEVDESC_V10_MEMBER(reserved) \
    };

#define RHOST_DEVDESC_V10_MEMBER(x) RHOST_DEVDESC_MEMBER(x)
#define RHOST_DEVDESC_V12_MEMBER(x) RHOST_DEVDESC_MEMBER(x)

        template<int ApiVer>
        struct gd_api;

        template<>
        struct gd_api<12> {
            struct DevDesc;
            typedef DevDesc* pDevDesc;

            RHOST_DEVDESC;
            RHOST_GD_SET(RHOST_GD_MEMBER_DECL);

            static void load();
            static void unload();
        };

#undef RHOST_DEVDESC_V12_MEMBER
#define RHOST_DEVDESC_V12_MEMBER(x)

        template<>
        struct gd_api<10> {
            struct DevDesc;
            typedef DevDesc* pDevDesc;
            RHOST_DEVDESC;
            RHOST_GD_SET(RHOST_GD_MEMBER_DECL);

            static void load();
            static void unload();
        };

        template<>
        struct gd_api<11> : gd_api<10> {
        };
    }
}