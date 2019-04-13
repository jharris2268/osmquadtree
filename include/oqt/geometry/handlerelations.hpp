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

#ifndef GEOMETRY_HANDLERELATIONS_HPP
#define GEOMETRY_HANDLERELATIONS_HPP

#include "oqt/geometry/addwaynodes.hpp"
#include "oqt/geometry/utils.hpp"
#include <set>

namespace oqt {
namespace geometry {




struct RelationTagSpec {
    enum class Type {
        Min,
        Max,
        List
    };
    
    std::string target_key;
    std::vector<Tag> source_filter;
    std::string source_key;
    Type type;
    
    RelationTagSpec(std::string target_key_, std::vector<Tag> source_filter_, std::string source_key_, Type type_) : 
        target_key(target_key_), source_filter(source_filter_), source_key(source_key_), type(type_) {}
    
    
};
    


std::shared_ptr<BlockHandler> make_handlerelations(const std::vector<RelationTagSpec>& spec);

}}



#endif //HANDLERELATIONS_HPP
