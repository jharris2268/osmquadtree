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

#ifndef CALCQTS_WAYNODES_HPP
#define CALCQTS_WAYNODES_HPP


#include "oqt/common.hpp"
namespace oqt {
class way_nodes {
    public:
        virtual size_t size() const =0;
        virtual std::pair<int64,int64> at(size_t i) const=0;
        virtual int64 key() const=0;
        virtual ~way_nodes() {}
};

std::shared_ptr<way_nodes> read_waynodes_block(const std::string& data, int64 minway, int64 maxway);
}
#endif
