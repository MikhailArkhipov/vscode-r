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

#ifdef _WIN32
#define Win32
#endif // _WIN32

#define R_INTERFACE_PTRS
#define R_NO_REMAP
#undef ERROR

#include "stdafx.h"

#include "R.h"
#include "Rinternals.h"

#ifndef _WIN32
#include "Rinterface.h"
#endif

#include "Rembedded.h"
#include "Rversion.h"

#define class class_
#define private private_
#include "R_ext/Connections.h"
#undef class
#undef private

#include "R_ext/Rdynload.h"
#include "R_ext/RStartup.h"
#include "R_ext/Parse.h"
#include "R_ext/GraphicsEngine.h"
#include "R_ext/GraphicsDevice.h"

#define R_FALSE Rboolean::FALSE
#define R_TRUE Rboolean::TRUE

extern "C" {
    extern SEXP Rf_deparse1line(SEXP, Rboolean);
    extern SEXP Rf_NewEnvironment(SEXP, SEXP, SEXP);
    extern void R_CleanUp(SA_TYPE, int, int);
    extern void run_Rmainloop(void);
    typedef SEXP(*CCODE)(SEXP, SEXP, SEXP, SEXP);

    enum {
        R_32_GE_version = 10,
        R_33_GE_version = 11,
        R_34_GE_version = 12,
    };

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

#ifdef _WIN32
    typedef struct _sigjmp_buf {
        jmp_buf buf;
        int sigmask;
        int savedmask;
    } sigjmp_buf[1];
#endif

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
        PP_INVALID = 0,
        PP_ASSIGN = 1,
        PP_ASSIGN2 = 2,
        PP_BINARY = 3,
        PP_BINARY2 = 4,
        PP_BREAK = 5,
        PP_CURLY = 6,
        PP_FOR = 7,
        PP_FUNCALL = 8,
        PP_FUNCTION = 9,
        PP_IF = 10,
        PP_NEXT = 11,
        PP_PAREN = 12,
        PP_RETURN = 13,
        PP_SUBASS = 14,
        PP_SUBSET = 15,
        PP_WHILE = 16,
        PP_UNARY = 17,
        PP_DOLLAR = 18,
        PP_FOREIGN = 19,
        PP_REPEAT = 20
    } PPkind;

    typedef enum {
        PREC_FN = 0,
        PREC_EQ = 1,
        PREC_LEFT = 2,
        PREC_RIGHT = 3,
        PREC_TILDE = 4,
        PREC_OR = 5,
        PREC_AND = 6,
        PREC_NOT = 7,
        PREC_COMPARE = 8,
        PREC_SUM = 9,
        PREC_PROD = 10,
        PREC_PERCENT = 11,
        PREC_COLON = 12,
        PREC_SIGN = 13,
        PREC_POWER = 14,
        PREC_SUBSET = 15,
        PREC_DOLLAR = 16,
        PREC_NS = 17
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

    RHOST_IMPORT extern FUNTAB R_FunTab[];
    RHOST_IMPORT extern int R_running_as_main_program;

#ifdef _WIN32
    RHOST_IMPORT extern RCNTXT* R_GlobalContext;
    RHOST_IMPORT extern void R_SaveGlobalEnvToFile(const char *);
    RHOST_IMPORT extern void R_RestoreGlobalEnvFromFile(const char *, Rboolean);
    RHOST_IMPORT extern SEXP in_memsize(SEXP);
    RHOST_IMPORT extern void run_Rmainloop(void);
    RHOST_IMPORT extern UImode  CharacterMode;
    RHOST_IMPORT extern size_t Rf_utf8towcs(wchar_t *wc, const char *s, size_t n);
    RHOST_IMPORT extern const wchar_t* Rf_wtransChar(SEXP s);
#endif

#ifdef _WIN32
#undef Win32
#endif // _WIN32
}


#include "loadr.h"
#include "r_gd_api.h"
