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
    
    
    


std::string readData(const std::string& data, size_t& pos) {
    size_t uv = readUVarint(data,pos);
    
    size_t p = pos;
    pos += uv;
    return std::move(data.substr(p,uv));
}

PbfTag readPbfTag(const std::string& data, size_t& pos) {
    if ((pos) == data.size()) {
        return PbfTag{0,0,""};
    }
    uint64_t tag = readUVarint(data,pos);
    
    if ((tag&7) == 0) {
        return PbfTag{tag>>3, readUVarint(data,pos), ""};
    } else if ((tag&7)==2) {
        return PbfTag{tag>>3, 0, readData(data,pos)};
    }
    Logger::Message() << "?? @ " << pos << "/" << data.size() << ": " << tag << " "  << (tag&7) << " " << (tag>>3);

    throw std::domain_error("only understand varint & data");
    return PbfTag{0,0,""};
}



size_t writePbfValue(std::string& data, size_t pos, uint64 t, uint64 v) {
    pos = writeUVarint(data,pos, t<<3);
    pos = writeUVarint(data,pos, v);
    return pos;
}


size_t pbfValueLength(uint64 t, size_t val) {
    return UVarintLength( (t<<3)) + UVarintLength(val);
}


size_t pbfDataLength(uint64 t, size_t data_len) {
    return UVarintLength( (t<<3)|2 ) + UVarintLength(data_len) + data_len;
}

size_t writePbfDataHeader(std::string& data, size_t pos, uint64 t, size_t ln) {
    pos = writeUVarint(data,pos, (t<<3) | 2);
    pos = writeUVarint(data,pos, ln);
    return pos;
}

size_t writePbfData(std::string& data, size_t pos, uint64 t, const std::string& v) {
    pos = writeUVarint(data,pos, (t<<3) | 2);
    pos = writeUVarint(data,pos, v.size());
    std::copy(v.begin(),v.end(),data.begin()+pos);
    pos+=v.size();
    return pos;
}


void sortPbfTags(std::list<PbfTag>& msgs) {
    msgs.sort([](const PbfTag& l, const PbfTag& r) { return l.tag<r.tag; });
}

std::string packPbfTags(const std::list<PbfTag>& msgs, bool forceData) {

    size_t ln=0;
    for (const auto& mm: msgs) {
        ln+=20;
        if (!mm.data.empty()) {
            ln+=mm.data.size();
        }
    }

    std::string out(ln,0);
    size_t pos=0;
    for (const auto& mm: msgs) {
        if (!mm.data.empty() || forceData) {
            pos=writePbfData(out,pos,mm.tag,mm.data);
        } else {
            pos=writePbfValue(out,pos,mm.tag,mm.value);
        }
    }
    out.resize(pos);
    return out;
}


std::list<PbfTag> readAllPbfTags(const std::string& data) {
    size_t pos=0;
    std::list<PbfTag> mm;
    PbfTag tg=readPbfTag(data,pos);
    for ( ; tg.tag>0; tg=readPbfTag(data,pos)) {
        mm.push_back(tg);
    }
    return mm;
}
    
    
}
