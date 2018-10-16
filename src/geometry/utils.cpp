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

#include "oqt/geometry/utils.hpp"

#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"




#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {
uint32_t epsg_code(bool transform) {
    if (transform) { return 3857; }
    return 4326;
}
    

double pythag(const xy& p, const xy& q) {
    return sqrt(pow(p.x-q.x,2)+pow(p.y-q.y,2));
}

double calc_line_length(const lonlatvec& ll) {
    if (ll.size()<2) {
        return 0;
    }

    double ans=0;
    auto prev=forward_transform(ll[0].lon,ll[0].lat);
    for (size_t i=1; i < ll.size(); i++) {
        auto curr=forward_transform(ll[i].lon,ll[i].lat);
        ans += pythag(prev,curr);
        prev=curr;
    }
    return ans;
}


double calc_ring_area(const lonlatvec& ll) {


    if (ll.size() < 3) {
        return 0;
    }
    double area=0;

    xy lastpt = forward_transform(ll[0].lon,ll[0].lat);

    for (size_t i = 0; i < ll.size(); i++) {
        size_t j = (i == (ll.size()-1)) ? 0 : i+1;
        xy nextpt = forward_transform(ll[j].lon, ll[j].lat);

        area += lastpt.x * nextpt.y;
        area -= lastpt.y * nextpt.x;
        lastpt = nextpt;
    }

    return -1.0 * area / 2.0; //want polygon exteriors to be anti-clockwise
}

bbox lonlats_bounds(const lonlatvec& llv) {
    bbox r;
    for (const auto& p : llv) {
        expand_point(r, p.lon,p.lat);
    }
    return r;
}


size_t write_point(std::string& data, size_t pos, lonlat ll, bool transform) {
    xy x;
    if (transform) {
        x = forward_transform(ll.lon, ll.lat);
    } else {
        x.x = (double) ll.lon * 0.0000001;
        x.y = (double) ll.lat * 0.0000001;
    }
    write_double(data,pos,x.x);
    write_double(data,pos+8,x.y);
    return pos+16;
}
/*
uint64 double_bytes(double f) {
    union {double sf; uint64 si;};
    sf=f;
    return si;
}

double un_double_bytes(uint64 i) {
    union {double sf; uint64 si;};
    si=i;
    return sf;
}
*/

int64 getlon(const lonlat& l) { return l.lon; }
int64 getlat(const lonlat& l) { return l.lat; }

std::string pack_bounds(const bbox& bounds) {
    std::list<PbfTag> mm;
    mm.push_back(PbfTag{1,zig_zag(bounds.minx),""});
    mm.push_back(PbfTag{2,zig_zag(bounds.miny),""});
    mm.push_back(PbfTag{3,zig_zag(bounds.maxx-bounds.minx),""});
    mm.push_back(PbfTag{3,zig_zag(bounds.maxy-bounds.miny),""});
    return pack_pbf_tags(mm);
}

bbox unpack_bounds(const std::string& data) {
    bbox bounds;
    int64 w=0,h=0;
    size_t pos=0;
    for (auto t=read_pbf_tag(data,pos); t.tag>0; t=read_pbf_tag(data,pos)) {
        if (t.tag==1) { bounds.minx=un_zig_zag(t.value); }
        if (t.tag==2) { bounds.miny=un_zig_zag(t.value); }
        if (t.tag==3) { w=un_zig_zag(t.value); }
        if (t.tag==4) { h=un_zig_zag(t.value); }
    }
    bounds.maxx = bounds.minx+w;
    bounds.maxy = bounds.miny+h;
    return bounds;
}




std::string get_tag(ElementPtr e, const std::string& k) {
    for (const auto& t : e->Tags()) {
        if (t.key==k) {
            return t.val;
        }
    }
    return "";
}

int64 to_int(double v) {
    if (v<0) {
        return (int64) (v-0.5);
    }
    return (int64) (v+0.5);
}

void read_lonlats_lons(lonlatvec& lonlats, const std::string& data) {
    auto lons = read_packed_delta(data);
    if (lonlats.empty()) {
        lonlats.resize(lons.size());
    }
    if (lonlats.size()!=lons.size()) {
        throw std::domain_error("???? lons.size()!=lonlats.size()");
    }
    for (size_t i=0; i < lons.size(); i++) {
        lonlats[i].lon=lons[i];
    }
}
void read_lonlats_lats(lonlatvec& lonlats, const std::string& data) {
    auto lats = read_packed_delta(data);
    if (lonlats.empty()) {
        lonlats.resize(lats.size());
    }
    if (lonlats.size()!=lats.size()) {
        throw std::domain_error("???? lats.size()!=lonlats.size()");
    }
    for (size_t i=0; i < lats.size(); i++) {
        lonlats[i].lat=lats[i];
    }
}

