
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
 
#ifndef SORTING_SORTBLOCKS_HPP
#define SORTING_SORTBLOCKS_HPP

//#include "oqt/calcqts/qtstoresplit.hpp"
//#include "oqt/calcqts/waynodesfile.hpp"
//#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/sorting/qttree.hpp"
#include "oqt/elements/block.hpp"

namespace oqt {


class SortBlocks {
    public:
        virtual std::vector<primitiveblock_callback> make_addblocks_cb(bool threaded)=0;
        virtual void finish()=0;
                
        virtual void read_blocks(std::vector<primitiveblock_callback> packers, bool sortobjs)=0;
        
        virtual ~SortBlocks() {}
};

std::shared_ptr<SortBlocks> make_sortblocks(int64 orig_file_size, std::shared_ptr<QtTree> groups,
    const std::string& tempfn, size_t blocksplit, size_t numchan);

int run_sortblocks(
    const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<QtTree> groups,
    const std::string& tempfn, size_t grptiles);

int run_sortblocks_inmem(const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<QtTree> groups);



std::shared_ptr<QtTree> make_qts_tree(const std::string& qtsfn, size_t numchan);
std::shared_ptr<QtTree> make_qts_tree_maxlevel(const std::string& qtsfn, size_t numchan, size_t maxlevel);
}
#endif 

