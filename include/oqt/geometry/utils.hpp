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

#ifndef GEOMETRY_UTILS_HPP
#define GEOMETRY_UTILS_HPP

#include "oqt/geometry/addwaynodes.hpp"

#include "oqt/common.hpp"


#include "oqt/elements/geometry.hpp"
#include <set>

namespace oqt {
namespace geometry {

uint32_t epsg_code(bool transform);



struct XY {
    XY() : x(0), y(0) {}
    XY(double x_, double y_) : x(x_), y(y_) {}
    double x, y;
};

XY forward_transform(int64 ln, int64 lt);
LonLat inverse_transform(double x, double y);


double calc_line_length(const std::vector<LonLat>& ll);
double calc_ring_area(const std::vector<LonLat>& ll);
bbox lonlats_bounds(const std::vector<LonLat>& llv);



size_t write_point(std::string& data, size_t pos, LonLat ll, bool transform);
size_t write_ring(std::string& data, size_t pos, const std::vector<LonLat>& lonlats, bool transform);
int64 to_int(double v);

std::string pack_bounds(const bbox& bounds);

typedef PrimitiveBlockPtr primblock_ptr;
typedef std::vector<primblock_ptr> primblock_vec;

class BlockHandler  {
    public:
        virtual primblock_vec process(primblock_ptr)=0;
        virtual primblock_vec finish() {return {}; };
        virtual ~BlockHandler() {};
};

std::string get_tag(ElementPtr, const std::string&);


ElementPtr read_geometry(ElementType ty, const std::string& data, const std::vector<std::string>& stringtable, changetype ct);
ElementPtr unpack_geometry(ElementType ty, int64 id, changetype ct, int64 qt, const std::string& d);

ElementPtr unpack_geometry_element(ElementPtr geom);
size_t unpack_geometry_primitiveblock(primblock_ptr pb);


std::string pack_tags(const std::vector<Tag>& tgs);
void unpack_tags(const std::string& str, std::vector<Tag>& tgs);
std::string convert_packed_tags_to_json(const std::string& str);
} }

#endif
