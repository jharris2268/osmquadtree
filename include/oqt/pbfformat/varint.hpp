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

#ifndef PBFFORMAT_VARINT_HPP
#define PBFFORMAT_VARINT_HPP

#include "oqt/common.hpp"

namespace oqt {
    
uint64 readUVarint(const std::string& data, size_t& pos);
int64 unZigZag(uint64 uv);
int64 readVarint(const std::string& data, size_t& pos);
size_t writeVarint(std::string& data, size_t pos, int64 v);
size_t writeUVarint(std::string& data, size_t pos, uint64 uv);
uint64 zigZag(int64 v);
    
}

#endif //PBFFORMAT_VARINT_HPP
