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
        Linestring(std::shared_ptr<WayWithNodes> wy);

        Linestring(std::shared_ptr<WayWithNodes> wy,
            const std::vector<Tag>& tgs, int64 zorder_,
            int64 layer_, int64 minzoom_);
            
        Linestring(int64 id, int64 qt, const ElementInfo& inf,
            const std::vector<Tag>& tags,
            const std::vector<int64>& refs_, const std::vector<LonLat>& lonlats_,
            int64 zorder_, int64 layer_, double length_,
            const bbox& bounds_, int64 minzoom_);

        virtual ~Linestring() {}


        virtual ElementType OriginalType() const;
        const std::vector<int64>& Refs() const;
        const std::vector<LonLat>& LonLats() const;
        double Length() const;
        int64 ZOrder() const;
        int64 Layer() const;
        virtual ElementPtr copy();
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const;

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        
        std::vector<int64> refs;
        std::vector<LonLat> lonlats;
        int64 zorder;
        int64 layer;
        double length;
        bbox bounds;
        
};
}
}
#endif
