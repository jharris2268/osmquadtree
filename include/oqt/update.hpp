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

#ifndef UPDATE_HPP
#define UPDATE_HPP
#include "oqt/quadtreegroups.hpp"
#include "oqt/count.hpp"
#include "oqt/xmlchange.hpp"
#include "oqt/combineblocks.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/store.hpp"
#include <set>
namespace oqt {
std::shared_ptr<header> getHeaderBlock(const std::string& fn);
size_t writeIndexFile(const std::string& srcfn, size_t numchan, const std::string& destfn);
std::set<int64> checkIndexFile(const std::string& idxfn, std::shared_ptr<header> head, size_t numchan, std::shared_ptr<idset> ids);

std::vector<std::shared_ptr<primitiveblock>> read_file_blocks(
    const std::string& fn, std::vector<int64> locs, size_t numchan,
    size_t index_offset, bool change, size_t objflags, std::shared_ptr<idset> ids);

std::tuple<
    std::shared_ptr<qtstore>, //orig allocs
    std::shared_ptr<qtstore>, //qts
    std::shared_ptr<qttree>   //tree
> add_orig_elements(
        typeid_element_map_ptr objs,
        const std::string& prfx,
        const std::vector<std::string>& fls);

std::tuple<
    std::shared_ptr<qtstore>, //orig allocs
    std::shared_ptr<qtstore>, //qts
    std::shared_ptr<qttree>   //tree
> add_orig_elements_alt(
        typeid_element_map_ptr objs,
        const std::string& prfx,
        const std::vector<std::string>& fls);



void calc_change_qts(typeid_element_map_ptr objs, std::shared_ptr<qtstore> qts);

std::vector<std::shared_ptr<primitiveblock>> find_change_tiles(
    typeid_element_map_ptr objs,
    std::shared_ptr<qtstore> orig_allocs,
    std::shared_ptr<qttree> tree,
    int64 startdate,
    int64 enddate);

std::pair<int64,int64> find_change_all(const std::string& src, const std::string& prfx, const std::vector<std::string>& fls, int64 st, int64 et, const std::string& outfn);


}
#endif
