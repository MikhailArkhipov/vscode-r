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
#include "xamlbuilder.h"

#ifdef _WIN32
#include "Rgraphicsapi.h"
#endif

#include "msvcrt.h"
#include "grdevices.h"
#include "exports.h"

#define DEVICE_UNIT_TO_INCH(x) (x / 72)
#define INCH_TO_DEVICE_UNIT(x) (72.0 * x)

#define FONTFACE_PLAIN          1
#define FONTFACE_BOLD           2
#define FONTFACE_ITALIC         3
#define FONTFACE_BOLD_ITALIC    4

#define DEFAULT_FONT_NAME       "Arial"

namespace rhost {
    namespace grdevices {
        namespace xaml {
            ///////////////////////////////////////////////////////////////////////
            // Xaml device
            ///////////////////////////////////////////////////////////////////////
            class xaml_device : public graphics_device {
            public:
                static std::unique_ptr<xaml_device> create(std::string filename, double width, double height);

                xaml_device(pDevDesc dd, std::string filename, double width, double height, std::string background_color, std::string font_family);
                virtual ~xaml_device();

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

            private:
                std::string get_raster_file_path();

            private:
                double _width;
                double _height;
                bool _debug;
                std::string _filename;
                rhost::graphics::xaml_builder _xaml;
            };

            static double string_width(const char *str, double ps, int fontface) {
#ifdef _WIN32
                SIZE size;
                HDC dc = GetDC(NULL);
                // https://msdn.microsoft.com/en-us/library/windows/desktop/dd183499(v=vs.85).aspx
                HFONT font = CreateFontA((int)ps, 0, 0, 0,
                    (fontface == FONTFACE_BOLD || fontface == FONTFACE_BOLD_ITALIC) ? FW_BOLD : FW_NORMAL,
                    fontface == FONTFACE_ITALIC || fontface == FONTFACE_BOLD_ITALIC,
                    FALSE,
                    FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    FF_DONTCARE | DEFAULT_PITCH,
                    DEFAULT_FONT_NAME);
                auto old_font = SelectObject(dc, font);
                BOOL result = GetTextExtentPoint32A(dc, str, (int)strlen(str), &size);
                SelectObject(dc, old_font);
                DeleteObject(font);
                ReleaseDC(NULL, dc);
                if (result) {
                    return size.cx;
                }
                else {
                    return 0;
                }
#endif
            }

            static std::string r_fontface_to_xaml_font_weight(int fontface) {
                switch (fontface) {
                case FONTFACE_BOLD:
                case FONTFACE_BOLD_ITALIC:
                    return "Bold";
                default:
                    return "";
                }
            }

            static std::string r_fontface_to_xaml_font_style(int fontface) {
                switch (fontface) {
                case FONTFACE_ITALIC:
                case FONTFACE_BOLD_ITALIC:
                    return "Italic";
                default:
                    return "";
                }
            }

            static std::string r_color_to_xaml(int col) {
                if (col == NA_INTEGER) {
                    return "";
                }
                char buffer[100];
                sprintf(buffer, "#%02X%02X%02X%02X", R_ALPHA(col), R_RED(col), R_GREEN(col), R_BLUE(col));
                return std::string(buffer);
            }

            static std::string r_line_type_to_xaml(int lty) {
                // Integer is a list of (up to 8) 4-bit values, starting the least significant bit.
                // Values represent the length of each segment, alternating between ON and OFF.
                // See help(par) for more details on line types.
                //
                // Examples for predefined styles:
                // LTY_SOLID     0x00000000
                // LTY_DASHED    0x00000044
                // LTY_DOTTED    0x00000031
                // LTY_DOTDASH   0x00003431
                // LTY_LONGDASH  0x00000037
                // LTY_TWODASH   0x00002622
                if (lty == 0) {
                    return "";
                }

                std::ostringstream stream;

                for (int i = 0; i < 8; i++) {
                    int val = (lty >> (i * 4)) & 0xf;
                    if (val == 0) {
                        break;
                    }
                    stream << val << " ";
                }

                std::string str = stream.str();
                boost::algorithm::trim_right(str);
                return str;
            }

