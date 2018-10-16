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


#include "oqt/utils/pbf/packedint.hpp"

namespace oqt {





std::vector<int64> read_packed_delta(const std::string& data) {
    size_t pos = 0;
    std::vector<int64> ans;

    int64 curr=0;
    size_t sz=0;
    for (auto it=data.begin(); it < data.end(); ++it) {
        if ((unsigned char)(*it) < 127) {
            sz++;
        }
    }
    ans.reserve(sz);

    while (pos<data.size()) {
        curr += read_varint(data,pos);
        ans.push_back(curr);
    }
    return std::move(ans);
}


std::vector<uint64> read_packed_int(const std::string& data)
{
    size_t pos = 0;
    std::vector<uint64> ans;
    size_t sz=0;
    for (auto it=data.begin(); it < data.end(); ++it) {
        if ((unsigned char)(*it) < 127) {
            sz++;
        }
    }
    ans.reserve(sz);

    while (pos<data.size()) {
        ans.push_back(read_unsigned_varint(data,pos));
    }
    return std::move(ans);
}





size_t packed_delta_length(const std::vector<int64>& vals, size_t last) {
    size_t l=0;
    int64 p =0;
    if (last == 0) { last = vals.size(); }
    for (size_t i=0; i < last; i++) {
        l += unsigned_varint_length(zig_zag(vals[i]-p));
        p=vals[i];
    }
    return l;
}


size_t write_packed_delta_in_place(std::string& out, size_t pos, const std::vector<int64>& vals, size_t last) {
    int64 p=0;
    if (last == 0) { last = vals.size(); }
    for (size_t i=0; i < last; i++) {
        pos=write_varint(out,pos,vals[i]-p);
        p=vals[i];
    }
    return pos;
}

size_t packed_delta_length_func(std::function<int64(size_t)> func, size_t len) {
    size_t l=0;
    int64 p=0;
    for (size_t i=0; i < len; i++) {
        int64 q = func(i);
        l += unsigned_varint_length(zig_zag(q-p));
        p=q;
    }
    return l;
}
    
size_t write_packed_delta_in_place_func(std::string& out, size_t pos, std::function<int64(size_t)> func, size_t len) {
    int64 p=0;
    for (size_t i=0; i < len; i++) {
        int64 q=func(i);
        pos=write_varint(out,pos,q-p);
        p=q;
    }
    return pos;
}



std::string write_packed_delta(const std::vector<int64>& vals) {
    std::string out(10*vals.size(),0);
    size_t pos=write_packed_delta_in_place(out,0,vals,0);
    out.resize(pos);
    return out;
}


std::string write_packed_int(const std::vector<uint64>& vals) {
    std::string out(10*vals.size(),0);
    size_t pos=0;

    for (auto v: vals) {
        pos=write_unsigned_varint(out,pos,v);
    }
    out.resize(pos);
    return out;
}
    
    
    
    
    
}
