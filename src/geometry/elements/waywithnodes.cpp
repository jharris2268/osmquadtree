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

#include "oqt/geometry/elements/waywithnodes.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {

WayWithNodes::WayWithNodes(std::shared_ptr<Way> wy, const std::vector<LonLat>& lonlats_)
    : Element(ElementType::WayWithNodes, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags()), refs(wy->Refs()), lonlats(lonlats_) {
    for (auto& l : lonlats) {
        expand_point(bounds, l.lon,l.lat);
    }
}
WayWithNodes:: WayWithNodes(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tgs, const std::vector<int64>& refs_, const std::vector<LonLat>& lonlats_, const bbox& bounds_)
    : Element(ElementType::WayWithNodes,changetype::Normal, id,qt,inf,tgs), refs(refs_), lonlats(lonlats_), bounds(bounds_) {}


const std::vector<int64>& WayWithNodes::Refs() const { return refs; }

const std::vector<LonLat>& WayWithNodes::LonLats() const { return lonlats; }
const bbox& WayWithNodes::Bounds() const { return bounds; }
bool WayWithNodes::IsRing() const {
    return (Refs().size()>3) && (Refs().front()==Refs().back());
}


ElementPtr WayWithNodes::copy() { return std::make_shared<WayWithNodes>(Id(),Quadtree(),Info(),Tags(),refs,lonlats,bounds); }

std::list<PbfTag> WayWithNodes::pack_extras() const {
    

    std::list<PbfTag> extras;
    extras.push_back(PbfTag{8,0,write_packed_delta(refs)}); //refs
    extras.push_back(PbfTag{12,0,write_packed_delta_func<LonLat>(lonlats,[](const LonLat& l)->int64 { return l.lon; })}); //lons
    extras.push_back(PbfTag{13,0,write_packed_delta_func<LonLat>(lonlats,[](const LonLat& l)->int64 { return l.lat; })}); //lats
    return extras;
}

}
}