            static std::string r_line_join_to_xaml(int ljoin) {
                switch (ljoin) {
                case GE_ROUND_JOIN:
                    return "Round";
                case GE_MITRE_JOIN:
                    return "Miter";
                case GE_BEVEL_JOIN:
                    return "Bevel";
                default:
                    return "";
                }
            }

            static std::string r_line_end_to_xaml(int lend) {
                switch (lend) {
                case GE_ROUND_CAP:
                    return "Round";
                case GE_BUTT_CAP:
                case GE_SQUARE_CAP:
                    return "Square";
                default:
                    return "";
                }
            }

            static void write_bitmap(std::ofstream& f, unsigned int *raster, int w, int h) {
#ifdef _WIN32
                BITMAPV4HEADER infoHeader;
                memset(&infoHeader, 0, sizeof(infoHeader));
                infoHeader.bV4Size = sizeof(infoHeader);
                infoHeader.bV4Width = w;
                infoHeader.bV4Height = h;
                infoHeader.bV4Planes = 0;
                infoHeader.bV4BitCount = 32;
                infoHeader.bV4V4Compression = BI_BITFIELDS;
                infoHeader.bV4SizeImage = 4 * w * h;
                infoHeader.bV4XPelsPerMeter = 2835;
                infoHeader.bV4YPelsPerMeter = 2835;
                infoHeader.bV4ClrUsed = 0;
                infoHeader.bV4ClrImportant = 0;
                infoHeader.bV4BlueMask = 0x00ff0000;
                infoHeader.bV4GreenMask = 0x0000ff00;
                infoHeader.bV4RedMask = 0x000000ff;
                infoHeader.bV4AlphaMask = 0xff000000;
                infoHeader.bV4CSType = 0x57696e20;

                BITMAPFILEHEADER bmp;
                memset(&bmp, 0, sizeof(bmp));
                bmp.bfType = 0x4D42;
                bmp.bfSize = sizeof(bmp) + sizeof(infoHeader) + 4 * w * h;
                bmp.bfOffBits = sizeof(bmp) + sizeof(infoHeader);

                f.write((char*)&bmp, sizeof(bmp));
                f.write((char*)&infoHeader, sizeof(infoHeader));
                f.write((char*)raster, 4 * w * h);
#endif
            }

            std::unique_ptr<xaml_device> xaml_device::create(std::string filename, double width, double height) {
                pDevDesc dd = static_cast<pDevDesc>(RHOST_calloc(1, sizeof(DevDesc)));

                int startfill = R_RGB(255, 255, 255);

                auto xdd = std::make_unique<xaml_device>(dd, filename, width, height, r_color_to_xaml(startfill), DEFAULT_FONT_NAME);

                dd->left = 0;
                dd->right = width;
                dd->bottom = height;
                dd->top = 0;

                dd->clipLeft = dd->left;
                dd->clipRight = dd->right;
                dd->clipBottom = dd->bottom;
                dd->clipTop = dd->top;

                dd->xCharOffset = 0;
                dd->yCharOffset = 0;
                dd->yLineBias = 0;

                dd->ipr[0] = dd->ipr[1] = DEVICE_UNIT_TO_INCH(1.0);
                dd->cra[0] = dd->cra[1] = 10;
                dd->gamma = 0;

                dd->canClip = R_FALSE;
                dd->canChangeGamma = R_FALSE;
                dd->canHAdj = 0;

                dd->startps = 10;
                dd->startcol = R_RGB(0, 0, 0);
                dd->startfill = startfill;
                dd->startlty = LTY_SOLID;
                dd->startfont = 0;
                dd->startgamma = 0;

                dd->hasTextUTF8 = R_FALSE;
                dd->wantSymbolUTF8 = R_FALSE;
                dd->useRotatedTextInContour = R_FALSE;

                dd->haveTransparency = 1; //no
                dd->haveTransparentBg = 2; //fully
                dd->haveRaster = 2; //yes
                dd->haveCapture = 1; //no
                dd->haveLocator = 1; //no

                dd->displayListOn = R_TRUE;
                dd->canGenMouseDown = R_FALSE;
                dd->canGenMouseMove = R_FALSE;
                dd->canGenMouseUp = R_FALSE;
                dd->canGenKeybd = R_FALSE;

                dd->deviceSpecific = xdd.get();

                return xdd;
            }

