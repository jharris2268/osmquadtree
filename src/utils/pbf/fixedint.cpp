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


#include "oqt/utils/pbf/fixedint.hpp"

namespace oqt {

size_t write_uint32(std::string& data, size_t pos, uint32_t i) {
    union {
        uint32_t si;
        char sc[4];
    };
    si=i;
    data[pos+3] = sc[0];
    data[pos+2] = sc[1];
    data[pos+1] = sc[2];
    data[pos] = sc[3];
    return pos+4;
}

size_t write_uint32_le(std::string& data, size_t pos, uint32_t i) {
    union {
        uint32_t si;
        char sc[4];
    };
    si=i;
    data[pos] = sc[0];
    data[pos+1] = sc[1];
    data[pos+2] = sc[2];
    data[pos+3] = sc[3];
    return pos+4;
}


uint32_t read_uint32_le(const std::string& data, size_t pos) {
    union {
        uint32_t si;
        char sc[4];
    };
    sc[0] = data[pos];
    sc[1] = data[pos+1];
    sc[2] = data[pos+2];
    sc[3] = data[pos+3];
    
    return si;
}

size_t write_double(std::string& data, size_t pos, double d) {
    union {
        double sd;
        char sc[8];
    };
    sd=d;
    data[pos+7] = sc[0];
    data[pos+6] = sc[1];
    data[pos+5] = sc[2];
    data[pos+4] = sc[3];
    data[pos+3] = sc[4];
    data[pos+2] = sc[5];
    data[pos+1] = sc[6];
    data[pos] = sc[7];
    return pos+8;
}

size_t write_double_le(std::string& data, size_t pos, double d) {
    union {
        double sd;
        char sc[8];
    };
    sd=d;
    data[pos] = sc[0];
    data[pos+1] = sc[1];
    data[pos+2] = sc[2];
    data[pos+3] = sc[3];
    data[pos+4] = sc[4];
    data[pos+5] = sc[5];
    data[pos+6] = sc[6];
    data[pos+7] = sc[7];
    return pos+8;
}


size_t write_int16(std::string& data, size_t pos, int64 i) {
    union {
        int16_t sd;
        char sc[2];
    };
    sd=i;
    data[pos+1] = sc[0];
    data[pos]   = sc[1];
    
    return pos+2;
}
size_t write_int32(std::string& data, size_t pos, int64 i) {
    union {
        int32_t sd;
        char sc[4];
    };
    sd=i;
    data[pos+3] = sc[0];
    data[pos+2] = sc[1];
    data[pos+1] = sc[2];
    data[pos]   = sc[3];
    
    return pos+4;
}
size_t write_int64(std::string& data, size_t pos, int64 i) {
    union {
        int64_t sd;
        char sc[8];
    };
    sd=i;
    data[pos+7] = sc[0];
    data[pos+6] = sc[1];
    data[pos+5] = sc[2];
    data[pos+4] = sc[3];
    data[pos+3] = sc[4];
    data[pos+2] = sc[5];
    data[pos+1] = sc[6];
    data[pos] = sc[7];
    return pos+8;
}






}
