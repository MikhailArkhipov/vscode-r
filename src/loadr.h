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

#define RHOST_RAPI_PTR(api) fp_##api
#define RHOST_RAPI_DECL(api) extern decltype(api) *RHOST_RAPI_PTR(api)
#define RHOST_RAPI_DECL_END(api) RHOST_RAPI_DECL(api);
#define RHOST_RAPI_STR(api) #api
#define RAPI(api) rhost::rapi::RHOST_RAPI_PTR(api)

#define RHOST_RAPI_SET_COMMON(macro) \
macro(CAR) \
macro(CDR) \
macro(INTEGER) \
macro(LOGICAL) \
macro(PRCODE) \
macro(PRVALUE) \
macro(R_BaseEnv) \
macro(R_CHAR) \
macro(R_CleanUp) \
macro(R_common_command_line) \
macro(R_curErrorBuf) \
macro(R_DefParams) \
macro(R_EmptyEnv) \
macro(R_FunTab) \
macro(R_getEmbeddingDllInfo) \
macro(R_GlobalContext) \
macro(R_GlobalEnv) \
macro(R_IsNA) \
macro(R_lsInternal3) \
macro(R_NaInt) \
macro(R_NamesSymbol) \
macro(R_NaString) \
macro(R_new_custom_connection) \
macro(R_NilValue) \
macro(R_ParseVector) \
macro(R_PreserveObject) \
macro(R_ProcessEvents) \
macro(R_registerRoutines) \
macro(R_ReleaseObject) \
macro(R_RestoreGlobalEnvFromFile) \
macro(R_running_as_main_program) \
macro(R_SaveGlobalEnvToFile) \
macro(R_set_command_line_arguments) \
macro(R_SetParams) \
macro(R_Srcref) \
macro(R_ToplevelExec) \
macro(R_UnboundValue) \
macro(RAW) \
macro(RDEBUG) \
macro(REAL) \
macro(Rf_allocList) \
macro(Rf_allocVector) \
macro(Rf_allocVector3) \
macro(Rf_asChar) \
macro(Rf_asInteger) \
macro(Rf_asLogical) \
macro(Rf_asReal) \
macro(Rf_classgets) \
macro(Rf_deparse1line) \
macro(Rf_duplicate) \
macro(Rf_error) \
macro(Rf_eval) \
macro(Rf_findVar) \
macro(Rf_getAttrib) \
macro(Rf_install) \
macro(Rf_installChar) \
macro(Rf_isEnvironment) \
macro(Rf_isNull) \
macro(Rf_isString) \
macro(Rf_length) \
macro(Rf_mkChar) \
macro(Rf_mkCharCE) \
macro(Rf_mkString) \
macro(Rf_NewEnvironment) \
macro(Rf_protect) \
macro(Rf_ScalarInteger) \
macro(Rf_ScalarLogical) \
macro(Rf_ScalarReal) \
macro(Rf_ScalarString) \
macro(Rf_translateCharUTF8) \
macro(Rf_unprotect) \
macro(SET_RDEBUG) \
macro(SET_STRING_ELT) \
macro(SET_TYPEOF) \
macro(SET_VECTOR_ELT) \
macro(SETCAR) \
macro(STRING_ELT) \
macro(TYPEOF) \
macro(VECTOR_ELT) \
macro(vmaxget) \
macro(vmaxset) \
macro(Rf_onintr) \
macro(R_GE_getVersion) \
macro(Rf_curDevice) \
macro(Rf_selectDevice) \
macro(R_interrupts_suspended) \
macro(R_interrupts_pending) \
macro(R_CheckDeviceAvailable) \
macro(GEcopyDisplayList)

#define RHOST_GD_SET(macro) \
macro(Rf_desc2GEDesc) \
macro(GEplayDisplayList) \
macro(GEplaySnapshot) \
macro(GEcreateSnapshot) \
macro(Rf_ndevNumber) \
macro(GEgetDevice) \
macro(GEkillDevice) \
macro(GEcreateDevDesc) \
macro(GEaddDevice2)

