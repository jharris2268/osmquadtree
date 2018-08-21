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

#ifndef SORTING_FINAL_HPP
#define SORTING_FINAL_HPP


#include "oqt/sorting/common.hpp"
namespace oqt {

std::function<void(keystring_ptr)> make_writepbf_callback(std::shared_ptr<PbfFileWriter> ww, int64 buffer);
std::vector<primitiveblock_callback> make_final_packers(std::shared_ptr<PbfFileWriter> write_file_obj, size_t numchan, int64 timestamp, bool writeqts, bool asthread);
std::vector<primitiveblock_callback> make_final_packers_sync(std::shared_ptr<PbfFileWriter> write_file_obj, size_t numchan, int64 timestamp, bool writeqts, bool asthread);
std::vector<primitiveblock_callback> make_final_packers_cb(std::function<void(keystring_ptr)> ks_cb, size_t numchan, int64 timestamp, bool writeqts, bool asthread);


}

#endif //SORTING_FINAL_HPP
