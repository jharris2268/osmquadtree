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

#ifndef GEOMETRY_ELEMENTS_COMPLICATEDPOLYGON_HPP
#define GEOMETRY_ELEMENTS_COMPLICATEDPOLYGON_HPP

#include "oqt/geometry/elements/ring.hpp"

#include "oqt/elements/geometry.hpp"
#include <set>

namespace oqt {
namespace geometry {
    

class ComplicatedPolygon : public BaseGeometry {
    public:
        ComplicatedPolygon(std::shared_ptr<Relation> rel, int64 part_, const Ring& outers_, const std::vector<Ring>& inners_, const std::vector<Tag>& tags, int64 zorder_, int64 layer_, int64 minzoom_) :
            BaseGeometry(ElementType::ComplicatedPolygon, changetype::Normal, rel->Id(), rel->Quadtree(), rel->Info(), tags,minzoom_),
            part(part_), outers(outers_), inners(inners_),zorder(zorder_), layer(layer_) {

            area = calc_ring_area(outers);
            if (area<0) {
                reverse_ring(outers);
                area *= -1;
            }
                
            for (auto& ii : inners) {
                double a = calc_ring_area(ii);
                if (a>0) {
                    reverse_ring(ii);
                    a *= -1;
                }
                area += a;
            }
            if (area < 0) {
                area = 0;
            }
            for (const auto& o : outers.parts) {
                for (const auto& ll : o.lonlats) {
                    bounds.expand_point(ll.lon,ll.lat);
                }
            }

        }
        ComplicatedPolygon(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, int64 part_, const Ring& outers_, const std::vector<Ring>& inners_, int64 zorder_, int64 layer_, double area_, const bbox& bounds_, int64 minzoom_)
            : BaseGeometry(ElementType::ComplicatedPolygon,changetype::Normal,id,qt,inf,tags,minzoom_), part(part_), outers(outers_), inners(inners_),zorder(zorder_), layer(layer_), area(area_), bounds(bounds_){}


        uint64 InternalId() const {
            return (6ull<<61) | (((uint64) Id()) << 16) | ((uint64) part);
        }   
        virtual ~ComplicatedPolygon() {}

        virtual ElementType OriginalType() const { return ElementType::Relation; }
        const Ring& OuterRing() const { return outers; }
        const std::vector<Ring>& InnerRings() const { return inners; }
        int64 ZOrder() const { return zorder; }
        int64 Layer() const { return layer; }
        double Area() const { return area; }
        int64 Part() const { return part; }
        virtual ElementPtr copy() { return std::make_shared<ComplicatedPolygon>(
            Id(),Quadtree(),Info(),Tags(),part,outers,inners,zorder,layer,area,bounds,MinZoom()); }
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bounds; }

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        //complicatedpolygon(const complicatedpolygon& p) :
        //    element(p), part(p.part), outers(p.outers), inners(p.inners), area(p.area),bounds(p.bounds) {}
        int64 part;
        Ring outers;
        std::vector<Ring> inners;
        int64 zorder;
        int64 layer;
        double area;
        bbox bounds;
        
};
}
}
#endif
