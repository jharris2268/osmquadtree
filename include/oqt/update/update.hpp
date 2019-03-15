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

#ifndef UPDATE_UPDATE_HPP
#define UPDATE_UPDATE_HPP
#include "oqt/calcqts/qtstore.hpp"
#include "oqt/pbfformat/idset.hpp"
#include "oqt/update/xmlchange.hpp"
#include "oqt/elements/combineblocks.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/sorting/tempobjs.hpp"
#include <set>
namespace oqt {

size_t write_index_file(const std::string& srcfn, size_t numchan, const std::string& destfn);
std::set<int64> check_index_file(const std::string& idxfn, HeaderPtr header, size_t numchan, IdSetPtr ids);

std::vector<PrimitiveBlockPtr> read_file_blocks(
    const std::string& fn, std::vector<int64> locs, size_t numchan,
    size_t index_offset, bool change, ReadBlockFlags objflags, IdSetPtr ids);

std::tuple<
    std::shared_ptr<QtStore>, //orig allocs
    std::shared_ptr<QtStore>, //qts
    std::shared_ptr<QtTree>   //tree
> add_orig_elements(
        typeid_element_map_ptr objs,
        const std::string& prfx,
        const std::vector<std::string>& fls);

std::tuple<
    std::shared_ptr<QtStore>, //orig allocs
    std::shared_ptr<QtStore>, //qts
    std::shared_ptr<QtTree>   //tree
> add_orig_elements_alt(
        typeid_element_map_ptr objs,
        const std::string& prfx,
        const std::vector<std::string>& fls);



void calc_change_qts(typeid_element_map_ptr objs, std::shared_ptr<QtStore> qts);

std::vector<PrimitiveBlockPtr> find_change_tiles(
    typeid_element_map_ptr objs,
    std::shared_ptr<QtStore> orig_allocs,
    std::shared_ptr<QtTree> tree,
    int64 startdate,
    int64 enddate);

std::pair<int64,int64> find_change_all(const std::vector<std::string>& src_filenames, const std::string& prfx, const std::vector<std::string>& fls, int64 st, int64 et, const std::string& outfn);


}
#endif
