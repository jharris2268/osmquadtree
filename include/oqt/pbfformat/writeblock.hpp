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

#ifndef PBFFORMAT_WRITEBLOCK_H
#define PBFFORMAT_WRITEBLOCK_H

#include "oqt/elements/block.hpp"

#include "oqt/elements/header.hpp"

namespace oqt {
std::string pack_primitive_block(PrimitiveBlockPtr block, bool includeQts, bool change, bool includeInfo, bool includeRefs);
std::string pack_header_block(HeaderPtr head, bool seperateFileLocs);
void write_filelocs_json(const block_index& index, const std::string& pbffilename);
}


#endif
