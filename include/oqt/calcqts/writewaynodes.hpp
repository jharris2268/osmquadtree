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

#ifndef CALCQTS_WRITEWAYNODES_HPP
#define CALCQTS_WRITEWAYNODES_HPP


#include "oqt/common.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/calcqts/waynodesfile.hpp"





namespace oqt {
            
std::tuple<std::shared_ptr<WayNodesFile>,std::shared_ptr<CalculateRelations>,std::string,std::vector<int64>>
    write_waynodes(const std::string& orig_fn, const std::string& waynodes_fn, size_t numchan, bool sortinmem);
            
}

#endif
