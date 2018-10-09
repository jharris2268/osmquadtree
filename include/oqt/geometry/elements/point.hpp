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

#ifndef GEOMETRY_ELEMENTS_POINT_HPP
#define GEOMETRY_ELEMENTS_POINT_HPP

#include "oqt/geometry/utils.hpp"
#include "oqt/elements/geometry.hpp"
#include <set>

namespace oqt {
namespace geometry {



class Point : public BaseGeometry {
    public:
        Point(std::shared_ptr<Node> nd) :
            BaseGeometry(ElementType::Point,changetype::Normal,nd->Id(),nd->Quadtree(),nd->Info(),nd->Tags(),-1),
            lon(nd->Lon()), lat(nd->Lat()) {}
        Point(std::shared_ptr<Node> nd, const std::vector<Tag>& tgs, int64 layer_, int64 minzoom_) :
            BaseGeometry(ElementType::Point,changetype::Normal,nd->Id(),nd->Quadtree(),nd->Info(),tgs,minzoom_),
            lon(nd->Lon()), lat(nd->Lat()),layer(layer_) {}

        Point(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, int64 lon_, int64 lat_, int64 layer_, int64 minzoom_) :
            BaseGeometry(ElementType::Point,changetype::Normal,id,qt,inf,tags,minzoom_), lon(lon_), lat(lat_),layer(layer_){}

        virtual ~Point() {}

        virtual ElementType OriginalType() const { return ElementType::Node; }

        lonlat LonLat() const { return lonlat{lon,lat}; };
        int64 Layer() const { return layer; }

        virtual ElementPtr copy() {
            return std::make_shared<Point>(//*this);
                Id(),Quadtree(),Info(),Tags(),lon,lat,layer,MinZoom());
        }

        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bbox{lon,lat,lon,lat}; }

        virtual std::string Wkb(bool transform, bool srid) const;

        

    private:
       //point(const point& p) : element(p), lon(p.lon), lat(p.lat) {}
        int64 lon, lat;
        int64 layer;
        
};    

}
}
#endif