#ifdef _WIN32

#define RHOST_RAPI_SET_WINDOWS(macro) \
macro(CharacterMode) \
macro(get_R_HOME) \
macro(getDLLVersion) \
macro(getRUser) \
macro(in_memsize) \
macro(R_setStartTime) \
macro(R_WaitEvent) \
macro(readconsolecfg) \
macro(Rf_utf8towcs) \
macro(Rf_wtransChar) \
macro(run_Rmainloop) \
macro(setup_Rmainloop)

#define RHOST_RGRAPHAPPAPI_SET(macro) \
macro(GA_initapp)

#define RHOST_RAPI_SET(macro) \
RHOST_RAPI_SET_COMMON(macro) \
RHOST_RAPI_SET_WINDOWS(macro)

#else // POSIX

#define RHOST_RAPI_SET_POSIX(macro) \
macro(ptr_R_Busy) \
macro(ptr_R_ReadConsole) \
macro(ptr_R_ShowMessage) \
macro(ptr_R_WriteConsole) \
macro(ptr_R_WriteConsoleEx) \
macro(R_Consolefile) \
macro(R_FalseValue) \
macro(R_Interactive) \
macro(R_Outputfile) \
macro(R_ReadConsole) \
macro(R_TrueValue) \
macro(Rf_initialize_R) \
macro(Rf_mainloop)

#define RHOST_RAPI_SET(macro) \
RHOST_RAPI_SET_COMMON(macro) \
RHOST_RAPI_SET_POSIX(macro)

#endif

namespace rhost {
    namespace rapi {

        // R.dll/R.so APIs
        RHOST_RAPI_SET(RHOST_RAPI_DECL_END);

#ifdef _WIN32
        // Rgraphapp.dll/Rgraphapp.so APIs
        RHOST_RGRAPHAPPAPI_SET(RHOST_RAPI_DECL_END);
#endif
        void load_r_apis(fs::path& r_dll_dir);
        void unload_r_apis();
    }
}

