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


#include "oqt/utils/pbf/protobuf.hpp"

#include "oqt/utils/logger.hpp"

namespace oqt {
    
    
    


std::string read_data(const std::string& data, size_t& pos) {
    size_t uv = read_unsigned_varint(data,pos);
    
    size_t p = pos;
    pos += uv;
    return std::move(data.substr(p,uv));
}

PbfTag read_pbf_tag(const std::string& data, size_t& pos) {
    if ((pos) == data.size()) {
        return PbfTag{0,0,""};
    }
    uint64_t tag = read_unsigned_varint(data,pos);
    
    if ((tag&7) == 0) {
        return PbfTag{tag>>3, read_unsigned_varint(data,pos), ""};
    } else if ((tag&7)==2) {
        return PbfTag{tag>>3, 0, read_data(data,pos)};
    }
    Logger::Message() << "?? @ " << pos << "/" << data.size() << ": " << tag << " "  << (tag&7) << " " << (tag>>3);

    throw std::domain_error("only understand varint & data");
    return PbfTag{0,0,""};
}



size_t write_pbf_value(std::string& data, size_t pos, uint64 t, uint64 v) {
    pos = write_unsigned_varint(data,pos, t<<3);
    pos = write_unsigned_varint(data,pos, v);
    return pos;
}


size_t pbf_value_length(uint64 t, size_t val) {
    return unsigned_varint_length( (t<<3)) + unsigned_varint_length(val);
}


size_t pbf_data_length(uint64 t, size_t data_len) {
    return unsigned_varint_length( (t<<3)|2 ) + unsigned_varint_length(data_len) + data_len;
}

size_t write_pbf_data_header(std::string& data, size_t pos, uint64 t, size_t ln) {
    pos = write_unsigned_varint(data,pos, (t<<3) | 2);
    pos = write_unsigned_varint(data,pos, ln);
    return pos;
}

size_t write_pbf_data(std::string& data, size_t pos, uint64 t, const std::string& v) {
    pos = write_pbf_data_header(data,pos, t, v.size());
    std::copy(v.begin(),v.end(),data.begin()+pos);
    pos+=v.size();
    return pos;
}


void sort_pbf_tags(std::list<PbfTag>& msgs) {
    msgs.sort([](const PbfTag& l, const PbfTag& r) { return l.tag<r.tag; });
}

std::string pack_pbf_tags(const std::list<PbfTag>& msgs, bool forceData) {

    size_t ln=0;
    /*for (const auto& mm: msgs) {
        ln+=20;
        if (!mm.data.empty()) {
            ln+=mm.data.size();
        }
    }*/
    for (const auto& mm: msgs) {
        if (!mm.data.empty() || forceData) {
            ln+=pbf_data_length(mm.tag,mm.data.size());
        } else {
            ln+=pbf_value_length(mm.tag,mm.value);
        }
    }
    

    std::string out(ln,0);
    size_t pos=0;
    for (const auto& mm: msgs) {
        if (!mm.data.empty() || forceData) {
            pos=write_pbf_data(out,pos,mm.tag,mm.data);
        } else {
            pos=write_pbf_value(out,pos,mm.tag,mm.value);
        }
    }
    out.resize(pos);
    return out;
}


std::list<PbfTag> read_all_pbf_tags(const std::string& data) {
    size_t pos=0;
    std::list<PbfTag> mm;
    PbfTag tg=read_pbf_tag(data,pos);
    for ( ; tg.tag>0; tg=read_pbf_tag(data,pos)) {
        mm.push_back(tg);
    }
    return mm;
}
    
    
}
