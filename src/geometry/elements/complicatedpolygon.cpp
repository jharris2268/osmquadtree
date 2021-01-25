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
    std::shared_ptr<Relation> rel, //int64 part_,
    //const Ring& outers_, const std::vector<Ring>& inners_,
    const std::vector<PolygonPart>& parts_,
    const std::vector<Tag>& tags, std::optional<int64> zorder_,
    std::optional<int64> layer_, std::optional<int64> minzoom_) :
    
    BaseGeometry(ElementType::ComplicatedPolygon, changetype::Normal, rel->Id(), rel->Quadtree(), rel->Info(), tags,minzoom_),
    parts(parts_)/*, outers(outers_), inners(inners_)*/,zorder(zorder_), layer(layer_) {


    CheckParts();

    

}

ComplicatedPolygon::ComplicatedPolygon(int64 id, int64 qt, const ElementInfo& inf, const std::vector<Tag>& tags, 
    //int64 part_, const Ring& outers_, const std::vector<Ring>& inners_,
    const std::vector<PolygonPart>& parts_,
    std::optional<int64> zorder_, std::optional<int64> layer_, /*double area_,*/ const bbox& bounds_, std::optional<int64> minzoom_)
    : BaseGeometry(ElementType::ComplicatedPolygon,changetype::Normal,id,qt,inf,tags,minzoom_),
    parts(parts_),/* outers(outers_), inners(inners_),*/ zorder(zorder_), layer(layer_), /*area(area_),*/ bounds(bounds_) {}


uint64 ComplicatedPolygon::InternalId() const {
    //return (6ull<<61) | (((uint64) Id()) << 16) | ((uint64) part);
    return (6ull<<61) | (uint64) Id();
}   

void ComplicatedPolygon::CheckParts() {
    for (auto& part: parts) {
        part.area = fix_ring_direction(part.outer, part.inners);
    
        for (const auto& o : part.outer.parts) {
            for (const auto& ll : o.lonlats) {
                expand_point(bounds, ll.lon,ll.lat);
            }
        }
    }
}


ElementType ComplicatedPolygon::OriginalType() const { return ElementType::Relation; }
//const Ring& ComplicatedPolygon::OuterRing() const { return outers; }
//const std::vector<Ring>& ComplicatedPolygon::InnerRings() const { return inners; }

const std::vector<PolygonPart>& ComplicatedPolygon::Parts() const { return parts; }

std::optional<int64> ComplicatedPolygon::ZOrder() const { return zorder; }
std::optional<int64> ComplicatedPolygon::Layer() const { return layer; }
double ComplicatedPolygon::Area() const {
    double a=0;
    for (const auto& p: parts) {
        a+=p.area;
    }
    return a;
}
//int64 ComplicatedPolygon::Part() const { return part; }
ElementPtr ComplicatedPolygon::copy() { return std::make_shared<ComplicatedPolygon>(
    Id(),Quadtree(),Info(),Tags(),parts/*,outers,inners*/,zorder,layer,/*area,*/bounds,MinZoom()); }

bbox ComplicatedPolygon::Bounds() const { return bounds; }


std::string ComplicatedPolygon::Wkb(bool transform, bool srid) const {
    if (parts.size()==0) {
        return make_multi_wkb(7, {}, transform, srid);
    }
    if (parts.size()==1) {
        return polygon_part_wkb(parts[0], transform, srid);
    }
    
    std::vector<std::string> pp;
    for (const auto& p: parts) {
        pp.push_back(polygon_part_wkb(p, transform, false));
    }
    return make_multi_wkb(6, pp, transform, srid);
}


std::string polygon_part_wkb(const PolygonPart& part, bool transform, bool srid) {
    
    size_t sz=9;
    if (srid) { sz+=4; }
    size_t outer_len = ringpart_numpoints(part.outer);

    sz += 4+16*outer_len;
    std::vector<size_t> inners_lens;

    for (const auto& inner : part.inners) {
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

    pos = write_uint32(res,pos, 1+part.inners.size());

    pos = write_ringpart_ring(res, pos, part.outer, outer_len,transform);
    if (!part.inners.empty()) {
        for (size_t i=0; i < part.inners.size(); i++) {
            pos = write_ringpart_ring(res,pos,part.inners[i],inners_lens[i],transform);
        }
    }
    return res;
}

            
    


std::list<PbfTag> ComplicatedPolygon::pack_extras() const {
    

    std::list<PbfTag> extras;
    extras.push_back(PbfTag{12,zig_zag(zorder.value_or(0)),""});
    extras.push_back(PbfTag{16,zig_zag(to_int(Area()*100)),""});

    //extras.push_back(PbfTag{17,0,pack_ring(outers)});
    //for (const auto& ii : inners) {
    //    extras.push_back(PbfTag{18,0,pack_ring(ii)});
    //}
    //extras.push_back(PbfTag{19,zig_zag(part),""});
    if (MinZoom()) {
        extras.push_back(PbfTag{22,uint64(*MinZoom()),""});
    }
    if (layer) {
        extras.push_back(PbfTag{24,zig_zag(*layer),""});
    }
    for (const auto& part: parts) {
        extras.push_back(PbfTag{25, 0, pack_polygon_part(part)});
    }
    
    return extras;
}

}
}
