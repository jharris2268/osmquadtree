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

#ifndef SORTING_QTTREE_HPP
#define SORTING_QTTREE_HPP

#include "oqt/common.hpp"
#include <map>
namespace oqt {


class QtTree {
    public:
        struct Item {

            int64  qt;
            uint32_t idx;
            uint32_t parent;
            int64 weight;
            int64 total;
            uint32_t children[4];

            Item() : qt(0),idx(0),parent(0),weight(0),total(0),children{0,0,0,0} {}
        };
    
    
        virtual Item& at(size_t i)=0;
        virtual size_t size()=0;

        virtual size_t find(int64 qt)=0;
        virtual size_t next(size_t curr, size_t c=0)=0;

        virtual void rollup(size_t curr)=0;
        virtual void rollup_child(size_t curr, size_t ci)=0;
        virtual size_t clip(size_t curr)=0;
        virtual size_t add(int64 qt, int64 val)=0;

        virtual Item find_tile(int64 qt)=0;
        virtual ~QtTree() {}

};


typedef std::map<int64,int64> count_map;


std::shared_ptr<QtTree> make_tree_empty();

std::function<void(std::shared_ptr<count_map>)> make_addcountmaptree(std::shared_ptr<QtTree> tree, size_t maxlevel);

}
#endif
