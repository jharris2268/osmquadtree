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

#include "oqt/geometry/elements/linestring.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {
Linestring::Linestring(std::shared_ptr<WayWithNodes> wy) :
    BaseGeometry(ElementType::Linestring, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags(),-1),
    refs(wy->Refs()), lonlats(wy->LonLats()), zorder(0), layer(0), bounds(wy->Bounds()) {
        length=calc_line_length(lonlats);
    }

Linestring::Linestring(std::shared_ptr<WayWithNodes> wy, const std::vector<Tag>& tgs, std::optional<int64> zorder_, std::optional<int64> layer_, std::optional<int64> minzoom_) :
    BaseGeometry(ElementType::Linestring, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), tgs,minzoom_),
    refs(wy->Refs()), lonlats(wy->LonLats()), zorder(zorder_), layer(layer_), bounds(wy->Bounds()){
        length=calc_line_length(lonlats);
    }
Linestring::Linestring(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, const std::vector<int64>& refs_, const std::vector<LonLat>& lonlats_, std::optional<int64> zorder_, std::optional<int64> layer_, double length_, const bbox& bounds_, std::optional<int64> minzoom_) :
    BaseGeometry(ElementType::Linestring,changetype::Normal,id,qt,inf,tags,minzoom_), refs(refs_), lonlats(lonlats_), zorder(zorder_), layer(layer_), length(length_), bounds(bounds_) {}



ElementType Linestring::OriginalType() const { return ElementType::Way; }
const std::vector<int64>& Linestring::Refs() const { return refs; }
const std::vector<LonLat>& Linestring::LonLats() const { return lonlats; }
double Linestring::Length() const { return length; }
std::optional<int64> Linestring::ZOrder() const { return zorder; }
std::optional<int64> Linestring::Layer() const { return layer; }
ElementPtr Linestring::copy() { return std::make_shared<Linestring>(//*this); }
    Id(),Quadtree(),Info(),Tags(),refs,lonlats,zorder,layer,length,bounds,MinZoom()); }

bbox Linestring::Bounds() const { return bounds; }

       
std::string Linestring::Wkb(bool transform, bool srid) const {

    std::string res((srid?13:9)+16*lonlats.size(),'\0');
    //res[0]='\1';
    res[4]='\2';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,epsg_code(transform));
    }
    write_ring(res,pos,lonlats, transform);
    return res;
}

std::list<PbfTag> Linestring::pack_extras() const {
    std::list<PbfTag> extras;
    
    extras.push_back(PbfTag{8,0,write_packed_delta(refs)}); //refs
    extras.push_back(PbfTag{12,zig_zag(zorder.value_or(0)),""});
    extras.push_back(PbfTag{13,0,write_packed_delta_func<LonLat>(lonlats,[](const LonLat& l)->int64 { return l.lon; })}); //lons
    extras.push_back(PbfTag{14,0,write_packed_delta_func<LonLat>(lonlats,[](const LonLat& l)->int64 { return l.lat; })}); //lats
    extras.push_back(PbfTag{15,zig_zag(to_int(length*100)),""});
    if (MinZoom()) {
        extras.push_back(PbfTag{22,uint64(*MinZoom()),""});
    }
    if (layer) {
        extras.push_back(PbfTag{24,zig_zag(*layer),""});
    }
    return extras;
}

}
}
