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
        Point(std::shared_ptr<Node> nd);
        Point(std::shared_ptr<Node> nd, const std::vector<Tag>& tgs, std::optional<int64> layer_, std::optional<int64> minzoom_);
        Point(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, int64 lon_, int64 lat_, std::optional<int64> layer_, std::optional<int64> minzoom_);
        
        virtual ~Point() {}

        virtual ElementType OriginalType() const;

        oqt::LonLat LonLat() const;
        std::optional<int64> Layer() const;

        virtual ElementPtr copy();

        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const;

        virtual std::string Wkb(bool transform, bool srid) const;

        

    private:
       
        int64 lon, lat;
        std::optional<int64> layer;
        
};    

}
}
#endif
