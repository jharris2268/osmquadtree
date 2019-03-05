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
 
#ifndef ELEMENTS_COMBINEBLOCKS_HPP
#define ELEMENTS_COMBINEBLOCKS_HPP



#include "oqt/elements/block.hpp"
#include "oqt/elements/minimalblock.hpp"

namespace oqt {

/*! Construct new PrimitiveBlock with elements from both input blocks.
 * If same Element (type and id) present in both blocks, chose element in
 * right block. If apply_change filter out elements with changetype of
 * changetype::Remove or changetype::Delete, and set all remaining elements
 * to changetype::Normal */
PrimitiveBlockPtr combine_primitiveblock_two(
    size_t idx,
    PrimitiveBlockPtr left,
    PrimitiveBlockPtr right,
    bool apply_change);


/*! Equivilant to
 * \verbatim embed:rst:leading-asterisk
 *      size_t idx = main->index();
 *      PrimitiveBlockPtr merged_changes = changes.pop_back();
 *      while (!changes.empty()) {
 *          merged_changes =
 *              combine_primitiveblock_two(idx, changes.pop_back(), merged_changes, false);
 *      }
 *      return combine_primitiveblock_two(idx, main, merged_changes, true);
 * \endverbatim */
 
PrimitiveBlockPtr combine_primitiveblock_many(
    PrimitiveBlockPtr main,
    const std::vector<PrimitiveBlockPtr>& changes);


minimal::BlockPtr combine_minimalblock_two(
    size_t idx,
    minimal::BlockPtr left,
    minimal::BlockPtr right,
    bool apply_change);

minimal::BlockPtr combine_minimalblock_many(
    minimal::BlockPtr main,
    const std::vector<minimal::BlockPtr>& changes);

}
#endif //ELEMENTS_COMBINEBLOCKS_HPP

