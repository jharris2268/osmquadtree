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

#include "oqt/utils/bbox.hpp"

namespace oqt {


namespace quadtree {

    
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
bool overlaps_quadtree(const bbox&, int64);
}
#endif

