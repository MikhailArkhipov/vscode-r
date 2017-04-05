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

#define _RAPI_PTR(api) fp_##api
#define _RAPI_DECL(api) extern decltype(api) *_RAPI_PTR(api)
#define _RAPI_DECL_END(api) _RAPI_DECL(api);
#define _RAPI_DEFINE(api) decltype(api) *_RAPI_PTR(api)
#define _RAPI_DEFINE_NULLPTR(api) _RAPI_DEFINE(api) = nullptr;
#define _RAPI_STR(api) #api
#define RAPI(api) rhost::rapi::_RAPI_PTR(api)

#define _RAPI_SET(macro) \
macro(CAR) \
macro(CDR) \
macro(CharacterMode) \
macro(GEaddDevice2) \
macro(GEaddDevice2f) \
macro(GEcopyDisplayList) \
macro(GEcreateDevDesc) \
macro(GEcreateSnapshot) \
macro(GEgetDevice) \
macro(GEkillDevice) \
macro(GEplayDisplayList) \
macro(GEplaySnapshot) \
macro(get_R_HOME) \
macro(getDLLVersion) \
macro(getRUser) \
macro(in_memsize) \
macro(INTEGER) \
macro(LOGICAL) \
macro(PRCODE) \
macro(PRVALUE) \
macro(R_BaseEnv) \
macro(R_CHAR) \
macro(R_CheckDeviceAvailable) \
macro(R_CleanUp) \
macro(R_common_command_line) \
macro(R_curErrorBuf) \
macro(R_DefParams) \
macro(R_EmptyEnv) \
macro(R_FunTab) \
macro(R_GE_getVersion) \
macro(R_getEmbeddingDllInfo) \
macro(R_GlobalContext) \
macro(R_GlobalEnv) \
macro(R_interrupts_pending) \
macro(R_interrupts_suspended) \
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
macro(R_SaveGlobalEnvToFile) \
macro(R_set_command_line_arguments) \
macro(R_SetParams) \
macro(R_setStartTime) \
macro(R_Srcref) \
macro(R_ToplevelExec) \
macro(R_UnboundValue) \
macro(R_WaitEvent) \
macro(RAW) \
macro(RDEBUG) \
macro(readconsolecfg) \
macro(REAL) \
macro(Rf_allocList) \
macro(Rf_allocVector) \
macro(Rf_allocVector3) \
macro(Rf_asChar) \
macro(Rf_asInteger) \
macro(Rf_asLogical) \
macro(Rf_asReal) \
macro(Rf_classgets) \
macro(Rf_curDevice) \
macro(Rf_deparse1line) \
macro(Rf_desc2GEDesc) \
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
macro(Rf_ndevNumber) \
macro(Rf_NewEnvironment) \
macro(Rf_onintr) \
macro(Rf_protect) \
macro(Rf_ScalarInteger) \
macro(Rf_ScalarLogical) \
macro(Rf_ScalarReal) \
macro(Rf_ScalarString) \
macro(Rf_selectDevice) \
macro(Rf_translateCharUTF8) \
macro(Rf_unprotect) \
macro(Rf_utf8towcs) \
macro(Rf_wtransChar) \
macro(run_Rmainloop) \
macro(SET_RDEBUG) \
macro(SET_STRING_ELT) \
macro(SET_TYPEOF) \
macro(SET_VECTOR_ELT) \
macro(SETCAR) \
macro(setup_Rmainloop) \
macro(STRING_ELT) \
macro(TYPEOF) \
macro(VECTOR_ELT) \
macro(vmaxget) \
macro(vmaxset) \

#ifdef _WIN32
#define _RGRAPHAPPAPI_SET(macro) \
macro(GA_initapp) 
#endif

namespace rhost {
    namespace rapi {

        // R.dll/R.so APIs
        _RAPI_SET(_RAPI_DECL_END);

#ifdef _WIN32
        // Rgraphapp.dll/Rgraphapp.so APIs
        _RGRAPHAPPAPI_SET(_RAPI_DECL_END);
#endif
        void load_r_apis(fs::path r_dll_dir);
        void unload_r_apis();
    }
}

