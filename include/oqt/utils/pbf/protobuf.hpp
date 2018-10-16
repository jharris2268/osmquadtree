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

#ifndef UTILS_PBF_PROTOBUF_HPP
#define UTILS_PBF_PROTOBUF_HPP


#include "oqt/utils/pbf/packedint.hpp"


#include <list>
namespace oqt {
struct PbfTag {
    uint64 tag;
    uint64 value;
    std::string data;
};


std::string read_data(const std::string& data, size_t& pos);

//double toDouble(uint64 uv);

PbfTag read_pbf_tag(const std::string& data, size_t& pos);

std::list<PbfTag> read_all_pbf_tags(const std::string& data);


size_t write_pbf_value(std::string& data, size_t pos, uint64 t, uint64 v);
size_t write_pbf_data(std::string& data, size_t pos, uint64 t, const std::string& v);

size_t write_pbf_data_header(std::string& data, size_t pos, uint64 t, size_t ln);

size_t pbf_value_length(uint64 t, uint64 val);
size_t pbf_data_length(uint64 t, size_t len);


void sort_pbf_tags(std::list<PbfTag>& msgs);

std::string pack_pbf_tags(const std::list<PbfTag>& msgs, bool forceData=false);

    
}

#endif //PBFFORMAT_PROTOBUF_HPP
