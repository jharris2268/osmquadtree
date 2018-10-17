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


#include "oqt/utils/pbf/varint.hpp"

namespace oqt {
    
    
uint64 read_unsigned_varint(const std::string& data, size_t& pos) {
    uint64 result = 0;
    int count = 0;
    uint8_t b=0;

    do {
        if (count == 10) return 0;
        b = data[(pos)++];

        result |= (static_cast<uint64_t>(b & 0x7F) << (7 * count));

        ++count;
    } while (b & 0x80);

    return result;
}



int64 un_zig_zag(uint64 uv) {
    int64 x = (int64) (uv>>1);
    if ((uv&1)!=0) {
        x = x^(-1);
    }
    return x;
}

uint64 zig_zag(int64 v) {
    return (static_cast<uint64>(v) << 1) ^ (v >> 63);

}


int64 read_varint(const std::string& data, size_t& pos) {
    uint64_t uv = read_unsigned_varint(data,pos);
    return un_zig_zag(uv);
}




size_t write_varint(std::string& data, size_t pos, int64 v) {

    return write_unsigned_varint(data,pos,zig_zag(v));
}
size_t write_unsigned_varint(std::string& data, size_t pos, uint64 value) {
    while (value > 0x7F) {
      data[pos++] = (static_cast<char>(value) & 0x7F) | 0x80;
      value >>= 7;
    }
    data[pos++] = static_cast<char>(value) & 0x7F;

    return pos;
}

size_t unsigned_varint_length(uint64 value) {
    for (size_t i=1; i < 10; i++) {
        if (value < (1ull << (7*i))) {
            return i;
        }
    }
    return 10;
}





    
    
    
    
}
