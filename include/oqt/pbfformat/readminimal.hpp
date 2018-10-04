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

#ifndef PBFFORMAT_READMINIMAL_HPP
#define PBFFORMAT_READMINIMAL_HPP

#include "oqt/common.hpp"
#include "oqt/elements/minimalblock.hpp"
#include "oqt/elements/quadtree.hpp"

namespace oqt {





minimal::BlockPtr readMinimalBlock(int64 index, const std::string& data, size_t objflags=7);

typedef std::vector<std::pair<uint64,int64> > qtvec;
std::shared_ptr<qtvec> readQtVecBlock(const std::string& data, size_t objflags=7);

}

#endif //PBFFORMAT_READMINIMAL_HPP
