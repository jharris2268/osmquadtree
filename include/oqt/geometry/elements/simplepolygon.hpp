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

#ifndef GEOMETRY_ELEMENTS_SIMPLEPOLYGON_HPP
#define GEOMETRY_ELEMENTS_SIMPLEPOLYGON_HPP

#include "oqt/geometry/utils.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"
#include "oqt/elements/geometry.hpp"
#include <set>

namespace oqt {
namespace geometry {
    

class SimplePolygon : public BaseGeometry {
    public:
        SimplePolygon(std::shared_ptr<WayWithNodes> wy) :
            BaseGeometry(ElementType::SimplePolygon, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags(),-1),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(0), layer(0), bounds(wy->Bounds()), reversed(false){
                area = calc_ring_area(lonlats);
                if (area<0) {
                    area *= -1;
                    reversed=true;
                }
            }

        SimplePolygon(std::shared_ptr<WayWithNodes> wy, const std::vector<Tag>& tgs, int64 zorder_, int64 layer_, int64 minzoom_) :
            BaseGeometry(ElementType::SimplePolygon, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), tgs,minzoom_),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(zorder_), layer(layer_), bounds(wy->Bounds()), reversed(false) {
                area = calc_ring_area(lonlats);
                if (area<0) {
                    area *= -1;
                    reversed=true;
                }
            }

        SimplePolygon(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, const refvector& refs_, const lonlatvec& lonlats_, int64 zorder_, int64 layer_, double area_, const bbox& bounds_, int64 minzoom_, bool reversed_) :
            BaseGeometry(ElementType::SimplePolygon,changetype::Normal,id,qt,inf,tags,minzoom_),
            refs(refs_), lonlats(lonlats_), zorder(zorder_), layer(layer_), area(area_), bounds(bounds_),reversed(reversed_) {}


        virtual ~SimplePolygon() {}


        virtual ElementType OriginalType() const { return ElementType::Way; }
        const refvector& Refs() const { return refs; }
        const lonlatvec& LonLats() const { return lonlats; }
        bool Reversed() const { return reversed; }
        
        int64 ZOrder() const { return zorder; }
        int64 Layer() const { return layer; }
        double Area() const { return area; }

        virtual ElementPtr copy() { return std::make_shared<SimplePolygon>(//*this); }
            Id(),Quadtree(),Info(),Tags(),refs,lonlats,zorder,layer,area,bounds,MinZoom(),reversed); }
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bounds; }

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        //simplepolygon(const simplepolygon& l) : element(l), refs(l.refs), lonlats(l.lonlats), zorder(l.zorder), area(l.area), bounds(l.bounds) {}
        refvector refs;
        lonlatvec lonlats;
        int64 zorder;
        int64 layer;
        double area;
        bbox bounds;
        bool reversed;
        
};
}
}
#endif
