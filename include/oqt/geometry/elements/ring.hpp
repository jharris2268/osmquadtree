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
        std::vector<int64> refs;
        std::vector<LonLat> lonlats;
        bool reversed;
    };
    
    std::vector<Part> parts;
};


double calc_ring_area(const Ring& ring);

std::vector<LonLat> ringpart_lonlats(const Ring& ring);
std::vector<int64> ringpart_refs(const Ring& ring);

void reverse_ring(Ring& ring);


std::string pack_ring(const Ring& rps);
size_t ringpart_numpoints(const Ring& rpv);
size_t write_ringpart_ring(std::string& data, size_t pos, const Ring& rpv, size_t r, bool transform);

struct PolygonPart {
    PolygonPart(int64 index_, const Ring& outer_, const std::vector<Ring>& inners_, double area_) :
        index(index_), outer(outer_), inners(inners_), area(area_) {}
    PolygonPart() : index(0), area(0) {}
        
    int64 index;
    Ring outer;
    std::vector<Ring> inners;
    double area;
};

std::string pack_ring(const Ring& ring);
Ring unpack_ring(const std::string& data);

std::string pack_polygon_part(const PolygonPart& poly);
PolygonPart unpack_polygon_part(const std::string& data);



}
}
#endif
