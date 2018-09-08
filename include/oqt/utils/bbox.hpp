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

#ifndef UTILS_BBOX_HPP
#define UTILS_BBOX_HPP

#include "oqt/common.hpp"
#include <sstream>
namespace oqt {
    struct bbox {
    int64 minx,miny,maxx,maxy;

    bbox() : minx(1800000000),miny(1800000000),maxx(-1800000000),maxy(-1800000000) {}
    bbox(int64 a, int64 b, int64 c, int64 d) : minx(a),miny(b),maxx(c),maxy(d) {}

    void expand_point(int64 x, int64 y) {
        if (x<minx) { minx=x; }
        if (y<miny) { miny=y; }
        if (x>maxx) { maxx=x; }
        if (y>maxy) { maxy=y; }
    }
};
bool box_empty(const bbox&);
bool box_planet(const bbox&);

std::ostream& operator<<(std::ostream&, const bbox&);
bool contains_point(const bbox&, int64, int64);
bool overlaps(const bbox&, const bbox&);
bool overlaps_quadtree(const bbox&, int64);
bool bbox_contains(const bbox&, const bbox&);

void bbox_expand(bbox&, const bbox&);

}

#endif
