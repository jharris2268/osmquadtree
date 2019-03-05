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

#include "oqt/elements/node.hpp"
namespace oqt {

Node::Node(changetype c, int64 i, int64 q, ElementInfo inf, std::vector<Tag> tags, int64 lon, int64 lat)
    : ElementImpl(node_data{ {ElementType::Node,c,i,q,inf,tags},lon,lat}) {}


int64 Node::Lon() const { return data_.lon; }
int64 Node::Lat() const { return data_.lat; }
/*
std::list<PbfTag> node::pack_extras() const {
    return {PbfTag{8,zigZag(lat_),""},PbfTag{9,zigZag(lon_),""}};
}*/
ElementPtr Node::copy() { return std::make_shared<Node>(ChangeType(),Id(),Quadtree(),Info(),Tags(),Lon(),Lat()); }
    
}

