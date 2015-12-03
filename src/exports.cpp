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
