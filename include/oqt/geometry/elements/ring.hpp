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

#ifndef GEOMETRY_ELEMENTS_RING_HPP
#define GEOMETRY_ELEMENTS_RING_HPP

#include "oqt/geometry/utils.hpp"

namespace oqt {
namespace geometry {


struct Ring {
    struct Part {
        int64 orig_id;
        refvector refs;
        lonlatvec lonlats;
        bool reversed;
    };
    
    std::vector<Part> parts;
};


double calc_ring_area(const Ring& ring);

lonlatvec ringpart_lonlats(const Ring& ring);
refvector ringpart_refs(const Ring& ring);

void reverse_ring(Ring& ring);


std::string pack_ring(const Ring& rps);
size_t ringpart_numpoints(const Ring& rpv);
size_t write_ringpart_ring(std::string& data, size_t pos, const Ring& rpv, size_t r, bool transform);


}
}
#endif