            void xaml_device::activate() {
            }

            void xaml_device::circle(double x, double y, double r, pGEcontext gc) {
                double top = y - r;
                double left = x - r;
                double width = r * 2;
                double height = r * 2;

                _xaml.circle(top, left, width, height, r_color_to_xaml(gc->fill), r_color_to_xaml(gc->col), gc->lwd,
                    r_line_type_to_xaml(gc->lty),
                    r_line_join_to_xaml(gc->ljoin),
                    r_line_end_to_xaml(gc->lend),
                    gc->lmitre);
            }

            void xaml_device::clip(double x0, double x1, double y0, double y1) {
                _xaml.clip_begin(x0, x1, y0, y1);
            }

            void xaml_device::close() {
                _xaml.clip_end();
                _xaml.write_xaml(_filename);
                delete this;
            }

            void xaml_device::deactivate() {
            }

            Rboolean xaml_device::locator(double *x, double *y) {
                *x = 0;
                *y = 0;
                return R_FALSE;
            }

            void xaml_device::line(double x1, double y1, double x2, double y2, const pGEcontext gc) {
                _xaml.line(x1, y1, x2, y2, r_color_to_xaml(gc->col), gc->lwd,
                    r_line_type_to_xaml(gc->lty),
                    r_line_join_to_xaml(gc->ljoin),
                    r_line_end_to_xaml(gc->lend),
                    gc->lmitre);
            }

            void xaml_device::metric_info(int c, const pGEcontext gc, double* ascent, double* descent, double* width) {
                *ascent = 0;
                *descent = 0;
                *width = 0;
            }

            void xaml_device::mode(int mode) {
                if (mode == 0) {
                    _xaml.clip_end();
                }
            }

            void xaml_device::new_page(const pGEcontext gc) {
                _xaml.clear();
            }

            void xaml_device::polygon(int n, double *x, double *y, const pGEcontext gc) {
                _xaml.polygon(n, x, y, r_color_to_xaml(gc->fill), r_color_to_xaml(gc->col), gc->lwd,
                    r_line_type_to_xaml(gc->lty),
                    r_line_join_to_xaml(gc->ljoin),
                    r_line_end_to_xaml(gc->lend),
                    gc->lmitre);
            }

            void xaml_device::polyline(int n, double *x, double *y, const pGEcontext gc) {
                _xaml.polyline(n, x, y, r_color_to_xaml(gc->col), gc->lwd,
                    r_line_type_to_xaml(gc->lty),
                    r_line_join_to_xaml(gc->ljoin),
                    r_line_end_to_xaml(gc->lend),
                    gc->lmitre);
            }

            void xaml_device::rect(double x0, double y0, double x1, double y1, const pGEcontext gc) {
                double left = fmin(x0, x1);
                double top = fmin(y0, y1);
                double width = fabs(x1 - x0);
                double height = fabs(y1 - y0);

                _xaml.rect(top, left, width, height, r_color_to_xaml(gc->fill), r_color_to_xaml(gc->col), gc->lwd,
                    r_line_type_to_xaml(gc->lty),
                    r_line_join_to_xaml(gc->ljoin),
                    r_line_end_to_xaml(gc->lend),
                    gc->lmitre);
            }

            void xaml_device::path(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc) {
                _xaml.path(x, y, npoly, nper, winding != R_FALSE, r_color_to_xaml(gc->fill), r_color_to_xaml(gc->col), gc->lwd,
                    r_line_type_to_xaml(gc->lty),
                    r_line_join_to_xaml(gc->ljoin),
                    r_line_end_to_xaml(gc->lend),
                    gc->lmitre);
            }

            void xaml_device::raster(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc) {
                auto path = get_raster_file_path();
                double left = x;
                double top = _height - y;

                std::ofstream f(path, std::ofstream::out);
                write_bitmap(f, raster, w, h);
                f.close();

                _xaml.bitmap_external_file(top, left, width, 0.0 - height, 0.0 - rot, interpolate != R_FALSE, path);
            }

