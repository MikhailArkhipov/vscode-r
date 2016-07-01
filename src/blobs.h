#pragma once
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
#include "Rapi.h"

namespace rhost {
    namespace raw {
        // Produces BLOB(raw bytes)-JSON from an R object according to the following mappings:
        //
        // NULL, NA, empty vector -> null
        // RAW -> bytes
        //
        // Returns true if serialized value was NA, and false otherwise (even if it contains NA somewhere inside).
        // Errors are reported via Rf_error.
        bool to_blobs(SEXP sexp, rhost::util::blobs& blobs, picojson::value& result);
    }
}