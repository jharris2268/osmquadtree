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

#ifndef PBFFORMAT_READBLOCK_HPP
#define PBFFORMAT_READBLOCK_HPP

#include "oqt/common.hpp"
#include "oqt/pbfformat/idset.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/elements/header.hpp"
#include "oqt/elements/quadtree.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
namespace oqt {
    


typedef std::function<ElementPtr(ElementType, const std::string&, const std::vector<std::string>&, changetype)> read_geometry_func;
PrimitiveBlockPtr readPrimitiveBlock(int64 idx, const std::string& data, bool change,
    size_t objflags=7, IdSetPtr ids=IdSetPtr(),
    read_geometry_func readGeometry = read_geometry_func());

std::tuple<int64,info,tagvector,int64,std::list<PbfTag> >
    readCommon(ElementType ty, const std::string& data, const std::vector<std::string>& stringtable, IdSetPtr ids);

std::vector<std::string> readStringTable(const std::string& data);
int64 readQuadTree(const std::string& data);


HeaderPtr readPbfHeader(const std::string& data, int64 fl);


    
}
#endif //PBFFORMAT_READBLOCK_HPP
