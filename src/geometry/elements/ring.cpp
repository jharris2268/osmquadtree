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

#include "oqt/geometry/elements/ring.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {

lonlatvec ringpart_lonlats(const Ring& ring) {
    size_t np=1;
    for (const auto& r : ring.parts) { np += (r.lonlats.size()-1); };
    lonlatvec ll = ring.parts[0].lonlats;
    ll.reserve(np);

    if (ring.parts[0].reversed) {
        std::reverse(ll.begin(),ll.end());
    }

    for (size_t i=1; i < ring.parts.size(); i++) {
        //size_t cl = ll.size()-1;
        ll.pop_back();
        if (ring.parts[i].reversed) {
            std::copy(ring.parts[i].lonlats.rbegin(),ring.parts[i].lonlats.rend(), std::back_inserter(ll));
        } else {
            std::copy(ring.parts[i].lonlats.begin(),ring.parts[i].lonlats.end(), std::back_inserter(ll));
        }
    }
    return ll;
}

refvector ringpart_refs(const Ring& ring) {
    size_t np=1;
    for (const auto& r : ring.parts) { np += (r.refs.size()-1); };

    refvector ll = ring.parts[0].refs;
    ll.reserve(np);

    if (ring.parts[0].reversed) {
        std::reverse(ll.begin(),ll.end());
    }

    for (size_t i=1; i < ring.parts.size(); i++) {
        //size_t cl = ll.size()-1;
        ll.pop_back();
        if (ring.parts[i].reversed) {
            std::copy(ring.parts[i].refs.rbegin(),ring.parts[i].refs.rend(), std::back_inserter(ll));
        } else {
            std::copy(ring.parts[i].refs.begin(),ring.parts[i].refs.end(), std::back_inserter(ll));
        }
    }
    return ll;
}

void reverse_ring(Ring& ring) {
    for (auto& rp: ring.parts) {
        rp.reversed = !rp.reversed;
    }
    std::reverse(ring.parts.begin(), ring.parts.end());
}


double calc_ring_area(const Ring& ring) {
    if (ring.parts.empty()) { return 0; }
    if (ring.parts.size()==1) {
        return calc_ring_area(ring.parts[0].lonlats);
    }
    auto ll = ringpart_lonlats(ring);
    return calc_ring_area(ll);
}

size_t write_ring(std::string& data, size_t pos, const lonlatvec& lonlats, bool transform) {
    write_uint32(data,pos,lonlats.size());
    pos+=4;

    for (const auto& l : lonlats) {
        write_point(data,pos,l, transform);
        pos+=16;
    }
    return pos;
}

std::string pack_ringpart(const Ring::Part& rp) {
    std::string refsp = writePackedDelta(rp.refs);
    std::string lonsp = writePackedDeltaFunc<lonlat>(rp.lonlats, [](const lonlat& l) { return l.lon; });
    std::string latsp = writePackedDeltaFunc<lonlat>(rp.lonlats, [](const lonlat& l) { return l.lat; });
    std::string res;
    res.resize(40+refsp.size()+lonsp.size()+latsp.size());
    size_t pos=writePbfValue(res,0,1,rp.orig_id);
    pos = writePbfData(res,pos,2,refsp);
    pos = writePbfData(res,pos,3,lonsp);
    pos = writePbfData(res,pos,4,latsp);
    if (rp.reversed) {
        pos = writePbfValue(res,pos,5,1);
    }
    res.resize(pos);
    return res;
}



std::string pack_ring(const Ring& rps) {
    std::list<PbfTag> rr;
    for (const auto& rp : rps.parts) {
        rr.push_back(PbfTag{1,0,pack_ringpart(rp)});
    }
    return packPbfTags(rr);
}

size_t ringpart_numpoints(const Ring& rpv) {
    size_t r=1;
    for (const auto& rp : rpv.parts) {
        r += (rp.refs.size()-1);
    }
    return r;
}

size_t write_ringpart_ring(std::string& data, size_t pos, const Ring& rpv, size_t r, bool transform) {
    pos = write_uint32(data, pos, r);
    bool first=true;
    for (const auto& rp : rpv.parts) {
        size_t rps=rp.lonlats.size();
        for (size_t i=(first?0:1); i < rps; i++) {
            pos=write_point(data,pos,rp.lonlats[rp.reversed ? (rps-i-1) : i], transform);
        }
        first=false;
    }
    return pos;
}




}
}
