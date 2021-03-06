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

#ifndef GEOMETRY_ADDPARENTTAGS_HPP
#define GEOMETRY_ADDPARENTTAGS_HPP

#include "oqt/geometry/addwaynodes.hpp"
#include "oqt/geometry/utils.hpp"
#include <map>

namespace oqt {
namespace geometry {

struct ParentTagSpec {
    std::string node_tag;
    std::string out_tag;
    std::string way_tag;
    std::map<std::string,int> priority;
    ParentTagSpec(const std::string& n, const std::string& o, const std::string& w, const std::map<std::string,int>& p)
        : node_tag(n),out_tag(o),way_tag(w),priority(p) {}
};


std::shared_ptr<BlockHandler> make_addparenttags(const std::vector<ParentTagSpec>& spec);


}}
#endif //ADDPARENTTAGS_HPP