Ring::Part unpack_ringpart(const std::string& data) {
    size_t pos=0;
    Ring::Part res{0,{},{},false};
    for (auto tag=read_pbf_tag(data,pos); tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) { res.orig_id = tag.value; }
        if (tag.tag==2) { res.refs = read_packed_delta(tag.data); }
        if (tag.tag==3) { read_lonlats_lons(res.lonlats, tag.data); }
        if (tag.tag==4) { read_lonlats_lats(res.lonlats, tag.data); }
        if (tag.tag==5) { res.reversed=(tag.value==1); }

    }
    return res;
}


void expand_bbox(bbox& bx, const lonlatvec& llv) {
    for (const auto& l : llv) {
        expand_point(bx, l.lon,l.lat);
    }
}

Ring unpack_ringpart_vec(const std::string& data) {
    Ring res;
    size_t pos=0;
    for (auto tag=read_pbf_tag(data,pos); tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) { res.parts.push_back(unpack_ringpart(tag.data)); }
    }
    return res;
}

ElementPtr readGeometry_int(ElementType ty, int64 id, ElementInfo inf, const std::vector<Tag>& tgs, int64 qt, const std::list<PbfTag>& pbftags, changetype ct) {
        
    if (ty==ElementType::Point) {
        int64 lon=0, lat=0, minzoom=-1; int64 layer=0;
        for (const auto& t : pbftags) {
            if (t.tag==8) { lat=un_zig_zag(t.value); }
            if (t.tag==9) { lon=un_zig_zag(t.value); }
            if (t.tag==22) { minzoom = (int64) t.value; }
            if (t.tag==24) { layer = un_zig_zag(t.value); }
        }
        return std::make_shared<Point>(id,qt,inf,tgs,lon,lat,layer,minzoom);
    } else if ((ty==ElementType::Linestring) || (ty==ElementType::SimplePolygon)) {
        refvector rfs;
        lonlatvec lonlats;
        int64 minzoom=-1;
        int64 zorder=0; int64 layer=0; double length=0, area=0;
        bool rev=false;

        for (const auto& t : pbftags) {
            if (t.tag==8) { rfs = read_packed_delta(t.data);}
            if (t.tag==12) { zorder=un_zig_zag(t.value); }
            if (t.tag==13) { read_lonlats_lons(lonlats,t.data); }
            if (t.tag==14) { read_lonlats_lats(lonlats,t.data); }
            if (t.tag==15) { length = un_zig_zag(t.value)*0.01; }
            if (t.tag==16) { area = un_zig_zag(t.value)*0.01; }
            if (t.tag==22) { minzoom = (int64) t.value; }
            if (t.tag==23) { rev = t.value==1; }
            if (t.tag==24) { layer=un_zig_zag(t.value); }
            //if (t.tag==20) { bounds=unpack_bounds(t.data); }
        }
        bbox bounds;
        expand_bbox(bounds,lonlats);

        if (ty==ElementType::Linestring) {
            return std::make_shared<Linestring>(id,qt,inf,tgs,rfs, lonlats, zorder, layer,length,bounds,minzoom);
        } else if (ty==ElementType::SimplePolygon) {
            return std::make_shared<SimplePolygon>(id,qt,inf,tgs,rfs, lonlats, zorder, layer,area,bounds,minzoom,rev);
        }
    } else if ((ty==ElementType::ComplicatedPolygon)) {
        int64 minzoom=-1;
        int64 zorder=0, layer=0, part=0; double area=0;
        Ring outer;
        std::vector<Ring> inners;

        for (const auto& t : pbftags) {
            if (t.tag==12) { zorder=un_zig_zag(t.value); }
            if (t.tag==16) { area=un_zig_zag(t.value)*0.01; }
            if (t.tag==17) {
                if (!outer.parts.empty()) {
                    throw std::domain_error("multiple outers??");
                }
                outer = unpack_ringpart_vec(t.data);

            }
            if (t.tag==18) { inners.push_back(unpack_ringpart_vec(t.data)); }
            if (t.tag==19) { part = un_zig_zag(t.value); }
            if (t.tag==22) { minzoom = (int64) t.value; }
            if (t.tag==24) { layer=un_zig_zag(t.value); }
            //if (t.tag==20) { bounds=unpack_bounds(t.data); }
        }
        bbox bounds;
        for (const auto& rp : outer.parts) {
            expand_bbox(bounds,rp.lonlats);
        }


        return std::make_shared<ComplicatedPolygon>(id,qt,inf,tgs,part,outer,inners,zorder,layer,area,bounds,minzoom);
    } else if ((ty==ElementType::WayWithNodes)) {
        refvector refs;
        lonlatvec lonlats;
        for (const auto& t: pbftags) {
            if (t.tag==8) { refs=read_packed_delta(t.data); }
            if (t.tag==13) { read_lonlats_lons(lonlats,t.data); }
            if (t.tag==14) { read_lonlats_lats(lonlats,t.data); }
        }
        bbox bounds;
        expand_bbox(bounds, lonlats);
        return std::make_shared<WayWithNodes>(id,qt,inf,tgs,refs,lonlats,bounds);
    }
    return ElementPtr();
}

