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

#include "oqt/geometry/elements/simplepolygon.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {



std::string SimplePolygon::Wkb(bool transform, bool srid) const {

    std::string res((srid?17:13)+16*lonlats.size(),'\0');
    //res[0]='\1';
    res[4]='\3';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,epsg_code(transform));
    }
    pos=write_uint32(res,pos, 1);
    if (reversed) {
        lonlatvec ll = lonlats;
        std::reverse(ll.begin(),ll.end());
        write_ring(res, pos,ll, transform);
    } else {
        write_ring(res, pos,lonlats, transform);
    }
    return res;
}


std::list<PbfTag> SimplePolygon::pack_extras() const {
    std::list<PbfTag> extras;
    
    extras.push_back(PbfTag{8,0,writePackedDelta(refs)}); //refs
    extras.push_back(PbfTag{12,zigZag(zorder),""});
    extras.push_back(PbfTag{13,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lon; })}); //lons
    extras.push_back(PbfTag{14,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lat; })}); //lats
    extras.push_back(PbfTag{16,zigZag(to_int(area*100)),""});
    
    if (MinZoom()>=0) {
        extras.push_back(PbfTag{22,uint64(MinZoom()),""});
    }
    if (reversed) {
        extras.push_back(PbfTag{23,1,""});
    }
    if (layer!=0) {
        extras.push_back(PbfTag{24,zigZag(layer),""});
    }
    return extras;
}

}
}
