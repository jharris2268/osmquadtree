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
class Node : public Element {
    public:
        Node(changetype c, int64 i, int64 q, info inf, std::vector<tag> tags, int64 lon, int64 lat);
        
        virtual ~Node() {}

        int64 Lon() const;
        int64 Lat() const;

        
        virtual ElementPtr copy();
    private:
        int64 lon_,lat_;
};
}
#endif
