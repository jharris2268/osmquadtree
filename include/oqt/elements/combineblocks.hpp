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

PrimitiveBlockPtr combine_primitiveblock_two(
    size_t idx,
    PrimitiveBlockPtr left,
    PrimitiveBlockPtr right,
    bool apply_change);

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

