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

#ifndef CALCQTS_WAYNODESFILE_HPP
#define CALCQTS_WAYNODESFILE_HPP

#include "oqt/common.hpp"
#include "oqt/calcqts/waynodes.hpp"
#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/utils/logger.hpp"

namespace oqt {
class WayNodesFile {
    public:
        virtual void read_waynodes(std::function<void(std::shared_ptr<WayNodes>)> cb, int64 minway, int64 maxway)=0;
        virtual void remove_file()=0;
        virtual ~WayNodesFile() {}
};

std::shared_ptr<WayNodesFile> make_waynodesfile(const std::string& fn, const std::vector<int64>& ll);


}
#endif
