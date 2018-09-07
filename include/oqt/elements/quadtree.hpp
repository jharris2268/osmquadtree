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
 
#ifndef ELEMENTS_QUADTREE_HPP
#define ELEMENTS_QUADTREE_HPP


#include <tuple>
#include <cmath>
#include "oqt/common.hpp"
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

namespace quadtree {

    /*struct xyz {
        int64 x,y,z;
    };*/
    typedef std::tuple<int64,int64,int64> xyz;

    int64 calculate(int64 min_x, int64 min_y, int64 max_x, int64 max_y, double buffer, uint64 max_depth);

    int64 round(int64 quadtree, uint64 new_depth);
    int64 common(int64 lhs, int64 rhs);

    std::string string(int64 quadtree);
    bbox bbox(int64 quadtree, double buffer);
    xyz tuple(int64 quadtree);

    int64 from_string(const std::string& str);
    int64 from_tuple(int64 x, int64 y, int64 z);
};

inline int64 toInt(double v) {
    if (v>0) {
        return (v*10000000)+0.5;
    }
    return (v*10000000)-0.5;
}

inline double toFloat(int64 v) {
    return ((double)v) * 0.0000001;
}
inline double merc(double y, double scale=90.0) {
    return log(tan(M_PI*(1.0+y/90.0)/4.0)) * scale / M_PI;
}

inline double unMerc(double d, double scale=90.0) {
    return (atan(exp(d*M_PI/scale))*4/M_PI - 1.0) * 90.0;
}

static const double earth_width = 20037508.342789244;
}
#endif

