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

#ifndef ELEMENTS_NODE_HPP
#define ELEMENTS_NODE_HPP

#include "oqt/elements/element.hpp"
namespace oqt {
class node : public element {
    public:
        node(changetype c, int64 i, int64 q, info inf, std::vector<tag> tags, int64 lon, int64 lat)
            : element(elementtype::Node,c,i,q,inf,tags), lon_(lon), lat_(lat) {}

        //node(const node& n) : element(n), lon_(n.lon_), lat_(n.lat_) {}
        virtual ~node() {}

        int64 Lon() const { return lon_; }
        int64 Lat() const { return lat_; }

        //virtual packedobj_ptr pack();
        virtual std::list<PbfTag> pack_extras() const;
        virtual std::shared_ptr<element> copy() { return std::make_shared<node>(ChangeType(),Id(),Quadtree(),Info(),Tags(),Lon(),Lat()); }
    private:
        int64 lon_,lat_;
};
}
#endif
