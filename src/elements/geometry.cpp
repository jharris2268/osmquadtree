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

#include "oqt/elements/geometry.hpp"
namespace oqt {
GeometryPacked::GeometryPacked(ElementType t, changetype c, int64 i, int64 q, ElementInfo inf, std::vector<Tag> tags, int64 minzoom_, std::list<PbfTag> geom_messages_) :
    BaseGeometry(t,c,i,q,inf,tags,minzoom_), geom_messages(geom_messages_), internalid(0) {
    
    internalid = (((uint64) t)<<61ull);
    if (t==ElementType::ComplicatedPolygon) {
        internalid |= ( ((uint64) i) << 16ull);
        for (const auto& m: geom_messages) {
            if (m.tag==19) { internalid |= m.value/2; } //part (zigzag int64)
        }
    } else {
        internalid |= i;
    }
        
}
uint64 GeometryPacked::InternalId() const { return internalid; }


ElementType GeometryPacked::OriginalType() const { return ElementType::Unknown; }
bbox GeometryPacked::Bounds() const { return bbox{1,1,0,0}; }
std::string GeometryPacked::Wkb(bool transform, bool srid) const { throw std::domain_error("not implemented"); }

std::list<PbfTag> GeometryPacked::pack_extras() const { return geom_messages; }

ElementPtr GeometryPacked::copy() {
    return std::make_shared<GeometryPacked>(Type(),ChangeType(),Id(),Quadtree(),Info(),Tags(),MinZoom(),geom_messages);
}
bool is_geometry_type(ElementType ty) {
    switch (ty) {
        case ElementType::Node: return false;
        case ElementType::Way: return false;
        case ElementType::Relation: return false;
        case ElementType::Point: return true;
        case ElementType::Linestring: return true;
        case ElementType::SimplePolygon: return true;
        case ElementType::ComplicatedPolygon: return true;
        case ElementType::WayWithNodes: return false;
        case ElementType::Unknown: return false;
    }
    return false;
}
    
}


