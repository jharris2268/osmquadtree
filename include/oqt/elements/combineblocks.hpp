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
 
#ifndef COMBINEBLOCKS_HPP
#define COMBINEBLOCKS_HPP

#include "oqt/simplepbf.hpp"

#include "oqt/elements/block.hpp"
#include "oqt/elements/minimalblock.hpp"

namespace oqt {

std::shared_ptr<primitiveblock> combine_primitiveblock_two(
    size_t idx,
    std::shared_ptr<primitiveblock> left,
    std::shared_ptr<primitiveblock> right,
    bool apply_change);

std::shared_ptr<primitiveblock> combine_primitiveblock_many(
    std::shared_ptr<primitiveblock> main,
    const std::vector<std::shared_ptr<primitiveblock>>& changes);


std::shared_ptr<minimalblock> combine_minimalblock_two(
    size_t idx,
    std::shared_ptr<minimalblock> left,
    std::shared_ptr<minimalblock> right,
    bool apply_change);

std::shared_ptr<minimalblock> combine_minimalblock_many(
    std::shared_ptr<minimalblock> main,
    const std::vector<std::shared_ptr<minimalblock>>& changes);

}
#endif //COMBINEBLOCKS_HPP

