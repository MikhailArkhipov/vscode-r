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
 // - Rembedded.h
 // - Rinternals.h
 // - R_ext/Boolean.h
 // - R_ext/Rdynload.h
 // - R_ext/RStartup.h

#pragma once
#include "stdafx.h"

extern "C" {
    typedef int R_len_t;
    typedef ptrdiff_t R_xlen_t;

    // Renamed to R_FALSE and R_TRUE to avoid conflicts with Win32 FALSE and TRUE.
    typedef enum { R_FALSE = 0, R_TRUE } Rboolean;

    typedef struct {
        jmp_buf buf;
        int sigmask;
        int savedmask;
    } sigjmp_buf[1];

    typedef struct SEXPREC *SEXP;

    enum SEXPTYPE : unsigned int {
        NILSXP = 0,
        SYMSXP = 1,
        LISTSXP = 2,
        CLOSXP = 3,
        ENVSXP = 4,
        PROMSXP = 5,
        LANGSXP = 6,
        SPECIALSXP = 7,
        BUILTINSXP = 8,
        CHARSXP = 9,
        LGLSXP = 10,
        INTSXP = 13,
        REALSXP = 14,
        CPLXSXP = 15,
        STRSXP = 16,
        DOTSXP = 17,
        ANYSXP = 18,
        VECSXP = 19,
        EXPRSXP = 20,
        BCODESXP = 21,
        EXTPTRSXP = 22,
        WEAKREFSXP = 23,
        RAWSXP = 24,
        S4SXP = 25,
        NEWSXP = 30,
        FREESXP = 31,
        FUNSXP = 99,
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
        PARSE_NULL,
        PARSE_OK,
        PARSE_INCOMPLETE,
        PARSE_ERROR,
        PARSE_EOF
    } ParseStatus;

    typedef enum {
        RGui,
        RTerm,
        LinkDLL
    } UImode;

    typedef enum {
        SA_NORESTORE,
        SA_RESTORE,
        SA_DEFAULT,
        SA_NOSAVE,
        SA_SAVE,
        SA_SAVEASK,
        SA_SUICIDE
    } SA_TYPE;

    typedef struct {
        double r;
        double i;
    } Rcomplex;

    typedef unsigned char Rbyte;

    typedef void(*R_CFinalizer_t)(SEXP);
    typedef int(*blah1) (const char *, char *, int, int);
    typedef void(*blah2) (const char *, int);
    typedef void(*blah3) (void);
    typedef void(*blah4) (const char *);
    typedef int(*blah5) (const char *);
    typedef void(*blah6) (int);
    typedef void(*blah7) (const char *, int, int);

    typedef struct {
        Rboolean R_Quiet;
        Rboolean R_Slave;
        Rboolean R_Interactive;
        Rboolean R_Verbose;
        Rboolean LoadSiteFile;
        Rboolean LoadInitFile;
        Rboolean DebugInitFile;
        SA_TYPE RestoreAction;
        SA_TYPE SaveAction;
        size_t vsize;
        size_t nsize;
        size_t max_vsize;
        size_t max_nsize;
        size_t ppsize;
        int NoRenviron;

#ifdef _WIN32
        char *rhome;
        char *home;
        blah1 ReadConsole;
        blah2 WriteConsole;
        blah3 CallBack;
        blah4 ShowMessage;
        blah5 YesNoCancel;
        blah6 Busy;
        UImode CharacterMode;
        blah7 WriteConsoleEx;
#endif
    } structRstart, *Rstart;

    typedef struct Rconn {
        char* class_;
        char* description;
        int enc;
        char mode[5];
        Rboolean text, isopen, incomplete, canread, canwrite, canseek, blocking, isGzcon;
        Rboolean(*open)(struct Rconn *);
        void(*close)(struct Rconn *);
        void(*destroy)(struct Rconn *);
        int(*vfprintf)(struct Rconn *, const char *, va_list);
        int(*fgetc)(struct Rconn *);
        int(*fgetc_internal)(struct Rconn *);
        double(*seek)(struct Rconn *, double, int, int);
        void(*truncate)(struct Rconn *);
        int(*fflush)(struct Rconn *);
        size_t(*read)(void *, size_t, size_t, struct Rconn *);
        size_t(*write)(const void *, size_t, size_t, struct Rconn *);
        int nPushBack, posPushBack;
        char **PushBack;
        int save, save2;
        char encname[101];
        void *inconv, *outconv;
        char iconvbuff[25], oconvbuff[50], *next, init_out[25];
        short navail, inavail;
        Rboolean EOF_signalled;
        Rboolean UTF8out;
        void *id;
        void *ex_ptr;
        void *private_;
        int status;
    } RConn;

    typedef RConn* Rconnection;

#ifdef _WIN32
    __declspec(dllimport) extern UImode CharacterMode;
    __declspec(dllimport) extern RCNTXT* R_GlobalContext;
    __declspec(dllimport) extern SEXP
        R_GlobalEnv, R_EmptyEnv, R_BaseEnv, R_BaseNamespace, R_Srcref, R_NilValue,
        R_TrueValue, R_FalseValue, R_UnboundValue, R_MissingArg, R_LogicalNAValue,
        R_NaString, R_BlankString, R_BlankScalarString, R_NamesSymbol;
    __declspec(dllimport) extern int R_DirtyImage;
    __declspec(dllimport) extern char *R_TempDir;
    __declspec(dllimport) extern int UserBreak;
    __declspec(dllimport) extern double R_NaN;
    __declspec(dllimport) extern double R_PosInf;
    __declspec(dllimport) extern double R_NegInf;
    __declspec(dllimport) extern double R_NaReal;
    __declspec(dllimport) extern int R_NaInt;
#endif

    extern void R_DefParams(Rstart);
    extern void R_SetParams(Rstart);
    extern void R_SetWin32(Rstart);
    extern void R_SizeFromEnv(Rstart);
    extern void R_common_command_line(int*, char**, Rstart);
    extern void R_set_command_line_arguments(int argc, char** argv);
    extern void R_RegisterCFinalizerEx(SEXP s, R_CFinalizer_t fun, Rboolean onexit);
    extern void R_ReplDLLinit(void);
    extern int R_ReplDLLdo1(void);
    extern void R_setStartTime(void);
    extern void R_RunExitFinalizers(void);
    extern void Rf_KillAllDevices(void);
    extern void R_CleanTempDir(void);
    extern void R_SaveGlobalEnv(void);
    extern SEXP R_ParseVector(SEXP, int, ParseStatus*, SEXP);
    extern SEXP R_tryEval(SEXP, SEXP, int*);
    extern SEXP R_tryEvalSilent(SEXP, SEXP, int*);
    extern const char *R_curErrorBuf();
    extern void R_PreserveObject(SEXP);
    extern void R_ReleaseObject(SEXP);
    extern Rboolean R_ToplevelExec(void(*fun)(void *), void *data);
    extern SEXP R_new_custom_connection(const char *description, const char *mode, const char *class_name, Rconnection *ptr);
    extern SEXP R_lsInternal(SEXP, Rboolean);
    extern SEXP R_lsInternal3(SEXP, Rboolean, Rboolean);

    extern int Rf_initialize_R(int ac, char** av);
    extern int Rf_initEmbeddedR(int argc, char** argv);
    extern void Rf_endEmbeddedR(int fatal);
    extern SEXP Rf_protect(SEXP);
    extern void Rf_unprotect(int);
    extern void Rf_unprotect_ptr(SEXP);
    extern SEXP Rf_allocVector(SEXPTYPE, R_xlen_t);
    extern SEXP Rf_allocVector3(SEXPTYPE, R_xlen_t, /*R_allocator_t*/ void*);
    extern SEXP Rf_allocList(int);
    extern R_len_t Rf_length(SEXP);
    extern SEXP Rf_duplicate(SEXP);
    extern void Rf_defineVar(SEXP, SEXP, SEXP);
    extern SEXP Rf_findVar(SEXP, SEXP);
    extern SEXP Rf_eval(SEXP, SEXP);
    extern void Rf_onintr();
    __declspec(noreturn) extern void Rf_error(const char *, ...);
    extern void Rf_init_con(Rconnection, const char *description, int enc, const char* const mode);
    extern SEXP Rf_deparse1(SEXP, Rboolean, int);
    extern SEXP Rf_deparse1s(SEXP);
    extern SEXP Rf_deparse1line(SEXP, Rboolean);
    extern SEXP Rf_namesgets(SEXP, SEXP);

    extern Rboolean Rf_isNull(SEXP s);
    extern Rboolean Rf_isSymbol(SEXP s);
    extern Rboolean Rf_isLogical(SEXP s);
    extern Rboolean Rf_isReal(SEXP s);
    extern Rboolean Rf_isComplex(SEXP s);
    extern Rboolean Rf_isExpression(SEXP s);
    extern Rboolean Rf_isEnvironment(SEXP s);
    extern Rboolean Rf_isString(SEXP s);
    extern Rboolean Rf_isObject(SEXP s);

    extern Rboolean R_IsNA(double);
    extern Rboolean R_IsNaN(double);

    extern SEXP Rf_asChar(SEXP);
    extern SEXP Rf_coerceVector(SEXP, SEXPTYPE);
    extern SEXP Rf_PairToVectorList(SEXP x);
    extern SEXP Rf_VectorToPairList(SEXP x);
    extern SEXP Rf_asCharacterFactor(SEXP x);
    extern int Rf_asLogical(SEXP x);
    extern int Rf_asInteger(SEXP x);
    extern double Rf_asReal(SEXP x);
    extern Rcomplex Rf_asComplex(SEXP x);

    extern SEXP Rf_mkChar(const char*);
    extern SEXP Rf_mkString(const char*);
    extern SEXP Rf_install(const char*);
    extern SEXP Rf_installChar(SEXP);
    extern SEXP Rf_classgets(SEXP, SEXP);
    extern SEXP Rf_NewEnvironment(SEXP, SEXP, SEXP);
    extern SEXP Rf_getAttrib(SEXP, SEXP);

    extern SEXP Rf_ScalarComplex(Rcomplex);
    extern SEXP Rf_ScalarInteger(int);
    extern SEXP Rf_ScalarLogical(int);
    extern SEXP Rf_ScalarRaw(Rbyte);
    extern SEXP Rf_ScalarReal(double);
    extern SEXP Rf_ScalarString(SEXP);

    typedef enum {
        CE_NATIVE = 0,
        CE_UTF8 = 1,
        CE_LATIN1 = 2,
        CE_BYTES = 3,
        CE_SYMBOL = 5,
        CE_ANY = 99
    } cetype_t;

    extern cetype_t Rf_getCharCE(SEXP);
    extern SEXP Rf_mkCharCE(const char*, cetype_t);
    extern SEXP Rf_mkCharLenCE(const char*, int, cetype_t);
    extern const char *Rf_reEnc(const char* x, cetype_t ce_in, cetype_t ce_out, int subst);
    extern const char* Rf_translateChar(SEXP);
    extern const char* Rf_translateChar0(SEXP);
    extern const char* Rf_translateCharUTF8(SEXP);

    extern void setup_Rmainloop(void);
    extern void run_Rmainloop(void);
    extern void CleanEd(void);

    extern int TYPEOF(SEXP x);
    extern void SET_TYPEOF(SEXP x, int v);
    extern SEXP STRING_ELT(SEXP x, R_xlen_t i);
    extern SEXP VECTOR_ELT(SEXP x, R_xlen_t i);
    extern void SET_STRING_ELT(SEXP x, R_xlen_t i, SEXP v);
    extern SEXP SET_VECTOR_ELT(SEXP x, R_xlen_t i, SEXP v);
    extern const char* R_CHAR(SEXP x);
    extern int PRSEEN(SEXP x);
    extern SEXP PRCODE(SEXP x);
    extern SEXP PRENV(SEXP x);
    extern SEXP PRVALUE(SEXP x);

    extern SEXP TAG(SEXP e);
    extern void SET_TAG(SEXP x, SEXP y);
    extern int MISSING(SEXP x);
    extern void SET_MISSING(SEXP x, int v);
    extern SEXP CAR(SEXP e);
    extern SEXP SETCAR(SEXP x, SEXP y);
    extern SEXP CDR(SEXP e);
    extern SEXP SETCDR(SEXP x, SEXP y);

    extern int RDEBUG(SEXP x);
    extern int RSTEP(SEXP x);
    extern int RTRACE(SEXP x);
    extern void SET_RDEBUG(SEXP x, int v);
    extern void SET_RSTEP(SEXP x, int v);
    extern void SET_RTRACE(SEXP x, int v);

    extern int* LOGICAL(SEXP x);
    extern int* INTEGER(SEXP x);
    extern double* REAL(SEXP x);
    extern Rbyte* RAW(SEXP x);

    extern int IS_UTF8(SEXP x);

    extern void* vmaxget(void);
    extern void vmaxset(const void*);

#ifdef _WIN32
    extern char *getDLLVersion(void), *getRUser(void), *get_R_HOME(void);
    extern void setup_term_ui(void);
    extern Rboolean AllDevicesKilled;
    extern void editorcleanall(void);
    extern int GA_initapp(int, char**);
    extern void GA_appcleanup(void);
    extern void readconsolecfg(void);
    SEXP in_memsize(SEXP ssize);
#endif

    typedef void * (*DL_FUNC)();
    typedef unsigned int R_NativePrimitiveArgType;
    typedef unsigned int R_NativeObjectArgType;
    typedef enum { R_ARG_IN, R_ARG_OUT, R_ARG_IN_OUT, R_IRRELEVANT } R_NativeArgStyle;

    typedef struct {
        const char *name;
        DL_FUNC     fun;
        int         numArgs;
        R_NativePrimitiveArgType *types;
        R_NativeArgStyle         *styles;
    } R_CMethodDef;
    typedef R_CMethodDef R_FortranMethodDef;

    typedef struct {
        const char *name;
        DL_FUNC     fun;
        int         numArgs;
    } R_CallMethodDef;
    typedef R_CallMethodDef R_ExternalMethodDef;

    typedef struct _DllInfo DllInfo;

    int R_registerRoutines(DllInfo *info, const R_CMethodDef * const croutines,
        const R_CallMethodDef * const callRoutines,
        const R_FortranMethodDef * const fortranRoutines,
        const R_ExternalMethodDef * const externalRoutines);

    Rboolean R_useDynamicSymbols(DllInfo *info, Rboolean value);
    Rboolean R_forceSymbols(DllInfo *info, Rboolean value);

    DllInfo *R_getEmbeddingDllInfo(void);

    void R_WaitEvent();
    void R_ProcessEvents();
    void R_Suicide(const char *);

    typedef SEXP(*CCODE)(SEXP, SEXP, SEXP, SEXP);

    /* Information for Deparsing Expressions */
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
        PREC_LEFT = 1,
        PREC_EQ = 2,
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
        PREC_DOLLAR = 15,
        PREC_NS = 16,
        PREC_SUBSET = 17
    } PPprec;

    typedef struct {
        PPkind kind;     /* deparse kind */
        PPprec precedence; /* operator precedence */
        unsigned int rightassoc;  /* right associative? */
    } PPinfo;

    /* The type definitions for the table of built-in functions. */
    /* This table can be found in ../main/names.c */
    typedef struct {
        char   *name;    /* print name */
        CCODE  cfun;     /* c-code address */
        int    code;     /* offset within c-code */
        int    eval;     /* evaluate args? */
        int    arity;    /* function arity */
        PPinfo gram;     /* pretty-print info */
    } FUNTAB;

    __declspec(dllimport) extern FUNTAB R_FunTab[];

    enum {
        KEEPINTEGER = 1,
        QUOTEEXPRESSIONS = 2,
        SHOWATTRIBUTES = 4,
        USESOURCE = 8,
        WARNINCOMPLETE = 16,
        DELAYPROMISES = 32,
        KEEPNA = 64,
        S_COMPAT = 128,
        HEXNUMERIC = 256,
        DIGITS16 = 512,
        SIMPLEDEPARSE = 0,
        DEFAULTDEPARSE = KEEPINTEGER | KEEPNA,
        FORSOURCING = KEEPNA | WARNINCOMPLETE | USESOURCE | SHOWATTRIBUTES | QUOTEEXPRESSIONS | KEEPINTEGER
    };

    extern size_t Rf_utf8towcs(wchar_t *wc, const char *s, size_t n);
    extern const wchar_t* Rf_wtransChar(SEXP s);
}
