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
 
#ifndef CALCQTS_CALCQTS_HPP
#define CALCQTS_CALCQTS_HPP

#include "oqt/calcqts/qtstoresplit.hpp"
#include "oqt/calcqts/waynodesfile.hpp"
#include "oqt/calcqts/calculaterelations.hpp"

namespace oqt {

int run_calcqts(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool splitways, bool resort, double buffer, size_t max_depth);
int run_calcqts_inmem(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool resort);



void find_way_quadtrees(
    const std::string& source_filename,
    const std::vector<int64>& source_locs, 
    size_t numchan,
    std::shared_ptr<QtStoreSplit> way_qts,
    std::shared_ptr<WayNodesFile> wns,
    double buffer, size_t max_depth,int64 minway, int64 maxway);


void write_qts_file(const std::string& qtsfn, const std::string& nodes_fn, size_t numchan,
    const std::vector<int64>& node_locs, std::shared_ptr<QtStoreSplit> way_qts,
    std::shared_ptr<WayNodesFile> wns, std::shared_ptr<CalculateRelations> rels, double buffer, size_t max_depth);

}
#endif


