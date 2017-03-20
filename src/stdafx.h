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
#pragma warning(disable: 4996)

#define NOMINMAX

#include <atomic>
#include <cstdarg>
#include <cinttypes>
#include <codecvt>
#include <chrono>
#include <condition_variable>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "boost/algorithm/string.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/endian/buffers.hpp"
#include "boost/format.hpp"
#include "boost/program_options/cmdline.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/optional.hpp"
#include "boost/locale.hpp"
#include "boost/signals2/signal.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/filesystem.hpp"

#include "picojson.h"
#include "zip.h"

#if defined(_MSC_VER)
#define RHOST_EXPORT __declspec(dllexport)
#define RHOST_IMPORT __declspec(dllimport)
#define RHOST_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define RHOST_EXPORT __attribute__((visibility("default")))
#define RHOST_IMPORT
#define RHOST_NORETURN
#else
#define RHOST_EXPORT
#define RHOST_IMPORT
#define RHOST_NORETURN
#pragma warning Unknown DLL import/export.
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <windows.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

#include "minhook.h"
#else // linux
#include <unistd.h>
#define R_INTERFACE_PTRS
#define R_NO_REMAP
#include "R.h"
#include "Rinternals.h"
#include "Rinterface.h"
#include "Rembedded.h"

#define class class_
#define private private_
#include "R_ext/Connections.h"
#undef class
#undef private

#include "R_ext/RStartup.h"
#include "R_ext/Parse.h"
#include "R_ext/GraphicsEngine.h"
#include "R_ext/GraphicsDevice.h"

#define R_FALSE Rboolean::FALSE
#define R_TRUE Rboolean::TRUE

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
extern SEXP Rf_deparse1line(SEXP, Rboolean);
extern SEXP Rf_NewEnvironment(SEXP, SEXP, SEXP);
extern void R_CleanUp(SA_TYPE, int, int);
typedef SEXP(*CCODE)(SEXP, SEXP, SEXP, SEXP);

#define R_32_GE_version 10
#define R_33_GE_version 11

    enum {
        CTXT_TOPLEVEL = 0,
        CTXT_NEXT = 1,
        CTXT_BREAK = 2,
        CTXT_LOOP = 3,
        CTXT_FUNCTION = 4,
        CTXT_CCODE = 8,
        CTXT_RETURN = 12,
        CTXT_BROWSER = 16,
        CTXT_GENERIC = 20,
        CTXT_RESTART = 32,
        CTXT_BUILTIN = 64
    };

    typedef struct RCNTXT {
        struct RCNTXT *nextcontext;
        int callflag;
        sigjmp_buf cjmpbuf;
        int cstacktop;
        int evaldepth;
        SEXP promargs;
        SEXP callfun;
        SEXP sysparent;
        SEXP call;
        SEXP cloenv;
        SEXP conexit;
        void(*cend)(void *);
        void *cenddata;
        void *vmax;
        int intsusp;
        SEXP handlerstack;
        SEXP restartstack;
        struct RPRSTACK *prstack;
        SEXP *nodestack;
#ifdef BC_INT_STACK
        IStackval *intstack;
#endif
        SEXP srcref;
    } RCNTXT, *context;

    typedef enum {
        PP_INVALID  =  0,
        PP_ASSIGN   =  1,
        PP_ASSIGN2  =  2,
        PP_BINARY   =  3,
        PP_BINARY2  =  4,
        PP_BREAK    =  5,
        PP_CURLY    =  6,
        PP_FOR      =  7,
        PP_FUNCALL  =  8,
        PP_FUNCTION =  9,
        PP_IF       = 10,
        PP_NEXT     = 11,
        PP_PAREN    = 12,
        PP_RETURN   = 13,
        PP_SUBASS   = 14,
        PP_SUBSET   = 15,
        PP_WHILE    = 16,
        PP_UNARY    = 17,
        PP_DOLLAR   = 18,
        PP_FOREIGN  = 19,
        PP_REPEAT   = 20
    } PPkind;
    
    typedef enum {
        PREC_FN      = 0,
        PREC_EQ      = 1,
        PREC_LEFT    = 2,
        PREC_RIGHT   = 3,
        PREC_TILDE   = 4,
        PREC_OR      = 5,
        PREC_AND     = 6,
        PREC_NOT     = 7,
        PREC_COMPARE = 8,
        PREC_SUM     = 9,
        PREC_PROD    = 10,
        PREC_PERCENT = 11,
        PREC_COLON   = 12,
        PREC_SIGN    = 13,
        PREC_POWER   = 14,
        PREC_SUBSET  = 15,
        PREC_DOLLAR  = 16,
        PREC_NS      = 17
    } PPprec;
    
    typedef struct {
        PPkind kind;      /* deparse kind */
        PPprec precedence; /* operator precedence */
        unsigned int rightassoc;  /* right associative? */
    } PPinfo;

    typedef struct {
        char   *name;    /* print name */
        CCODE  cfun;     /* c-code address */
        int    code;     /* offset within c-code */
        int    eval;     /* evaluate args? */
        int    arity;    /* function arity */
        PPinfo gram;     /* pretty-print info */
    } FUNTAB;

    RHOST_IMPORT extern SEXP
        R_GlobalEnv, R_EmptyEnv, R_BaseEnv, R_BaseNamespace, R_Srcref, R_NilValue,
        R_TrueValue, R_FalseValue, R_UnboundValue, R_MissingArg, R_LogicalNAValue,
        R_NaString, R_BlankString, R_BlankScalarString, R_NamesSymbol;
    RHOST_IMPORT extern double R_NaN;
    RHOST_IMPORT extern double R_PosInf;
    RHOST_IMPORT extern double R_NegInf;
    RHOST_IMPORT extern double R_NaReal;
    RHOST_IMPORT extern int R_NaInt;
    RHOST_IMPORT extern FUNTAB R_FunTab[];

    extern int R_running_as_main_program;
