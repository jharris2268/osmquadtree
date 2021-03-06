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

#ifndef SORTING_SPLITBYID_HPP
#define SORTING_SPLITBYID_HPP
#include "oqt/sorting/common.hpp"

namespace oqt {


std::function<void(ElementPtr)> make_collectobjs(std::vector<primitiveblock_callback> callbacks, size_t blocksize);

primitiveblock_callback make_splitbyid_callback(primitiveblock_callback packers, size_t blocksplit, size_t writeat, int64 split_at);

int64 split_id_key(ElementPtr obj, int64 node_split_at, int64 way_split_at, int64 offset);
}

#endif //SORTING_SORTBYID_HPP
