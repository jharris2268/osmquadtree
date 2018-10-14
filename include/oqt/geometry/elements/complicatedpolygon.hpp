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
        ComplicatedPolygon(
            std::shared_ptr<Relation> rel, int64 part_,
            const Ring& outers_, const std::vector<Ring>& inners_,
            const std::vector<Tag>& tags, int64 zorder_,
            int64 layer_, int64 minzoom_);
        
        
        ComplicatedPolygon(
            int64 id, int64 qt, const ElementInfo& inf,
            const std::vector<Tag>& tags, int64 part_,
            const Ring& outers_, const std::vector<Ring>& inners_,
            int64 zorder_, int64 layer_, double area_,
            const bbox& bounds_, int64 minzoom_);

        uint64 InternalId() const;
        virtual ~ComplicatedPolygon() {}

        virtual ElementType OriginalType() const;
        const Ring& OuterRing() const;
        const std::vector<Ring>& InnerRings() const;
        int64 ZOrder() const;
        int64 Layer() const;
        double Area() const;
        int64 Part() const;
        virtual ElementPtr copy();
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const;

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        
        
        
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