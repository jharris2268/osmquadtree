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

#ifndef UTILS_PBF_VARINT_HPP
#define UTILS_PBF_VARINT_HPP

#include "oqt/common.hpp"

namespace oqt {

uint64 zig_zag(int64 v);
int64 un_zig_zag(uint64 uv);

    
uint64 read_unsigned_varint(const std::string& data, size_t& pos);
int64 read_varint(const std::string& data, size_t& pos);

size_t unsigned_varint_length(uint64 value);
size_t write_unsigned_varint(std::string& data, size_t pos, uint64 uv);
size_t write_varint(std::string& data, size_t pos, int64 v);


}

#endif //PBFFORMAT_VARINT_HPP
