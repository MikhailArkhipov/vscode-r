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
#include "exports.h"

namespace rhost {
    namespace exports {
        static std::vector<R_ExternalMethodDef> external_methods;
        static std::vector<R_CallMethodDef> call_methods;
        static std::vector<R_CMethodDef> c_methods;

        void add_external_methods(R_ExternalMethodDef *methods) {
            for (int i = 0; methods[i].name != nullptr; i++) {
                external_methods.push_back(methods[i]);
            }
        }

        void add_call_methods(R_CallMethodDef *methods) {
            for (int i = 0; methods[i].name != nullptr; i++) {
                call_methods.push_back(methods[i]);
            }
        }

        void add_c_methods(R_CMethodDef *methods) {
            for (int i = 0; methods[i].name != nullptr; i++) {
                c_methods.push_back(methods[i]);
            }
        }

        void register_all(DllInfo *dll) {
            external_methods.push_back({});
            call_methods.push_back({});
            c_methods.push_back({});
            R_registerRoutines(dll, c_methods.data(), call_methods.data(), nullptr, external_methods.data());
        }
    }
}