#ifdef __cplusplus
}
#endif // __cplusplus
#endif

#ifdef _WIN32
#define RHOST_MAX_PATH MAX_PATH
#define RHOST_mbstowcs msvcrt::mbstowcs
#define RHOST_wctomb msvcrt::wctomb
#define RHOST_mbtowc msvcrt::mbtowc
#define RHOST_calloc msvcrt::calloc
#define RHOST_vsnprintf msvcrt::vsnprintf
#else

#define RHOST_MAX_PATH PATH_MAX
#define RHOST_mbstowcs std::mbstowcs
#define RHOST_wctomb std::wctomb
#define RHOST_mbtowc std::mbtowc
#define RHOST_calloc calloc
#define RHOST_vsnprintf vsnprintf

#define vsprintf_s vsprintf
void strcpy_s(char* dest, size_t n, char const* source) ;
void memcpy_s(void* const dest, size_t const destSize, void const* const source, size_t const sourceSize);
#endif

namespace fs = boost::filesystem;

#define RHOST_BITMASK_OPS(Ty) \
inline Ty& operator&=(Ty& _Left, Ty _Right) { _Left = (Ty)((int)_Left & (int)_Right); return (_Left); } \
inline Ty& operator|=(Ty& _Left, Ty _Right) { _Left = (Ty)((int)_Left | (int)_Right); return (_Left); } \
inline Ty& operator^=(Ty& _Left, Ty _Right) { _Left = (Ty)((int)_Left ^ (int)_Right); return (_Left); } \
inline constexpr Ty operator&(Ty _Left, Ty _Right) { return ((Ty)((int)_Left & (int)_Right)); } \
inline constexpr Ty operator|(Ty _Left, Ty _Right) { return ((Ty)((int)_Left | (int)_Right)); } \
inline constexpr Ty operator^(Ty _Left, Ty _Right) { return ((Ty)((int)_Left ^ (int)_Right)); } \
inline constexpr Ty operator~(Ty _Left) { return ((Ty)~(int)_Left); }
