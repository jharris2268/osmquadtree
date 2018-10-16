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

#ifndef UTILS_PBF_PACKEDINT_HPP
#define UTILS_PBF_PACKEDINT_HPP



#include <tuple>
#include <list>

#include <functional>
#include "oqt/utils/pbf/varint.hpp"


namespace oqt {


std::vector<int64> read_packed_delta(const std::string& data);
std::vector<uint64> read_packed_int(const std::string& data);

std::string write_packed_delta(const std::vector<int64>& vals);
std::string write_packed_int(const std::vector<uint64>& vals);

size_t packed_delta_length(const std::vector<int64>& vals, size_t last);
size_t write_packed_delta_in_place(std::string& out, size_t pos, const std::vector<int64>& vals, size_t last);

size_t packed_delta_length_func(std::function<int64(size_t)>, size_t len);
size_t write_packed_delta_in_place_func(std::string& out, size_t pos, std::function<int64(size_t)>, size_t len);


template <class T>
std::string write_packed_delta_func(const std::vector<T>& vals, std::function<int64(const T&)> func) {
    std::string out(10*vals.size(),0);
    size_t pos=0;
    int64 p=0;
    for (const auto& vl: vals) {
        auto v = func(vl);
        pos=write_varint(out,pos,v-p);
        p=v;
    }
    out.resize(pos);
    return out;
}
template <class T>
std::string write_packed_int_func(const std::vector<T>& vals, std::function<uint64(const T&)> func) {
    std::string out(10*vals.size(),0);
    size_t pos=0;
    
    for (const auto& vl: vals) {
        pos=write_unsigned_varint(out,pos,func(vl));
    }
    out.resize(pos);
    return out;
}

    
    
}

#endif //PBFFORMAT_PACKEDINT_HPP
