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
#include "util.h"

namespace rhost {
    namespace json {
        // Produces JSON from an R object according to the following mappings:
        //
        // NULL, NA, empty vector -> null
        // TRUE -> true
        // FALSE -> false
        // Vector of a single non-NA integer or double -> numeric literal
        // Vector of a single non-NA string -> string literal
        // List with all elements unnamed -> array (recursively)
        // List with all elements named, or environment -> object (recursively)
        //
        // If any element of a list or environment is NA, that element is skipped.
        // Any input not covered by the rules above is considered invalid.
        //
        // Returns true if serialized value was NA, and false otherwise (even if it contains NA somewhere inside).
        // Errors are reported via Rf_error.
        bool to_json(SEXP sexp, picojson::value& result);

        // Same as above, but value is returned directly instead of being constructed in the provided location,
        // and errors are reported as C++ exceptions.
        inline picojson::value to_json(SEXP sexp) {
            picojson::value result;
            rhost::util::errors_to_exceptions([&] { to_json(sexp, result); });
            return result;
        }
    }
}