#ifndef RHOST_NO_API_REDIRECT
#define CAR rhost::rapi::_RAPI_PTR(CAR)
#define CDR rhost::rapi::_RAPI_PTR(CDR)
#define GEaddDevice2 rhost::rapi::_RAPI_PTR(GEaddDevice2)
#define GEaddDevice2f rhost::rapi::_RAPI_PTR(GEaddDevice2f)
#define GEcopyDisplayList rhost::rapi::_RAPI_PTR(GEcopyDisplayList)
#define GEcreateDevDesc rhost::rapi::_RAPI_PTR(GEcreateDevDesc)
#define GEcreateSnapshot rhost::rapi::_RAPI_PTR(GEcreateSnapshot)
#define GEgetDevice rhost::rapi::_RAPI_PTR(GEgetDevice)
#define GEkillDevice rhost::rapi::_RAPI_PTR(GEkillDevice)
#define GEplayDisplayList rhost::rapi::_RAPI_PTR(GEplayDisplayList)
#define GEplaySnapshot rhost::rapi::_RAPI_PTR(GEplaySnapshot)
#define get_R_HOME rhost::rapi::_RAPI_PTR(get_R_HOME)
#define getDLLVersion rhost::rapi::_RAPI_PTR(getDLLVersion)
#define getRUser rhost::rapi::_RAPI_PTR(getRUser)
#define in_memsize rhost::rapi::_RAPI_PTR(in_memsize)
#define INTEGER rhost::rapi::_RAPI_PTR(INTEGER)
#define LOGICAL rhost::rapi::_RAPI_PTR(LOGICAL)
#define PRCODE rhost::rapi::_RAPI_PTR(PRCODE)
#define PRVALUE rhost::rapi::_RAPI_PTR(PRVALUE)
#define R_BaseEnv (*rhost::rapi::_RAPI_PTR(R_BaseEnv))
#define R_CHAR rhost::rapi::_RAPI_PTR(R_CHAR)
#define R_CheckDeviceAvailable rhost::rapi::_RAPI_PTR(R_CheckDeviceAvailable)
#define R_CleanUp rhost::rapi::_RAPI_PTR(R_CleanUp)
#define R_common_command_line rhost::rapi::_RAPI_PTR(R_common_command_line)
#define R_curErrorBuf rhost::rapi::_RAPI_PTR(R_curErrorBuf)
#define R_DefParams rhost::rapi::_RAPI_PTR(R_DefParams)
#define R_EmptyEnv (*rhost::rapi::_RAPI_PTR(R_EmptyEnv))
#define R_FunTab (*rhost::rapi::_RAPI_PTR(R_FunTab))
#define R_GE_getVersion rhost::rapi::_RAPI_PTR(R_GE_getVersion)
#define R_getEmbeddingDllInfo rhost::rapi::_RAPI_PTR(R_getEmbeddingDllInfo)
#define R_GlobalContext rhost::rapi::_RAPI_PTR(R_GlobalContext)
#define R_GlobalEnv (*rhost::rapi::_RAPI_PTR(R_GlobalEnv))
#define R_interrupts_pending (*rhost::rapi::_RAPI_PTR(R_interrupts_pending))
#define R_interrupts_suspended (*rhost::rapi::_RAPI_PTR(R_interrupts_suspended))
#define R_IsNA rhost::rapi::_RAPI_PTR(R_IsNA)
#define R_lsInternal3 rhost::rapi::_RAPI_PTR(R_lsInternal3)
#define R_NaInt (*rhost::rapi::_RAPI_PTR(R_NaInt))
#define R_NamesSymbol (*rhost::rapi::_RAPI_PTR(R_NamesSymbol))
#define R_NaString (*rhost::rapi::_RAPI_PTR(R_NaString))
#define R_new_custom_connection rhost::rapi::_RAPI_PTR(R_new_custom_connection)
#define R_NilValue (*rhost::rapi::_RAPI_PTR(R_NilValue))
#define R_ParseVector rhost::rapi::_RAPI_PTR(R_ParseVector)
#define R_PreserveObject rhost::rapi::_RAPI_PTR(R_PreserveObject)
#define R_ProcessEvents rhost::rapi::_RAPI_PTR(R_ProcessEvents)
#define R_registerRoutines rhost::rapi::_RAPI_PTR(R_registerRoutines)
#define R_ReleaseObject rhost::rapi::_RAPI_PTR(R_ReleaseObject)
#define R_RestoreGlobalEnvFromFile rhost::rapi::_RAPI_PTR(R_RestoreGlobalEnvFromFile)
#define R_SaveGlobalEnvToFile rhost::rapi::_RAPI_PTR(R_SaveGlobalEnvToFile)
#define R_set_command_line_arguments rhost::rapi::_RAPI_PTR(R_set_command_line_arguments)
#define R_SetParams rhost::rapi::_RAPI_PTR(R_SetParams)
#define R_setStartTime rhost::rapi::_RAPI_PTR(R_setStartTime)
#define R_Srcref (*rhost::rapi::_RAPI_PTR(R_Srcref))
#define R_ToplevelExec rhost::rapi::_RAPI_PTR(R_ToplevelExec)
#define R_UnboundValue (*rhost::rapi::_RAPI_PTR(R_UnboundValue))
#define R_WaitEvent rhost::rapi::_RAPI_PTR(R_WaitEvent)
#define RAW rhost::rapi::_RAPI_PTR(RAW)
#define RDEBUG rhost::rapi::_RAPI_PTR(RDEBUG)
#define readconsolecfg rhost::rapi::_RAPI_PTR(readconsolecfg)
#define REAL rhost::rapi::_RAPI_PTR(REAL)
#define Rf_allocList rhost::rapi::_RAPI_PTR(Rf_allocList)
#define Rf_allocVector rhost::rapi::_RAPI_PTR(Rf_allocVector)
#define Rf_allocVector3 rhost::rapi::_RAPI_PTR(Rf_allocVector3)
#define Rf_asChar rhost::rapi::_RAPI_PTR(Rf_asChar)
#define Rf_asInteger rhost::rapi::_RAPI_PTR(Rf_asInteger)
#define Rf_asLogical rhost::rapi::_RAPI_PTR(Rf_asLogical)
#define Rf_asReal rhost::rapi::_RAPI_PTR(Rf_asReal)
#define Rf_classgets rhost::rapi::_RAPI_PTR(Rf_classgets)
#define Rf_curDevice rhost::rapi::_RAPI_PTR(Rf_curDevice)
#define Rf_deparse1line rhost::rapi::_RAPI_PTR(Rf_deparse1line)
#define Rf_desc2GEDesc rhost::rapi::_RAPI_PTR(Rf_desc2GEDesc)
#define Rf_duplicate rhost::rapi::_RAPI_PTR(Rf_duplicate)
#define Rf_error rhost::rapi::_RAPI_PTR(Rf_error)
#define Rf_eval rhost::rapi::_RAPI_PTR(Rf_eval)
#define Rf_findVar rhost::rapi::_RAPI_PTR(Rf_findVar)
#define Rf_getAttrib rhost::rapi::_RAPI_PTR(Rf_getAttrib)
#define Rf_install rhost::rapi::_RAPI_PTR(Rf_install)
#define Rf_installChar rhost::rapi::_RAPI_PTR(Rf_installChar)
#define Rf_isEnvironment rhost::rapi::_RAPI_PTR(Rf_isEnvironment)
#define Rf_isNull rhost::rapi::_RAPI_PTR(Rf_isNull)
#define Rf_isString rhost::rapi::_RAPI_PTR(Rf_isString)
#define Rf_length rhost::rapi::_RAPI_PTR(Rf_length)
#define Rf_mkChar rhost::rapi::_RAPI_PTR(Rf_mkChar)
#define Rf_mkCharCE rhost::rapi::_RAPI_PTR(Rf_mkCharCE)
#define Rf_mkString rhost::rapi::_RAPI_PTR(Rf_mkString)
#define Rf_ndevNumber rhost::rapi::_RAPI_PTR(Rf_ndevNumber)
#define Rf_NewEnvironment rhost::rapi::_RAPI_PTR(Rf_NewEnvironment)
#define Rf_onintr rhost::rapi::_RAPI_PTR(Rf_onintr)
#define Rf_protect rhost::rapi::_RAPI_PTR(Rf_protect)
#define Rf_ScalarInteger rhost::rapi::_RAPI_PTR(Rf_ScalarInteger)
#define Rf_ScalarLogical rhost::rapi::_RAPI_PTR(Rf_ScalarLogical)
#define Rf_ScalarReal rhost::rapi::_RAPI_PTR(Rf_ScalarReal)
#define Rf_ScalarString rhost::rapi::_RAPI_PTR(Rf_ScalarString)
#define Rf_selectDevice rhost::rapi::_RAPI_PTR(Rf_selectDevice)
#define Rf_translateCharUTF8 rhost::rapi::_RAPI_PTR(Rf_translateCharUTF8)
#define Rf_unprotect rhost::rapi::_RAPI_PTR(Rf_unprotect)
#define Rf_utf8towcs rhost::rapi::_RAPI_PTR(Rf_utf8towcs)
#define Rf_wtransChar rhost::rapi::_RAPI_PTR(Rf_wtransChar)
#define run_Rmainloop rhost::rapi::_RAPI_PTR(run_Rmainloop)
#define SET_RDEBUG rhost::rapi::_RAPI_PTR(SET_RDEBUG)
#define SET_STRING_ELT rhost::rapi::_RAPI_PTR(SET_STRING_ELT)
#define SET_TYPEOF rhost::rapi::_RAPI_PTR(SET_TYPEOF)
#define SET_VECTOR_ELT rhost::rapi::_RAPI_PTR(SET_VECTOR_ELT)
#define SETCAR rhost::rapi::_RAPI_PTR(SETCAR)
#define setup_Rmainloop rhost::rapi::_RAPI_PTR(setup_Rmainloop)
#define STRING_ELT rhost::rapi::_RAPI_PTR(STRING_ELT)
#define TYPEOF rhost::rapi::_RAPI_PTR(TYPEOF)
#define VECTOR_ELT rhost::rapi::_RAPI_PTR(VECTOR_ELT)
#define vmaxget rhost::rapi::_RAPI_PTR(vmaxget)
#define vmaxset rhost::rapi::_RAPI_PTR(vmaxset)

#define GA_initapp rhost::rapi::_RAPI_PTR(GA_initapp)
#endif
