/*****************************************************************************
 *
 * This file is part of osmquadtree
 *
 * Copyright (C) 2018 James Harris
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#include "oqt_python.hpp"

#ifdef INDIVIDUAL_MODULES
//pass
#else


PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);


PYBIND11_MODULE(_oqt, m) {
    //py::module m("_oqt", "pybind11 example plugin");

    calcqts_defs(m);
    count_defs(m);
    elements_defs(m);
    geometry_defs(m);
    pbfformat_defs(m);
    postgis_defs(m);
    sorting_defs(m);
    update_defs(m);
    utils_defs(m);

    return m.ptr();
};

#endif