#ifndef RHOST_NO_API_REDIRECT
#define CAR rhost::rapi::RHOST_RAPI_PTR(CAR)
#define CDR rhost::rapi::RHOST_RAPI_PTR(CDR)
//#define GEaddDevice2 rhost::rapi::RHOST_RAPI_PTR(GEaddDevice2)
//#define GEaddDevice2f rhost::rapi::RHOST_RAPI_PTR(GEaddDevice2f)
#define GEcopyDisplayList rhost::rapi::RHOST_RAPI_PTR(GEcopyDisplayList)
//#define GEcreateDevDesc rhost::rapi::RHOST_RAPI_PTR(GEcreateDevDesc)
//#define GEcreateSnapshot rhost::rapi::RHOST_RAPI_PTR(GEcreateSnapshot)
//#define GEgetDevice rhost::rapi::RHOST_RAPI_PTR(GEgetDevice)
//#define GEkillDevice rhost::rapi::RHOST_RAPI_PTR(GEkillDevice)
//#define GEplayDisplayList rhost::rapi::RHOST_RAPI_PTR(GEplayDisplayList)
//#define GEplaySnapshot rhost::rapi::RHOST_RAPI_PTR(GEplaySnapshot)
#define INTEGER rhost::rapi::RHOST_RAPI_PTR(INTEGER)
#define LOGICAL rhost::rapi::RHOST_RAPI_PTR(LOGICAL)
#define PRCODE rhost::rapi::RHOST_RAPI_PTR(PRCODE)
#define PRVALUE rhost::rapi::RHOST_RAPI_PTR(PRVALUE)
#define R_BaseEnv (*rhost::rapi::RHOST_RAPI_PTR(R_BaseEnv))
#define R_CHAR rhost::rapi::RHOST_RAPI_PTR(R_CHAR)
#define R_CheckDeviceAvailable rhost::rapi::RHOST_RAPI_PTR(R_CheckDeviceAvailable)
#define R_CleanUp rhost::rapi::RHOST_RAPI_PTR(R_CleanUp)
#define R_common_command_line rhost::rapi::RHOST_RAPI_PTR(R_common_command_line)
#define R_curErrorBuf rhost::rapi::RHOST_RAPI_PTR(R_curErrorBuf)
#define R_DefParams rhost::rapi::RHOST_RAPI_PTR(R_DefParams)
#define R_EmptyEnv (*rhost::rapi::RHOST_RAPI_PTR(R_EmptyEnv))
#define R_FunTab (*rhost::rapi::RHOST_RAPI_PTR(R_FunTab))
#define R_GE_getVersion rhost::rapi::RHOST_RAPI_PTR(R_GE_getVersion)
#define R_getEmbeddingDllInfo rhost::rapi::RHOST_RAPI_PTR(R_getEmbeddingDllInfo)
#define R_GlobalContext rhost::rapi::RHOST_RAPI_PTR(R_GlobalContext)
#define R_GlobalEnv (*rhost::rapi::RHOST_RAPI_PTR(R_GlobalEnv))
#define R_interrupts_pending (*rhost::rapi::RHOST_RAPI_PTR(R_interrupts_pending))
#define R_interrupts_suspended (*rhost::rapi::RHOST_RAPI_PTR(R_interrupts_suspended))
#define R_IsNA rhost::rapi::RHOST_RAPI_PTR(R_IsNA)
#define R_lsInternal3 rhost::rapi::RHOST_RAPI_PTR(R_lsInternal3)
#define R_NaInt (*rhost::rapi::RHOST_RAPI_PTR(R_NaInt))
#define R_NamesSymbol (*rhost::rapi::RHOST_RAPI_PTR(R_NamesSymbol))
#define R_NaString (*rhost::rapi::RHOST_RAPI_PTR(R_NaString))
#define R_new_custom_connection rhost::rapi::RHOST_RAPI_PTR(R_new_custom_connection)
#define R_NilValue (*rhost::rapi::RHOST_RAPI_PTR(R_NilValue))
#define R_ParseVector rhost::rapi::RHOST_RAPI_PTR(R_ParseVector)
#define R_PreserveObject rhost::rapi::RHOST_RAPI_PTR(R_PreserveObject)
#define R_ProcessEvents rhost::rapi::RHOST_RAPI_PTR(R_ProcessEvents)
#define R_registerRoutines rhost::rapi::RHOST_RAPI_PTR(R_registerRoutines)
#define R_ReleaseObject rhost::rapi::RHOST_RAPI_PTR(R_ReleaseObject)
#define R_RestoreGlobalEnvFromFile rhost::rapi::RHOST_RAPI_PTR(R_RestoreGlobalEnvFromFile)
#define R_running_as_main_program (*rhost::rapi::RHOST_RAPI_PTR(R_running_as_main_program))
#define R_SaveGlobalEnvToFile rhost::rapi::RHOST_RAPI_PTR(R_SaveGlobalEnvToFile)
#define R_set_command_line_arguments rhost::rapi::RHOST_RAPI_PTR(R_set_command_line_arguments)
#define R_SetParams rhost::rapi::RHOST_RAPI_PTR(R_SetParams)
#define R_Srcref (*rhost::rapi::RHOST_RAPI_PTR(R_Srcref))
#define R_ToplevelExec rhost::rapi::RHOST_RAPI_PTR(R_ToplevelExec)
#define R_UnboundValue (*rhost::rapi::RHOST_RAPI_PTR(R_UnboundValue))
#define RAW rhost::rapi::RHOST_RAPI_PTR(RAW)
#define RDEBUG rhost::rapi::RHOST_RAPI_PTR(RDEBUG)
#define REAL rhost::rapi::RHOST_RAPI_PTR(REAL)
#define Rf_allocList rhost::rapi::RHOST_RAPI_PTR(Rf_allocList)
#define Rf_allocVector rhost::rapi::RHOST_RAPI_PTR(Rf_allocVector)
#define Rf_allocVector3 rhost::rapi::RHOST_RAPI_PTR(Rf_allocVector3)
#define Rf_asChar rhost::rapi::RHOST_RAPI_PTR(Rf_asChar)
#define Rf_asInteger rhost::rapi::RHOST_RAPI_PTR(Rf_asInteger)
#define Rf_asLogical rhost::rapi::RHOST_RAPI_PTR(Rf_asLogical)
#define Rf_asReal rhost::rapi::RHOST_RAPI_PTR(Rf_asReal)
#define Rf_classgets rhost::rapi::RHOST_RAPI_PTR(Rf_classgets)
#define Rf_curDevice rhost::rapi::RHOST_RAPI_PTR(Rf_curDevice)
#define Rf_deparse1line rhost::rapi::RHOST_RAPI_PTR(Rf_deparse1line)
//#define Rf_desc2GEDesc rhost::rapi::RHOST_RAPI_PTR(Rf_desc2GEDesc)
#define Rf_duplicate rhost::rapi::RHOST_RAPI_PTR(Rf_duplicate)
#define Rf_error rhost::rapi::RHOST_RAPI_PTR(Rf_error)
#define Rf_eval rhost::rapi::RHOST_RAPI_PTR(Rf_eval)
#define Rf_findVar rhost::rapi::RHOST_RAPI_PTR(Rf_findVar)
#define Rf_getAttrib rhost::rapi::RHOST_RAPI_PTR(Rf_getAttrib)
#define Rf_install rhost::rapi::RHOST_RAPI_PTR(Rf_install)
#define Rf_installChar rhost::rapi::RHOST_RAPI_PTR(Rf_installChar)
#define Rf_isEnvironment rhost::rapi::RHOST_RAPI_PTR(Rf_isEnvironment)
#define Rf_isNull rhost::rapi::RHOST_RAPI_PTR(Rf_isNull)
#define Rf_isString rhost::rapi::RHOST_RAPI_PTR(Rf_isString)
#define Rf_length rhost::rapi::RHOST_RAPI_PTR(Rf_length)
#define Rf_mkChar rhost::rapi::RHOST_RAPI_PTR(Rf_mkChar)
#define Rf_mkCharCE rhost::rapi::RHOST_RAPI_PTR(Rf_mkCharCE)
#define Rf_mkString rhost::rapi::RHOST_RAPI_PTR(Rf_mkString)
//#define Rf_ndevNumber rhost::rapi::RHOST_RAPI_PTR(Rf_ndevNumber)
#define Rf_NewEnvironment rhost::rapi::RHOST_RAPI_PTR(Rf_NewEnvironment)
#define Rf_onintr rhost::rapi::RHOST_RAPI_PTR(Rf_onintr)
#define Rf_protect rhost::rapi::RHOST_RAPI_PTR(Rf_protect)
#define Rf_ScalarInteger rhost::rapi::RHOST_RAPI_PTR(Rf_ScalarInteger)
#define Rf_ScalarLogical rhost::rapi::RHOST_RAPI_PTR(Rf_ScalarLogical)
#define Rf_ScalarReal rhost::rapi::RHOST_RAPI_PTR(Rf_ScalarReal)
#define Rf_ScalarString rhost::rapi::RHOST_RAPI_PTR(Rf_ScalarString)
#define Rf_selectDevice rhost::rapi::RHOST_RAPI_PTR(Rf_selectDevice)
#define Rf_translateCharUTF8 rhost::rapi::RHOST_RAPI_PTR(Rf_translateCharUTF8)
#define Rf_unprotect rhost::rapi::RHOST_RAPI_PTR(Rf_unprotect)
#define SET_RDEBUG rhost::rapi::RHOST_RAPI_PTR(SET_RDEBUG)
#define SET_STRING_ELT rhost::rapi::RHOST_RAPI_PTR(SET_STRING_ELT)
#define SET_TYPEOF rhost::rapi::RHOST_RAPI_PTR(SET_TYPEOF)
#define SET_VECTOR_ELT rhost::rapi::RHOST_RAPI_PTR(SET_VECTOR_ELT)
#define SETCAR rhost::rapi::RHOST_RAPI_PTR(SETCAR)
#define STRING_ELT rhost::rapi::RHOST_RAPI_PTR(STRING_ELT)
#define TYPEOF rhost::rapi::RHOST_RAPI_PTR(TYPEOF)
#define VECTOR_ELT rhost::rapi::RHOST_RAPI_PTR(VECTOR_ELT)
#define vmaxget rhost::rapi::RHOST_RAPI_PTR(vmaxget)
#define vmaxset rhost::rapi::RHOST_RAPI_PTR(vmaxset)

