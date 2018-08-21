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

#include "oqt/geometry/geometrytypes.hpp"
#include <algorithm>

#include <picojson.h>

namespace oqt {
namespace geometry {

    
static uint32_t web_merc_epsg = 3857;
static uint32_t lat_long_epsg = 4326;
    

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
        r.expand_point(p.lon,p.lat);
    }
    return r;
}

lonlatvec ringpart_lonlats(const ringpartvec& ring) {
    size_t np=1;
    for (const auto& r : ring) { np += (r.lonlats.size()-1); };
    lonlatvec ll = ring[0].lonlats;
    ll.reserve(np);

    if (ring[0].reversed) {
        std::reverse(ll.begin(),ll.end());
    }

    for (size_t i=1; i < ring.size(); i++) {
        //size_t cl = ll.size()-1;
        ll.pop_back();
        if (ring[i].reversed) {
            std::copy(ring[i].lonlats.rbegin(),ring[i].lonlats.rend(), std::back_inserter(ll));
        } else {
            std::copy(ring[i].lonlats.begin(),ring[i].lonlats.end(), std::back_inserter(ll));
        }
    }
    return ll;
}

refvector ringpart_refs(const ringpartvec& ring) {
    size_t np=1;
    for (const auto& r : ring) { np += (r.refs.size()-1); };

    refvector ll = ring[0].refs;
    ll.reserve(np);

    if (ring[0].reversed) {
        std::reverse(ll.begin(),ll.end());
    }

    for (size_t i=1; i < ring.size(); i++) {
        //size_t cl = ll.size()-1;
        ll.pop_back();
        if (ring[i].reversed) {
            std::copy(ring[i].refs.rbegin(),ring[i].refs.rend(), std::back_inserter(ll));
        } else {
            std::copy(ring[i].refs.begin(),ring[i].refs.end(), std::back_inserter(ll));
        }
    }
    return ll;
}

void reverse_ring(ringpartvec& ring) {
    for (auto& rp: ring) {
        rp.reversed = !rp.reversed;
    }
    std::reverse(ring.begin(), ring.end());
}


double calc_ring_area(const ringpartvec& ring) {
    if (ring.empty()) { return 0; }
    if (ring.size()==1) {
        return calc_ring_area(ring[0].lonlats);
    }
    auto ll = ringpart_lonlats(ring);
    return calc_ring_area(ll);
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

std::string point::Wkb(bool transform, bool srid) const {
    std::string res(srid ? 25 : 21,'\0');
    res[4]='\1';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,(transform ? web_merc_epsg : lat_long_epsg));
    }
    write_point(res, pos, LonLat(), transform);
    return res;
}

std::list<PbfTag> point::pack_extras() const {
    
    std::list<PbfTag> extras{PbfTag{8,zigZag(lat),""}, PbfTag{9,zigZag(lon),""}};    
    if (MinZoom()>=0) {
        extras.push_back(PbfTag{22,uint64(MinZoom()),""});
    }
    if (layer != 0) {
        extras.push_back(PbfTag{24,zigZag(layer),""});
    }
    return extras;
    
}

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


int64 getlon(const lonlat& l) { return l.lon; }
int64 getlat(const lonlat& l) { return l.lat; }

std::string pack_bounds(const bbox& bounds) {
    std::list<PbfTag> mm;
    mm.push_back(PbfTag{1,zigZag(bounds.minx),""});
    mm.push_back(PbfTag{2,zigZag(bounds.miny),""});
    mm.push_back(PbfTag{3,zigZag(bounds.maxx-bounds.minx),""});
    mm.push_back(PbfTag{3,zigZag(bounds.maxy-bounds.miny),""});
    return packPbfTags(mm);
}

