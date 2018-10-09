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

#include "oqt/geometry/elements/waywithnodes.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {


std::list<PbfTag> WayWithNodes::pack_extras() const {
    

    std::list<PbfTag> extras;
    extras.push_back(PbfTag{8,0,writePackedDelta(refs)}); //refs
    extras.push_back(PbfTag{12,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lon; })}); //lons
    extras.push_back(PbfTag{13,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lat; })}); //lats
    return extras;
}

}
}
