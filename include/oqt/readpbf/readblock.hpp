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

#ifndef READPBF_READBLOCK_HPP
#define READPBF_READBLOCK_HPP

#include "oqt/common.hpp"
#include "oqt/readpbf/idset.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/elements/header.hpp"
#include "oqt/quadtree.hpp"

namespace oqt {
    


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
#endif //READPBF_READBLOCK_HPP
