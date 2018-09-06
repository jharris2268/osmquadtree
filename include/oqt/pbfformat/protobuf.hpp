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

#ifndef PBFFORMAT_PROTOBUF_HPP
#define PBFFORMAT_PROTOBUF_HPP

#include "oqt/common.hpp"

namespace oqt {


size_t write_uint32(std::string& data, size_t pos, uint32_t i);
size_t write_uint32_le(std::string& data, size_t pos, uint32_t i);
size_t write_double(std::string& data, size_t pos, double d);
size_t write_double_le(std::string& data, size_t pos, double d);

uint32_t read_uint32_le(std::string& data, size_t pos);

std::string readData(const std::string& data, size_t& pos);

//double toDouble(uint64 uv);

PbfTag readPbfTag(const std::string& data, size_t& pos);

std::list<PbfTag> readAllPbfTags(const std::string& data);


size_t writePbfValue(std::string& data, size_t pos, uint64 t, uint64 v);
size_t writePbfData(std::string& data, size_t pos, uint64 t, const std::string& v);

size_t writePbfDataHeader(std::string& data, size_t pos, uint64 t, size_t ln);

size_t pbfValueLength(uint64 t, uint64 val);
size_t pbfDataLength(uint64 t, size_t len);


void sortPbfTags(std::list<PbfTag>& msgs);

std::string packPbfTags(const std::list<PbfTag>& msgs, bool forceData=false);

    
}

#endif //PBFFORMAT_PROTOBUF_HPP
