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
        SimplePolygon(std::shared_ptr<WayWithNodes> wy);

        SimplePolygon(std::shared_ptr<WayWithNodes> wy, const std::vector<Tag>& tgs,
            std::optional<int64> zorder_, std::optional<int64> layer_, std::optional<int64> minzoom_);

        SimplePolygon(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags,
            const std::vector<int64>& refs_, const std::vector<LonLat>& lonlats_,
            std::optional<int64> zorder_, std::optional<int64> layer_, double area_,
            const bbox& bounds_, std::optional<int64> minzoom_, bool reversed_);

        virtual ~SimplePolygon() {}


        virtual ElementType OriginalType() const;
        const std::vector<int64>& Refs() const;
        const std::vector<LonLat>& LonLats() const;
        bool Reversed() const;
        
        std::optional<int64> ZOrder() const;
        std::optional<int64> Layer() const;
        double Area() const;

        virtual ElementPtr copy();
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const;

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        
        std::vector<int64> refs;
        std::vector<LonLat> lonlats;
        std::optional<int64> zorder;
        std::optional<int64> layer;
        double area;
        bbox bounds;
        bool reversed;
        
};
}
}
#endif
