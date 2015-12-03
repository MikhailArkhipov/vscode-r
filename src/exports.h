#pragma once

#include "Rapi.h"

namespace rhost {
    namespace exports {
        void add_external_methods(R_ExternalMethodDef *call_method);
        void add_call_methods(R_CallMethodDef *call_method);
        void add_c_methods(R_CMethodDef *c_method);
        void register_all(DllInfo *dll);
    }
}
