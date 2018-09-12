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
#include "oqt/pbfformat/writepbffile.hpp"


namespace oqt {
class way_nodes {
    public:
        virtual size_t size() const =0;
        virtual int64 node_at(size_t i) const=0;
        virtual int64 way_at(size_t i) const=0;
        virtual int64 key() const=0;
        virtual ~way_nodes() {}
};

class way_nodes_write : public way_nodes {
    public:
        virtual bool add(int64 n, int64 w, bool resize)=0;
        virtual void reset(bool)=0;
        virtual void reserve(size_t cap)=0;
        virtual void sort_node()=0;
        virtual void sort_way()=0;
        virtual std::string str() const=0;
};

std::shared_ptr<way_nodes> make_way_nodes(size_t cap, int64 key);
std::shared_ptr<way_nodes_write> make_way_nodes_write(size_t cap, int64 key);

keystring_ptr pack_waynodes_block(std::shared_ptr<way_nodes> wns);
std::shared_ptr<way_nodes> read_waynodes_block(const std::string& data, int64 minway, int64 maxway);
}
#endif
