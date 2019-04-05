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
#include "oqt/pbfformat/readblock.hpp"

namespace oqt {





minimal::BlockPtr read_minimal_block(int64 index, const std::string& data, ReadBlockFlags objflags=ReadBlockFlags::Empty);


std::shared_ptr<quadtree_vector> read_quadtree_vector_block(const std::string& data, ReadBlockFlags objflags=ReadBlockFlags::Empty);

}

#endif //PBFFORMAT_READMINIMAL_HPP