            SEXP xaml_device::cap() {
                return R_NilValue;
            }

            void xaml_device::size(double *left, double *right, double *bottom, double *top) {
                *left = device_desc->left;
                *right = device_desc->right;
                *bottom = device_desc->bottom;
                *top = device_desc->top;
            }

            double xaml_device::str_width(const char *str, const pGEcontext gc) {
                double width = string_width(str, gc->ps * gc->cex, gc->fontface);
                return width;
            }

            void xaml_device::text(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                y -= gc->ps * gc->cex;

                _xaml.text(x, y, str, 0.0 - rot, hadj,
                    r_color_to_xaml(gc->col), gc->ps * gc->cex,
                    r_fontface_to_xaml_font_weight(gc->fontface),
                    r_fontface_to_xaml_font_style(gc->fontface));
            }

            void xaml_device::on_exit() {
            }

            Rboolean xaml_device::new_frame_confirm() {
                return R_FALSE;
            }

            void xaml_device::text_utf8(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc) {
                y -= gc->ps * gc->cex;

                _xaml.text(x, y, str, 0.0 - rot, hadj,
                    r_color_to_xaml(gc->col), gc->ps * gc->cex,
                    r_fontface_to_xaml_font_weight(gc->fontface),
                    r_fontface_to_xaml_font_style(gc->fontface));
            }

            double xaml_device::str_width_utf8(const char *str, const pGEcontext gc) {
                double width = string_width(str, gc->ps * gc->cex, gc->fontface);

                return width;
            }

            void xaml_device::event_helper(int code) {
            }

            int xaml_device::hold_flush(int level) {
                return 0;
            }

            xaml_device::xaml_device(pDevDesc dd, std::string filename, double width, double height, std::string background_color, std::string font_family) :
                graphics_device(dd),
                _xaml(width, height, background_color, font_family),
                _filename(filename),
                _width(width),
                _height(height),
                _debug(false)
            {
            }

            xaml_device::~xaml_device()
            {
            }

            std::string xaml_device::get_raster_file_path() {
#ifdef _WIN32
                // TODO: change this to use same folder/name as _filename
                // but with an incrementing integer suffix, and a .bmp extension
                char folderpath[1024];
                char filepath[1024];
                GetTempPathA(1024, folderpath);
                GetTempFileNameA(folderpath, "rt", 0, filepath);

                return std::string(filepath);
#else
                // return std::string(mktemp("bmp.rt.XXXXXXXX"));
                return std::string();
#endif
            }

            ///////////////////////////////////////////////////////////////////////
            // Exported R routines
            ///////////////////////////////////////////////////////////////////////

            extern "C" SEXP xaml_graphicsdevice_new(SEXP args) {
                args = CDR(args);
                SEXP file = CAR(args);
                args = CDR(args);
                SEXP width = CAR(args);
                args = CDR(args);
                SEXP height = CAR(args);

                const char *f = R_CHAR(STRING_ELT(file, 0));
                double *w = REAL(width);
                double *h = REAL(height);

                int ver = R_GE_getVersion();
                if (ver < R_32_GE_version || ver > R_33_GE_version) {
                    Rf_error("Graphics API version %d is not supported.", ver);
                }

                R_CheckDeviceAvailable();
                BEGIN_SUSPEND_INTERRUPTS{
                    auto dev = xaml_device::create(f, *w, *h);
                    pGEDevDesc gdd = GEcreateDevDesc(dev->device_desc);
                    GEaddDevice2f(gdd, "xaml", f);
                    // Owner is DevDesc::deviceSpecific, and is released in close()
                    dev.release();
                } END_SUSPEND_INTERRUPTS;

                return R_NilValue;
            }

            static R_ExternalMethodDef external_methods[] = {
                { "Microsoft.R.Host::External.xaml_graphicsdevice_new", (DL_FUNC)&xaml_graphicsdevice_new, 3 },
                { }
            };

            void init(DllInfo *dll)
            {
                rhost::exports::add_external_methods(external_methods);
            }
        }
    }
}
