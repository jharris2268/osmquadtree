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

#ifndef QUADTREEGROUPS_HPP
#define QUADTREEGROUPS_HPP

#include "oqt/simplepbf.hpp"
#include "oqt/packedblock.hpp"
#include "oqt/minimalblock.hpp"
#include "oqt/readfile.hpp"
#include "oqt/quadtree.hpp"
#include "oqt/writeblock.hpp"
#include <map>
namespace oqt {
struct qttree_item {

    int64  qt;
    uint32_t idx;
    uint32_t parent;
    int64 weight;
    int64 total;
    uint32_t children[4];

    qttree_item() : qt(0),idx(0),parent(0),weight(0),total(0),children{0,0,0,0} {}
};

class qttree {
    public:
        virtual qttree_item& at(size_t i)=0;
        virtual size_t size()=0;

        virtual size_t find(int64 qt)=0;
        virtual size_t next(size_t curr, size_t c=0)=0;

        virtual void rollup(size_t curr)=0;
        virtual void rollup_child(size_t curr, size_t ci)=0;
        virtual size_t clip(size_t curr)=0;
        virtual size_t add(int64 qt, int64 val)=0;

        virtual qttree_item find_tile(int64 qt)=0;
        virtual ~qttree() {}
};

typedef std::map<int64,int64> count_map;

void collect_block(std::shared_ptr<count_map> res, std::shared_ptr<minimalblock> block);

std::shared_ptr<qttree> make_tree_empty();


std::shared_ptr<CallFinish<count_map,void>> make_addcountmaptree(std::shared_ptr<qttree> tree, size_t maxlevel);

std::shared_ptr<qttree> find_groups_copy(std::shared_ptr<qttree> tree, int64 target, int64 minsize);
void tree_rollup(std::shared_ptr<qttree> tree, int64 minsize);
std::shared_ptr<qttree> tree_round_copy(std::shared_ptr<qttree> tree, int64 maxlevel);
}
#endif //QUADTREEGROUPS_HPP
