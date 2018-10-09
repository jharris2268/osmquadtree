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

#ifndef GEOMETRY_ELEMENTS_LINESTRING_HPP
#define GEOMETRY_ELEMENTS_LINESTRING_HPP

#include "oqt/elements/geometry.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"


#include <set>

namespace oqt {
namespace geometry {
    


class Linestring : public BaseGeometry {
    public:
        Linestring(std::shared_ptr<WayWithNodes> wy) :
            BaseGeometry(ElementType::Linestring, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags(),-1),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(0), layer(0), bounds(wy->Bounds()) {
                length=calc_line_length(lonlats);
            }

        Linestring(std::shared_ptr<WayWithNodes> wy, const std::vector<Tag>& tgs, int64 zorder_, int64 layer_, int64 minzoom_) :
            BaseGeometry(ElementType::Linestring, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), tgs,minzoom_),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(zorder_), layer(layer_), bounds(wy->Bounds()){
                length=calc_line_length(lonlats);
            }
        Linestring(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, const refvector& refs_, const lonlatvec& lonlats_, int64 zorder_, int64 layer_, double length_, const bbox& bounds_, int64 minzoom_) :
            BaseGeometry(ElementType::Linestring,changetype::Normal,id,qt,inf,tags,minzoom_), refs(refs_), lonlats(lonlats_), zorder(zorder_), layer(layer_), length(length_), bounds(bounds_) {}


        virtual ~Linestring() {}


        virtual ElementType OriginalType() const { return ElementType::Way; }
        const refvector& Refs() const { return refs; }
        const lonlatvec& LonLats() const { return lonlats; }
        double Length() const { return length; }
        int64 ZOrder() const { return zorder; }
        int64 Layer() const { return layer; }
        virtual ElementPtr copy() { return std::make_shared<Linestring>(//*this); }
            Id(),Quadtree(),Info(),Tags(),refs,lonlats,zorder,layer,length,bounds,MinZoom()); }
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bounds; }

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        //linestring(const linestring& l) : element(l), refs(l.refs), lonlats(l.lonlats), zorder(l.zorder), length(l.length), bounds(l.bounds) {}
        refvector refs;
        lonlatvec lonlats;
        int64 zorder;
        int64 layer;
        double length;
        bbox bounds;
        
};
}
}
#endif
