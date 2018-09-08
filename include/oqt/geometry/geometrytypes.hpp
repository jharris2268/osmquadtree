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

#ifndef GEOMETRYTYPES_HPP
#define GEOMETRYTYPES_HPP

#include "oqt/geometry/addwaynodes.hpp"



#include "oqt/elements/geometry.hpp"
#include <set>

namespace oqt {
namespace geometry {


double calc_line_length(const lonlatvec& ll);
double calc_ring_area(const lonlatvec& ll);
bbox lonlats_bounds(const lonlatvec& llv);


class point : public basegeometry {
    public:
        point(std::shared_ptr<node> nd) :
            basegeometry(elementtype::Point,changetype::Normal,nd->Id(),nd->Quadtree(),nd->Info(),nd->Tags(),-1),
            lon(nd->Lon()), lat(nd->Lat()) {}
        point(std::shared_ptr<node> nd, const tagvector& tgs, int64 layer_, int64 minzoom_) :
            basegeometry(elementtype::Point,changetype::Normal,nd->Id(),nd->Quadtree(),nd->Info(),tgs,minzoom_),
            lon(nd->Lon()), lat(nd->Lat()),layer(layer_) {}

        point(int64 id, int64 qt, const info& inf, const tagvector& tags, int64 lon_, int64 lat_, int64 layer_, int64 minzoom_) :
            basegeometry(elementtype::Point,changetype::Normal,id,qt,inf,tags,minzoom_), lon(lon_), lat(lat_),layer(layer_){}

        virtual ~point() {}

        virtual elementtype OriginalType() const { return Node; }

        lonlat LonLat() const { return lonlat{lon,lat}; };
        int64 Layer() const { return layer; }

        virtual std::shared_ptr<element> copy() {
            return std::make_shared<point>(//*this);
                Id(),Quadtree(),Info(),Tags(),lon,lat,layer,MinZoom());
        }

        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bbox{lon,lat,lon,lat}; }

        virtual std::string Wkb(bool transform, bool srid) const;

        

    private:
       //point(const point& p) : element(p), lon(p.lon), lat(p.lat) {}
        int64 lon, lat;
        int64 layer;
        
};

class linestring : public basegeometry {
    public:
        linestring(std::shared_ptr<way_withnodes> wy) :
            basegeometry(elementtype::Linestring, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags(),-1),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(0), layer(0), bounds(wy->Bounds()) {
                length=calc_line_length(lonlats);
            }

        linestring(std::shared_ptr<way_withnodes> wy, const tagvector& tgs, int64 zorder_, int64 layer_, int64 minzoom_) :
            basegeometry(elementtype::Linestring, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), tgs,minzoom_),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(zorder_), layer(layer_), bounds(wy->Bounds()){
                length=calc_line_length(lonlats);
            }
        linestring(int64 id, int64 qt, const info& inf, const tagvector& tags, const refvector& refs_, const lonlatvec& lonlats_, int64 zorder_, int64 layer_, double length_, const bbox& bounds_, int64 minzoom_) :
            basegeometry(elementtype::Linestring,changetype::Normal,id,qt,inf,tags,minzoom_), refs(refs_), lonlats(lonlats_), zorder(zorder_), layer(layer_), length(length_), bounds(bounds_) {}


        virtual ~linestring() {}


        virtual elementtype OriginalType() const { return Way; }
        const refvector& Refs() const { return refs; }
        const lonlatvec& LonLats() const { return lonlats; }
        double Length() const { return length; }
        int64 ZOrder() const { return zorder; }
        int64 Layer() const { return layer; }
        virtual std::shared_ptr<element> copy() { return std::make_shared<linestring>(//*this); }
            Id(),Quadtree(),Info(),Tags(),refs,lonlats,zorder,layer,length,bounds,MinZoom()); }
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bounds; }

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        //linestring(const linestring& l) : element(l), refs(l.refs), lonlats(l.lonlats), zorder(l.zorder), length(l.length), bounds(l.bounds) {}
        refvector refs;
        lonlatvec lonlats;
        int64 zorder;
        int64 layer;
        double length;
        bbox bounds;
        
};
class simplepolygon : public basegeometry {
    public:
        simplepolygon(std::shared_ptr<way_withnodes> wy) :
            basegeometry(elementtype::SimplePolygon, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags(),-1),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(0), layer(0), bounds(wy->Bounds()), reversed(false){
                area = calc_ring_area(lonlats);
                if (area<0) {
                    area *= -1;
                    reversed=true;
                }
            }

        simplepolygon(std::shared_ptr<way_withnodes> wy, const tagvector& tgs, int64 zorder_, int64 layer_, int64 minzoom_) :
            basegeometry(elementtype::SimplePolygon, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), tgs,minzoom_),
            refs(wy->Refs()), lonlats(wy->LonLats()), zorder(zorder_), layer(layer_), bounds(wy->Bounds()), reversed(false) {
                area = calc_ring_area(lonlats);
                if (area<0) {
                    area *= -1;
                    reversed=true;
                }
            }

        simplepolygon(int64 id, int64 qt, const info& inf, const tagvector& tags, const refvector& refs_, const lonlatvec& lonlats_, int64 zorder_, int64 layer_, double area_, const bbox& bounds_, int64 minzoom_, bool reversed_) :
            basegeometry(elementtype::SimplePolygon,changetype::Normal,id,qt,inf,tags,minzoom_),
            refs(refs_), lonlats(lonlats_), zorder(zorder_), layer(layer_), area(area_), bounds(bounds_),reversed(reversed_) {}


        virtual ~simplepolygon() {}


        virtual elementtype OriginalType() const { return Way; }
        const refvector& Refs() const { return refs; }
        const lonlatvec& LonLats() const { return lonlats; }
        bool Reversed() const { return reversed; }
        
        int64 ZOrder() const { return zorder; }
        int64 Layer() const { return layer; }
        double Area() const { return area; }

        virtual std::shared_ptr<element> copy() { return std::make_shared<simplepolygon>(//*this); }
            Id(),Quadtree(),Info(),Tags(),refs,lonlats,zorder,layer,area,bounds,MinZoom(),reversed); }
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bounds; }

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        //simplepolygon(const simplepolygon& l) : element(l), refs(l.refs), lonlats(l.lonlats), zorder(l.zorder), area(l.area), bounds(l.bounds) {}
        refvector refs;
        lonlatvec lonlats;
        int64 zorder;
        int64 layer;
        double area;
        bbox bounds;
        bool reversed;
        
};