ElementPtr readGeometry(ElementType ty, const std::string& data, const std::vector<std::string>& st, changetype ct) {
    int64 id, qt;
    ElementInfo inf; std::vector<Tag> tgs;
    std::list<PbfTag> pbftags;
    std::tie(id,inf,tgs,qt,pbftags)=read_common(ty,data,st,nullptr);
    
    return readGeometry_int(ty,id,inf,tgs,qt,pbftags,ct);
}

ElementPtr unpack_geometry_element(ElementPtr ele) {
    
    auto geom = std::dynamic_pointer_cast<BaseGeometry>(ele);
    if (!geom) {
        throw std::domain_error("not a geometry?");
    }
    
    return readGeometry_int(geom->Type(), geom->Id(), geom->Info(), geom->Tags(), geom->Quadtree(), geom->pack_extras(), geom->ChangeType());
}



size_t unpack_geometry_primitiveblock(primblock_ptr pb) {
    size_t cnt=0;
    for (size_t i=0; i<pb->size(); i++) {
        auto o = pb->at(i);
        if (is_geometry_type(o->Type())) {
            auto gp = std::dynamic_pointer_cast<BaseGeometry>(o);
            if (!gp) { throw std::domain_error("cast to geometry failed??"); }
            if (gp->OriginalType()==ElementType::Unknown) {
                auto g = unpack_geometry_element(gp);
                if (g) {
                    pb->Objects().at(i) = g;
                    cnt++;
                }
            }
        }
    }
    return cnt;
}


inline size_t write_str(std::string& data, size_t pos, const std::string& str) {
    data[pos] = (unsigned char) (str.size()>>8)&255;
    data[pos+1] = (unsigned char) (str.size()&255);
    
    std::copy(str.begin(), str.end(), data.begin()+pos+2);
    return pos + 2 + str.size();
    
}

inline std::string read_str(const std::string& data, size_t& pos) {
    
    size_t ln = ((unsigned char) data[pos]) * 256;
    ln += ((unsigned char) data[pos+1]);
    if (((pos+2+ln) >data.size()) || (ln > 1000)) {
        Logger::Message() << "read_str pos=" << pos << ", ln=" << ln << ", data.size()=" << data.size();
        throw std::domain_error("???"); 
    }
    
    std::string res = data.substr(pos+2, ln);
    pos += 2 + ln;
    return res;
}
    


std::string pack_tags(const std::vector<Tag>& tgs) {
    size_t len = 1;
    for (const auto& t: tgs) {
        if ((t.key.size()>0xffff) || (t.val.size()>0xffff)) {
            throw std::domain_error("tags keys and vals must be less than 65536 bytes");
        }
        len += 4 + t.key.size() + t.val.size();
    }
    
    std::string data(len, '\0');
    size_t pos=1;
    for (const auto& t: tgs) {
        pos = write_str(data,pos,t.key);
        pos = write_str(data,pos,t.val);
    }
    if (pos != len) { throw std::domain_error("???"); }
    return data;
    
}
    
void unpack_tags(const std::string& data, std::vector<Tag>& tgs) {
    if (data.empty()) { return; }
    if (data[0] != '\0') { throw std::domain_error("not packed tags"); }
    size_t pos=1;
    while (pos < data.size() ) {
        
        std::string k = read_str(data, pos);
        std::string v = read_str(data, pos);
        tgs.push_back(Tag{std::move(k), std::move(v)});
    }
    if (pos != data.size()) { throw std::domain_error("???"); }
}
        
std::string convert_packed_tags_to_json(const std::string& data) {
    if (data.empty()) { return std::string(); }
    if (data[0] != '\0') { throw std::domain_error("not packed tags"); }
    size_t pos=1;
    
    picojson::value::object obj;
    
    while (pos < data.size() ) {
        
        std::string k = read_str(data, pos);
        std::string v = read_str(data, pos);
        
        obj[k] = picojson::value(v);
        
        
    }
    if (pos != data.size()) { throw std::domain_error("???"); }
    return picojson::value(obj).serialize();    
}



}}

