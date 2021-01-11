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

#include "oqt/geometry/elements/point.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {
    

Point::Point(std::shared_ptr<Node> nd) :
    BaseGeometry(ElementType::Point,changetype::Normal,nd->Id(),nd->Quadtree(),nd->Info(),nd->Tags(),-1),
    lon(nd->Lon()), lat(nd->Lat()) {}
Point::Point(std::shared_ptr<Node> nd, const std::vector<Tag>& tgs, std::optional<int64> layer_, std::optional<int64> minzoom_) :
    BaseGeometry(ElementType::Point,changetype::Normal,nd->Id(),nd->Quadtree(),nd->Info(),tgs,minzoom_),
    lon(nd->Lon()), lat(nd->Lat()),layer(layer_) {}

Point::Point(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, int64 lon_, int64 lat_, std::optional<int64> layer_, std::optional<int64> minzoom_) :
    BaseGeometry(ElementType::Point,changetype::Normal,id,qt,inf,tags,minzoom_), lon(lon_), lat(lat_),layer(layer_){}

ElementType Point::OriginalType() const { return ElementType::Node; }

oqt::LonLat Point::LonLat() const { return oqt::LonLat{lon,lat}; };
std::optional<int64> Point::Layer() const { return layer; }

ElementPtr Point::copy() {
    return std::make_shared<Point>(//*this);
        Id(),Quadtree(),Info(),Tags(),lon,lat,layer,MinZoom());
}

bbox Point::Bounds() const { return bbox{lon,lat,lon,lat}; }

    
    
    
std::string Point::Wkb(bool transform, bool srid) const {
    std::string res(srid ? 25 : 21,'\0');
    res[4]='\1';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,epsg_code(transform));
    }
    write_point(res, pos, LonLat(), transform);
    return res;
}

std::list<PbfTag> Point::pack_extras() const {
    
    std::list<PbfTag> extras{PbfTag{8,zig_zag(lat),""}, PbfTag{9,zig_zag(lon),""}};    
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