bbox unpack_bounds(const std::string& data) {
    bbox bounds;
    int64 w=0,h=0;
    size_t pos=0;
    for (auto t=readPbfTag(data,pos); t.tag>0; t=readPbfTag(data,pos)) {
        if (t.tag==1) { bounds.minx=unZigZag(t.value); }
        if (t.tag==2) { bounds.miny=unZigZag(t.value); }
        if (t.tag==3) { w=unZigZag(t.value); }
        if (t.tag==4) { h=unZigZag(t.value); }
    }
    bounds.maxx = bounds.minx+w;
    bounds.maxy = bounds.miny+h;
    return bounds;
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

std::string linestring::Wkb(bool transform, bool srid) const {

    std::string res((srid?13:9)+16*lonlats.size(),'\0');
    //res[0]='\1';
    res[4]='\2';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,(transform ? web_merc_epsg : lat_long_epsg));
    }
    write_ring(res,pos,lonlats, transform);
    return res;
}
int64 to_int(double v) {
    if (v<0) {
        return (int64) (v-0.5);
    }
    return (int64) (v+0.5);
}
std::list<PbfTag> linestring::pack_extras() const {
    std::list<PbfTag> extras;
    
    extras.push_back(PbfTag{8,0,writePackedDelta(refs)}); //refs
    extras.push_back(PbfTag{12,zigZag(zorder),""});
    extras.push_back(PbfTag{13,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lon; })}); //lons
    extras.push_back(PbfTag{14,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lat; })}); //lats
    extras.push_back(PbfTag{15,zigZag(to_int(length*100)),""});
    if (MinZoom()>=0) {
        extras.push_back(PbfTag{22,uint64(MinZoom()),""});
    }
    if (layer!=0) {
        extras.push_back(PbfTag{24,zigZag(layer),""});
    }
    return extras;
}

