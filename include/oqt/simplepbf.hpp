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

#ifndef SIMPLEPBF_HPP
#define SIMPLEPBF_HPP

#include "oqt/common.hpp"

#include <tuple>
#include <list>
#include <set>
#include <functional>
#include "oqt/elements/block.hpp"
#include "oqt/elements/element.hpp"
#include "oqt/elements/geometry.hpp"
#include "oqt/elements/header.hpp"
#include "oqt/elements/info.hpp"
#include "oqt/elements/member.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/packedobj.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/tag.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/readpbf/idset.hpp"
#include "oqt/readpbf/objsidset.hpp"

namespace oqt {



uint64 readUVarint(const std::string& data, size_t& pos);
int64 unZigZag(uint64 uv);
int64 readVarint(const std::string& data, size_t& pos);
std::string readData(const std::string& data, size_t& pos);

//double toDouble(uint64 uv);

PbfTag readPbfTag(const std::string& data, size_t& pos);

std::list<PbfTag> readAllPbfTags(const std::string& data);

std::vector<int64> readPackedDelta(const std::string& data);
std::vector<uint64> readPackedInt(const std::string& data);


size_t writeVarint(std::string& data, size_t pos, int64 v);
size_t writeUVarint(std::string& data, size_t pos, uint64 uv);


std::string writePackedDelta(const std::vector<int64>& vals);
std::string writePackedInt(const std::vector<uint64>& vals);

size_t packedDeltaLength(const std::vector<int64>& vals, size_t last);
size_t writePackedDeltaInPlace(std::string& out, size_t pos, const std::vector<int64>& vals, size_t last);

size_t packedDeltaLength_func(std::function<int64(size_t)>, size_t len);
size_t writePackedDeltaInPlace_func(std::string& out, size_t pos, std::function<int64(size_t)>, size_t len);


template <class T>
std::string writePackedDeltaFunc(const std::vector<T>& vals, std::function<int64(const T&)> func) {
    std::string out(10*vals.size(),0);
    size_t pos=0;
    int64 p=0;
    for (const auto& vl: vals) {
        auto v = func(vl);
        pos=writeVarint(out,pos,v-p);
        p=v;
    }
    out.resize(pos);
    return out;
}
template <class T>
std::string writePackedIntFunc(const std::vector<T>& vals, std::function<uint64(const T&)> func) {
    std::string out(10*vals.size(),0);
    size_t pos=0;
    
    for (const auto& vl: vals) {
        pos=writeUVarint(out,pos,func(vl));
    }
    out.resize(pos);
    return out;
}

size_t writePbfValue(std::string& data, size_t pos, uint64 t, uint64 v);
size_t writePbfData(std::string& data, size_t pos, uint64 t, const std::string& v);

size_t writePbfDataHeader(std::string& data, size_t pos, uint64 t, size_t ln);

size_t pbfValueLength(uint64 t, uint64 val);
size_t pbfDataLength(uint64 t, size_t len);


void sortPbfTags(std::list<PbfTag>& msgs);

std::string packPbfTags(const std::list<PbfTag>& msgs, bool forceData=false);

uint64 zigZag(int64 v);






typedef std::function<std::shared_ptr<element>(elementtype, const std::string&, const std::vector<std::string>&, changetype)> read_geometry_func;
std::shared_ptr<primitiveblock> readPrimitiveBlock(int64 idx, const std::string& data, bool change,
    size_t objflags=7, std::shared_ptr<idset> ids=std::shared_ptr<idset>(),
    read_geometry_func readGeometry = read_geometry_func());

std::tuple<int64,info,tagvector,int64,std::list<PbfTag> >
    readCommon(elementtype ty, const std::string& data, const std::vector<std::string>& stringtable, std::shared_ptr<idset> ids);

std::vector<std::string> readStringTable(const std::string& data);
int64 readQuadTree(const std::string& data);


std::shared_ptr<header> readPbfHeader(const std::string& data, int64 fl);

std::string fix_str(const std::string& s);

}

#endif
