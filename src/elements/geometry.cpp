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
geometry_packed::geometry_packed(elementtype t, changetype c, int64 i, int64 q, info inf, std::vector<tag> tags, int64 minzoom_, std::list<PbfTag> geom_messages_) :
    basegeometry(t,c,i,q,inf,tags,minzoom_), geom_messages(geom_messages_), internalid(0) {
    
    internalid = (((uint64) t)<<61ull);
    if (t==6) {
        internalid |= ( ((uint64) i) << 16ull);
        for (const auto& m: geom_messages) {
            if (m.tag==19) { internalid |= m.value/2; } //part (zigzag int64)
        }
    } else {
        internalid |= i;
    }
        
}
uint64 geometry_packed::InternalId() const { return internalid; }


elementtype geometry_packed::OriginalType() const { return Unknown; }
bbox geometry_packed::Bounds() const { return bbox{1,1,0,0}; }
std::string geometry_packed::Wkb(bool transform, bool srid) const { throw std::domain_error("not implemented"); }

std::list<PbfTag> geometry_packed::pack_extras() const { return geom_messages; }

element_ptr geometry_packed::copy() {
    return std::make_shared<geometry_packed>(Type(),ChangeType(),Id(),Quadtree(),Info(),Tags(),MinZoom(),geom_messages);
}
bool isGeometryType(elementtype ty) {
    switch (ty) {
        case Node: return false;
        case Way: return false;
        case Relation: return false;
        case Point: return true;
        case Linestring: return true;
        case SimplePolygon: return true;
        case ComplicatedPolygon: return true;
        case WayWithNodes: return false;
        case Unknown: return false;
    }
    return false;
}
    
}