#ifdef _WIN32

#define get_R_HOME rhost::rapi::RHOST_RAPI_PTR(get_R_HOME)
#define getDLLVersion rhost::rapi::RHOST_RAPI_PTR(getDLLVersion)
#define getRUser rhost::rapi::RHOST_RAPI_PTR(getRUser)
#define in_memsize rhost::rapi::RHOST_RAPI_PTR(in_memsize)
#define R_setStartTime rhost::rapi::RHOST_RAPI_PTR(R_setStartTime)
#define R_WaitEvent rhost::rapi::RHOST_RAPI_PTR(R_WaitEvent)
#define readconsolecfg rhost::rapi::RHOST_RAPI_PTR(readconsolecfg)
#define Rf_utf8towcs rhost::rapi::RHOST_RAPI_PTR(Rf_utf8towcs)
#define Rf_wtransChar rhost::rapi::RHOST_RAPI_PTR(Rf_wtransChar)
#define run_Rmainloop rhost::rapi::RHOST_RAPI_PTR(run_Rmainloop)
#define setup_Rmainloop rhost::rapi::RHOST_RAPI_PTR(setup_Rmainloop)

#define GA_initapp rhost::rapi::RHOST_RAPI_PTR(GA_initapp)

#else // POSIX

#define ptr_R_Busy (*rhost::rapi::RHOST_RAPI_PTR(ptr_R_Busy))
#define ptr_R_ReadConsole (*rhost::rapi::RHOST_RAPI_PTR(ptr_R_ReadConsole))
#define ptr_R_ShowMessage (*rhost::rapi::RHOST_RAPI_PTR(ptr_R_ShowMessage))
#define ptr_R_WriteConsole (*rhost::rapi::RHOST_RAPI_PTR(ptr_R_WriteConsole))
#define ptr_R_WriteConsoleEx (*rhost::rapi::RHOST_RAPI_PTR(ptr_R_WriteConsoleEx))
#define R_Consolefile rhost::rapi::RHOST_RAPI_PTR(R_Consolefile)
#define R_FalseValue (*rhost::rapi::RHOST_RAPI_PTR(R_FalseValue))
#define R_Interactive rhost::rapi::RHOST_RAPI_PTR(R_Interactive)
#define R_Outputfile rhost::rapi::RHOST_RAPI_PTR(R_Outputfile)
#define R_ReadConsole rhost::rapi::RHOST_RAPI_PTR(R_ReadConsole)
#define R_TrueValue (*rhost::rapi::RHOST_RAPI_PTR(R_TrueValue))
#define Rf_initialize_R rhost::rapi::RHOST_RAPI_PTR(Rf_initialize_R)
#define Rf_mainloop rhost::rapi::RHOST_RAPI_PTR(Rf_mainloop)

#endif 

#endif // RHOST_NO_API_REDIRECT
