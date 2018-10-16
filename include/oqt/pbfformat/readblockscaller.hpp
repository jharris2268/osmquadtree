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

#ifndef PBFFORMAT_READBLOCKSCALLER_HPP
#define PBFFORMAT_READBLOCKSCALLER_HPP

#include "oqt/pbfformat/idset.hpp"

#include "oqt/elements/block.hpp"
#include "oqt/elements/minimalblock.hpp"
#include "oqt/utils/bbox.hpp"
#include "oqt/utils/geometry.hpp"


namespace oqt {


std::pair<std::vector<std::string>,int64> read_filenames(const std::string& prfx, int64 enddate);
class ReadBlocksCaller {
    public:
        virtual void read_primitive(std::vector<primitiveblock_callback> cbs, IdSetPtr filter)=0;
        virtual void read_minimal(std::vector<minimalblock_callback> cbs, IdSetPtr filter)=0;
        virtual size_t num_tiles()=0;
};

std::shared_ptr<ReadBlocksCaller> make_read_blocks_caller(
        const std::string& infile_name, 
        bbox& filter_box, const std::vector<LonLat>& poly, int64& enddate);

}

#endif
