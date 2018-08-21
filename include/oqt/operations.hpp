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
 
#ifndef OPERATIONS_HPP
#define OPERATIONS_HPP
#include "oqt/readpbffile.hpp"
#include "oqt/utils.hpp"

#include "oqt/calcqts/qtstoresplit.hpp"
#include "oqt/calcqts/waynodesfile.hpp"
#include "oqt/calcqts/calculaterelations.hpp"

namespace oqt {
class qttree;

int run_calcqts(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool splitways, bool resort, double buffer, size_t max_depth);
int run_calcqts_inmem(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool resort);



void find_way_quadtrees(
    const std::string& source_filename,
    const std::vector<int64>& source_locs, 
    size_t numchan,
    std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns,
    double buffer, size_t max_depth,int64 minway, int64 maxway);


void write_qts_file(const std::string& qtsfn, const std::string& nodes_fn, size_t numchan,
    const std::vector<int64>& node_locs, std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns, std::shared_ptr<calculate_relations> rels, double buffer, size_t max_depth);

typedef std::function<void(primitiveblock_ptr)> primitiveblock_callback;
class SortBlocks {
    public:
        virtual std::vector<primitiveblock_callback> make_addblocks_cb(bool threaded)=0;
        virtual void finish()=0;
                
        virtual void read_blocks(std::vector<primitiveblock_callback> packers, bool sortobjs)=0;
        
        virtual ~SortBlocks() {}
};

std::shared_ptr<SortBlocks> make_sortblocks(int64 orig_file_size, std::shared_ptr<qttree> groups,
    const std::string& tempfn, size_t blocksplit, size_t numchan);

int run_sortblocks(
    const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<qttree> groups,
    const std::string& tempfn, size_t grptiles);

int run_sortblocks_inmem(const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<qttree> groups);
    


std::shared_ptr<qttree> make_qts_tree(const std::string& qtsfn, size_t numchan);
std::shared_ptr<qttree> make_qts_tree_maxlevel(const std::string& qtsfn, size_t numchan, size_t maxlevel);

void run_mergechanges(
    const std::string& infile_name,
    const std::string& outfn,
    size_t numchan, bool sort_objs, bool filter_objs,
    bbox filter_box, const lonlatvec& poly, int64 enddate,
    const std::string& tempfn, size_t blocksize, bool sortfile, bool inmem);

void run_applychange(const std::string& origfn, const std::string& outfn, size_t numchan, const std::vector<std::string>& changes);
}
#endif
