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

#ifndef GEOMETRY_ELEMENTS_WAYWITHNODES_HPP
#define GEOMETRY_ELEMENTS_WAYWITHNODES_HPP

#include "oqt/geometry/utils.hpp"

#include "oqt/elements/geometry.hpp"
#include <set>

namespace oqt {
namespace geometry {





class WayWithNodes : public Element {
    public:
        WayWithNodes(std::shared_ptr<Way> wy, const lonlatvec& lonlats_)
            : Element(ElementType::WayWithNodes, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags()), refs(wy->Refs()), lonlats(lonlats_) {
            for (auto& l : lonlats) {
                bounds.expand_point(l.lon,l.lat);
            }
        }
        WayWithNodes(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tgs, const std::vector<int64>& refs_, const lonlatvec& lonlats_, const bbox& bounds_)
            : Element(ElementType::WayWithNodes,changetype::Normal, id,qt,inf,tgs), refs(refs_), lonlats(lonlats_), bounds(bounds_) {}


        const std::vector<int64>& Refs() { return refs; }

        const lonlatvec& LonLats() { return lonlats; }
        const bbox& Bounds() { return bounds; }
        bool IsRing() {
            return (Refs().size()>3) && (Refs().front()==Refs().back());
        }

        virtual std::list<PbfTag> pack_extras() const;
        virtual ~WayWithNodes() {}
        virtual ElementPtr copy() { return std::make_shared<WayWithNodes>(Id(),Quadtree(),Info(),Tags(),refs,lonlats,bounds); }
    private:
        WayWithNodes(const WayWithNodes&)=delete;
        WayWithNodes operator=(const WayWithNodes&)=delete;

        refvector refs;
        lonlatvec lonlats;
        bbox bounds;

};
    

}
}

#endif
