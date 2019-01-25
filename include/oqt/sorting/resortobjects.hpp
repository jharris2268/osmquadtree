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

#ifndef SORTING_RESORTOBJECTS_HPP
#define SORTING_RESORTOBJECTS_HPP


#include "oqt/sorting/common.hpp"
#include "oqt/sorting/tempobjs.hpp"

namespace oqt {

//typedef std::function<void(std::shared_ptr<std::vector<PrimitiveBlockPtr>>)> primblock_vec_callback;

//primitiveblock_callback make_resortobjects_callback(primblock_vec_callback callback, std::shared_ptr<QtTree> groups, int64 blocksize, bool sortobjs);


std::vector<keyedblob_callback> make_resortobjects_callback_alt(
    std::shared_ptr<QtTree> groups, bool sortobjs,
    int64 timestamp, int complevel,
    std::function<void(keystring_ptr)> cb, size_t numchan);



std::vector<keyedblob_callback> make_resort_objects_collect_block(
    std::shared_ptr<QtTree> groups, bool sortobjs,
    int64 timestamp,
    primitiveblock_callback cb, size_t numchan);
}
#endif //SORTING_RESORTOBJECTS_HPP