struct ringpart {
    ringpart(int64 oi, const refvector& rv, const lonlatvec& lv, bool rr) : orig_id(oi),refs(rv),lonlats(lv),reversed(rr) {}
    int64 orig_id;
    refvector refs;
    lonlatvec lonlats;
    bool reversed;
};
typedef std::vector<ringpart> ringpartvec;
double calc_ring_area(const ringpartvec& ring);

lonlatvec ringpart_lonlats(const ringpartvec& ring);
refvector ringpart_refs(const ringpartvec& ring);

void reverse_ring(ringpartvec& ring);

class complicatedpolygon : public basegeometry {
    public:
        complicatedpolygon(std::shared_ptr<relation> rel, int64 part_, const ringpartvec& outers_, const std::vector<ringpartvec>& inners_, const tagvector& tags, int64 zorder_, int64 layer_, int64 minzoom_) :
            basegeometry(elementtype::ComplicatedPolygon, changetype::Normal, rel->Id(), rel->Quadtree(), rel->Info(), tags,minzoom_),
            part(part_), outers(outers_), inners(inners_),zorder(zorder_), layer(layer_) {

            area = calc_ring_area(outers);
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
            for (const auto& o : outers) {
                for (const auto& ll : o.lonlats) {
                    bounds.expand_point(ll.lon,ll.lat);
                }
            }

        }
        complicatedpolygon(int64 id, int64 qt, const info& inf, const tagvector& tags, int64 part_, const ringpartvec& outers_, const std::vector<ringpartvec>& inners_, int64 zorder_, int64 layer_, double area_, const bbox& bounds_, int64 minzoom_)
            : basegeometry(elementtype::ComplicatedPolygon,changetype::Normal,id,qt,inf,tags,minzoom_), part(part_), outers(outers_), inners(inners_),zorder(zorder_), layer(layer_), area(area_), bounds(bounds_){}


        uint64 InternalId() const {
            return (6ull<<61) | (((uint64) Id()) << 16) | ((uint64) part);
        }   
        virtual ~complicatedpolygon() {}

        virtual elementtype OriginalType() const { return Relation; }
        const ringpartvec& Outers() const { return outers; }
        const std::vector<ringpartvec>& Inners() const { return inners; }
        int64 ZOrder() const { return zorder; }
        int64 Layer() const { return layer; }
        double Area() const { return area; }
        int64 Part() const { return part; }
        virtual std::shared_ptr<element> copy() { return std::make_shared<complicatedpolygon>(
            Id(),Quadtree(),Info(),Tags(),part,outers,inners,zorder,layer,area,bounds,MinZoom()); }
        virtual std::list<PbfTag> pack_extras() const;
        virtual bbox Bounds() const { return bounds; }

        virtual std::string Wkb(bool transform, bool srid) const;
        
    private:
        //complicatedpolygon(const complicatedpolygon& p) :
        //    element(p), part(p.part), outers(p.outers), inners(p.inners), area(p.area),bounds(p.bounds) {}
        int64 part;
        ringpartvec outers;
        std::vector<ringpartvec> inners;
        int64 zorder;
        int64 layer;
        double area;
        bbox bounds;
        
};

typedef std::shared_ptr<primitiveblock> primblock_ptr;
typedef std::vector<primblock_ptr> primblock_vec;

class BlockHandler  {
    public:
        virtual primblock_vec process(primblock_ptr)=0;
        virtual primblock_vec finish() {return {}; };
        virtual ~BlockHandler() {};
};

/*
void process_all(std::vector<std::shared_ptr<single_queue<primitiveblock>>> in,
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> out,
    std::shared_ptr<BlockHandler> handler);
*/
std::string get_tag(std::shared_ptr<element>, const std::string&);


std::shared_ptr<element> readGeometry(elementtype ty, const std::string& data, const std::vector<std::string>& stringtable, uint64 ct);
std::shared_ptr<element> unpack_geometry(elementtype ty, int64 id, int64 ct, int64 qt, const std::string& d);

std::shared_ptr<element> unpack_geometry_element(std::shared_ptr<element> geom);
size_t unpack_geometry_primitiveblock(primblock_ptr pb);

/*
void process_all_vec(std::vector<std::shared_ptr<single_queue<primitiveblock>>> in,
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> out,
    std::vector<std::shared_ptr<BlockHandler>> handlers);
*/



std::string pack_tags(const tagvector& tgs);
void unpack_tags(const std::string& str, tagvector& tgs);
std::string convert_packed_tags_to_json(const std::string& str);
} }

#endif
