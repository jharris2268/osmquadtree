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

#include "oqt/geometry/elements/complicatedpolygon.hpp"

#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {


double fix_ring_direction(Ring& outers, std::vector<Ring>& inners) {
    double area = calc_ring_area(outers);
    if (area<0) {
        reverse_ring(outers);
        area *= -1;
    }
        
    for (auto& ii : inners) {
        double a = calc_ring_area(ii);
        if (a>0) {
            reverse_ring(ii);
            a *= -1;
        }
        area += a;
    }
    if (area < 0) {
        area = 0;
    }
    return area;
}

ComplicatedPolygon::ComplicatedPolygon(
    std::shared_ptr<Relation> rel, int64 part_,
    const Ring& outers_, const std::vector<Ring>& inners_,
    const std::vector<Tag>& tags, int64 zorder_,
    int64 layer_, int64 minzoom_) :
    
    BaseGeometry(ElementType::ComplicatedPolygon, changetype::Normal, rel->Id(), rel->Quadtree(), rel->Info(), tags,minzoom_),
    part(part_), outers(outers_), inners(inners_),zorder(zorder_), layer(layer_) {

    area = fix_ring_direction(outers, inners);
    
    for (const auto& o : outers.parts) {
        for (const auto& ll : o.lonlats) {
            bounds.expand_point(ll.lon,ll.lat);
        }
    }

}

ComplicatedPolygon::ComplicatedPolygon(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, int64 part_, const Ring& outers_, const std::vector<Ring>& inners_, int64 zorder_, int64 layer_, double area_, const bbox& bounds_, int64 minzoom_)
    : BaseGeometry(ElementType::ComplicatedPolygon,changetype::Normal,id,qt,inf,tags,minzoom_),
    part(part_), outers(outers_), inners(inners_),zorder(zorder_), layer(layer_), area(area_), bounds(bounds_) {}


uint64 ComplicatedPolygon::InternalId() const {
    return (6ull<<61) | (((uint64) Id()) << 16) | ((uint64) part);
}   
        

ElementType ComplicatedPolygon::OriginalType() const { return ElementType::Relation; }
const Ring& ComplicatedPolygon::OuterRing() const { return outers; }
const std::vector<Ring>& ComplicatedPolygon::InnerRings() const { return inners; }
int64 ComplicatedPolygon::ZOrder() const { return zorder; }
int64 ComplicatedPolygon::Layer() const { return layer; }
double ComplicatedPolygon::Area() const { return area; }
int64 ComplicatedPolygon::Part() const { return part; }
ElementPtr ComplicatedPolygon::copy() { return std::make_shared<ComplicatedPolygon>(
    Id(),Quadtree(),Info(),Tags(),part,outers,inners,zorder,layer,area,bounds,MinZoom()); }

bbox ComplicatedPolygon::Bounds() const { return bounds; }





std::string ComplicatedPolygon::Wkb(bool transform, bool srid) const {
    size_t sz=13;
    if (srid) { sz+=4; }
    size_t outer_len = ringpart_numpoints(outers);

    sz += 4+16*outer_len;
    std::vector<size_t> inners_lens;

    for (const auto& inner : inners) {
        size_t l=ringpart_numpoints(inner);
        inners_lens.push_back(l);
        sz += 4+16*l;
    }


    std::string res(sz,'\0');
    //res[0]='\1';
    res[4]='\3';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,epsg_code(transform));
    }
    pos = write_uint32(res,pos, 1+inners.size());

    pos = write_ringpart_ring(res, pos, outers, outer_len,transform);
    if (!inners.empty()) {
        for (size_t i=0; i < inners.size(); i++) {
            pos = write_ringpart_ring(res,pos,inners[i],inners_lens[i],transform);
        }
    }
    return res;
}


std::list<PbfTag> ComplicatedPolygon::pack_extras() const {
    

    std::list<PbfTag> extras;
    extras.push_back(PbfTag{12,zigZag(zorder),""});
    extras.push_back(PbfTag{16,zigZag(to_int(area*100)),""});

    extras.push_back(PbfTag{17,0,pack_ring(outers)});
    for (const auto& ii : inners) {
        extras.push_back(PbfTag{18,0,pack_ring(ii)});
    }
    extras.push_back(PbfTag{19,zigZag(part),""});
    if (MinZoom()>=0) {
        extras.push_back(PbfTag{22,uint64(MinZoom()),""});
    }
    if (layer!=0) {
        extras.push_back(PbfTag{24,zigZag(layer),""});
    }
    return extras;
}

}
}