std::string simplepolygon::Wkb(bool transform, bool srid) const {

    std::string res((srid?17:13)+16*lonlats.size(),'\0');
    //res[0]='\1';
    res[4]='\3';
    size_t pos=5;
    if (srid) {
        res[1]=' ';
        pos = write_uint32(res,pos,(transform ? web_merc_epsg: lat_long_epsg));
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


std::list<PbfTag> simplepolygon::pack_extras() const {
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

std::string pack_ringpart(const ringpart& rp) {
    std::string refsp = writePackedDelta(rp.refs);
    std::string lonsp = writePackedDeltaFunc<lonlat>(rp.lonlats, getlon);
    std::string latsp = writePackedDeltaFunc<lonlat>(rp.lonlats, getlat);
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




std::string pack_ringparts(const std::vector<ringpart>& rps) {
    std::list<PbfTag> rr;
    for (const auto& rp : rps) {
        rr.push_back(PbfTag{1,0,pack_ringpart(rp)});
    }
    return packPbfTags(rr);
}

size_t ringpart_numpoints(const ringpartvec& rpv) {
    size_t r=1;
    for (const auto& rp : rpv) {
        r += (rp.refs.size()-1);
    }
    return r;
}

size_t write_ringpart_ring(std::string& data, size_t pos, const ringpartvec& rpv, size_t r, bool transform) {
    pos = write_uint32(data, pos, r);
    bool first=true;
    for (const auto& rp : rpv) {
        size_t rps=rp.lonlats.size();
        for (size_t i=(first?0:1); i < rps; i++) {
            pos=write_point(data,pos,rp.lonlats[rp.reversed ? (rps-i-1) : i], transform);
        }
        first=false;
    }
    return pos;
}




std::string complicatedpolygon::Wkb(bool transform, bool srid) const {
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
        pos = write_uint32(res,pos,(transform ? web_merc_epsg : lat_long_epsg));
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


std::list<PbfTag> complicatedpolygon::pack_extras() const {
    

    std::list<PbfTag> extras;
    extras.push_back(PbfTag{12,zigZag(zorder),""});
    extras.push_back(PbfTag{16,zigZag(to_int(area*100)),""});

    extras.push_back(PbfTag{17,0,pack_ringparts(outers)});
    for (const auto& ii : inners) {
        extras.push_back(PbfTag{18,0,pack_ringparts(ii)});
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

void process_all(std::vector<std::shared_ptr<single_queue<primitiveblock>>> in,
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> out,
    std::shared_ptr<BlockHandler> handler) {

    size_t in_idx=0;
    size_t out_idx=0;

    for (auto bl = in[in_idx%in.size()]->wait_and_pop(); bl; bl=in[in_idx%in.size()]->wait_and_pop()) {
        for (auto rr : handler->process(bl)) {
            out[out_idx % out.size()]->wait_and_push(rr);
            out_idx++;
        }
        in_idx++;
    }
    for (auto rr : handler->finish()) {
        out[out_idx % out.size()]->wait_and_push(rr);
        out_idx++;
    }

    for (auto o : out) {
        o->wait_and_finish();
    }
}

std::string get_tag(std::shared_ptr<element> e, const std::string& k) {
    for (const auto& t : e->Tags()) {
        if (t.key==k) {
            return t.val;
        }
    }
    return "";
}



void read_lonlats_lons(lonlatvec& lonlats, const std::string& data) {
    auto lons = readPackedDelta(data);
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
    auto lats = readPackedDelta(data);
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

ringpart unpack_ringpart(const std::string& data) {
    size_t pos=0;
    ringpart res{0,{},{},false};
    for (auto tag=readPbfTag(data,pos); tag.tag>0; tag=readPbfTag(data,pos)) {
        if (tag.tag==1) { res.orig_id = tag.value; }
        if (tag.tag==2) { res.refs = readPackedDelta(tag.data); }
        if (tag.tag==3) { read_lonlats_lons(res.lonlats, tag.data); }
        if (tag.tag==4) { read_lonlats_lats(res.lonlats, tag.data); }
        if (tag.tag==5) { res.reversed=(tag.value==1); }

    }
    return res;
}


void expand_bbox(bbox& bx, const lonlatvec& llv) {
    for (const auto& l : llv) {
        bx.expand_point(l.lon,l.lat);
    }
}

ringpartvec unpack_ringpart_vec(const std::string& data) {
    ringpartvec res;
    size_t pos=0;
    for (auto tag=readPbfTag(data,pos); tag.tag>0; tag=readPbfTag(data,pos)) {
        if (tag.tag==1) { res.push_back(unpack_ringpart(tag.data)); }
    }
    return res;
}

std::shared_ptr<element> readGeometry_int(elementtype ty, int64 id, info inf, const tagvector& tgs, int64 qt, const std::list<PbfTag>& pbftags, uint64 ct) {
        
    if (ty==Point) {
        int64 lon=0, lat=0, minzoom=-1; int64 layer=0;
        for (const auto& t : pbftags) {
            if (t.tag==8) { lat=unZigZag(t.value); }
            if (t.tag==9) { lon=unZigZag(t.value); }
            if (t.tag==22) { minzoom = (int64) t.value; }
            if (t.tag==24) { layer = unZigZag(t.value); }
        }
        return std::make_shared<point>(id,qt,inf,tgs,lon,lat,layer,minzoom);
    } else if ((ty==Linestring) || (ty==SimplePolygon)) {
        refvector rfs;
        lonlatvec lonlats;
        int64 minzoom=-1;
        int64 zorder=0; int64 layer=0; double length=0, area=0;
        bool rev=false;

        for (const auto& t : pbftags) {
            if (t.tag==8) { rfs = readPackedDelta(t.data);}
            if (t.tag==12) { zorder=unZigZag(t.value); }
            if (t.tag==13) { read_lonlats_lons(lonlats,t.data); }
            if (t.tag==14) { read_lonlats_lats(lonlats,t.data); }
            if (t.tag==15) { length = unZigZag(t.value)*0.01; }
            if (t.tag==16) { area = unZigZag(t.value)*0.01; }
            if (t.tag==22) { minzoom = (int64) t.value; }
            if (t.tag==23) { rev = t.value==1; }
            if (t.tag==24) { layer=unZigZag(t.value); }
            //if (t.tag==20) { bounds=unpack_bounds(t.data); }
        }
        bbox bounds;
        expand_bbox(bounds,lonlats);

        if (ty==Linestring) {
            return std::make_shared<linestring>(id,qt,inf,tgs,rfs, lonlats, zorder, layer,length,bounds,minzoom);
        } else if (ty==SimplePolygon) {
            return std::make_shared<simplepolygon>(id,qt,inf,tgs,rfs, lonlats, zorder, layer,area,bounds,minzoom,rev);
        }
    } else if ((ty==ComplicatedPolygon)) {
        int64 minzoom=-1;
        int64 zorder=0, layer=0, part=0; double area=0;
        ringpartvec outer;
        std::vector<ringpartvec> inners;

        for (const auto& t : pbftags) {
            if (t.tag==12) { zorder=unZigZag(t.value); }
            if (t.tag==16) { area=unZigZag(t.value)*0.01; }
            if (t.tag==17) {
                if (!outer.empty()) {
                    throw std::domain_error("multiple outers??");
                }
                outer = unpack_ringpart_vec(t.data);

            }
            if (t.tag==18) { inners.push_back(unpack_ringpart_vec(t.data)); }
            if (t.tag==19) { part = unZigZag(t.value); }
            if (t.tag==22) { minzoom = (int64) t.value; }
            if (t.tag==24) { layer=unZigZag(t.value); }
            //if (t.tag==20) { bounds=unpack_bounds(t.data); }
        }
        bbox bounds;
        for (const auto& rp : outer) {
            expand_bbox(bounds,rp.lonlats);
        }


        return std::make_shared<complicatedpolygon>(id,qt,inf,tgs,part,outer,inners,zorder,layer,area,bounds,minzoom);
    } else if ((ty==WayWithNodes)) {
        refvector refs;
        lonlatvec lonlats;
        for (const auto& t: pbftags) {
            if (t.tag==8) { refs=readPackedDelta(t.data); }
            if (t.tag==13) { read_lonlats_lons(lonlats,t.data); }
            if (t.tag==14) { read_lonlats_lats(lonlats,t.data); }
        }
        bbox bounds;
        for (const auto& l : lonlats) { bounds.expand_point(l.lon,l.lat); }
        return std::make_shared<way_withnodes>(id,qt,inf,tgs,refs,lonlats,bounds);
    }
    return std::shared_ptr<element>();
}

std::shared_ptr<element> readGeometry(elementtype ty, const std::string& data, const std::vector<std::string>& st, uint64 ct) {
    int64 id, qt;
    info inf; tagvector tgs;
    std::list<PbfTag> pbftags;
    std::tie(id,inf,tgs,qt,pbftags)=readCommon(ty,data,st,std::shared_ptr<idset>());
    
    return readGeometry_int(ty,id,inf,tgs,qt,pbftags,ct);
}

std::shared_ptr<element> unpack_geometry(elementtype ty, int64 id, int64 ct, int64 qt, const std::string& d) {
    info inf; tagvector tv; std::list<PbfTag> mm;
    std::tie(inf,tv,mm) = unpack_packedobj_common(d);
    return readGeometry_int(ty, id, inf, tv, qt, mm, ct);
}

std::shared_ptr<element> unpack_geometry_element(std::shared_ptr<element> geom) {
    return readGeometry_int(geom->Type(), geom->Id(), geom->Info(), geom->Tags(), geom->Quadtree(), geom->pack_extras(), geom->ChangeType());
}

size_t unpack_geometry_primitiveblock(primblock_ptr pb) {
    size_t cnt=0;
    for (size_t i=0; i<pb->size(); i++) {
        auto o = pb->at(i);
        if (isGeometryType(o->Type())) {
            auto gp = std::dynamic_pointer_cast<basegeometry>(o);
            if (!gp) { throw std::domain_error("cast to geometry failed??"); }
            if (gp->OriginalType()==Unknown) {
                auto g = unpack_geometry_element(gp);
                if (g) {
                    pb->objects.at(i) = g;
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
        logger_message() << "read_str pos=" << pos << ", ln=" << ln << ", data.size()=" << data.size();
        throw std::domain_error("???"); 
    }
    
    std::string res = data.substr(pos+2, ln);
    pos += 2 + ln;
    return res;
}
    


std::string pack_tags(const tagvector& tgs) {
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
    
void unpack_tags(const std::string& data, tagvector& tgs) {
    if (data.empty()) { return; }
    if (data[0] != '\0') { throw std::domain_error("not packed tags"); }
    size_t pos=1;
    while (pos < data.size() ) {
        
        std::string k = read_str(data, pos);
        std::string v = read_str(data, pos);
        tgs.push_back(tag{std::move(k), std::move(v)});
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

