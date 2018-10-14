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

#ifndef COUNT_HPP
#define COUNT_HPP

#include "oqt/elements/minimalblock.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/varint.hpp"
#include "oqt/utils/date.hpp"
#include "oqt/elements/element.hpp"

#include <map>

namespace oqt {
class CountElement {

    public:
        int64 min_id, max_id, num_objects, min_timestamp, max_timestamp;

        CountElement() : min_id(0), max_id(0), num_objects(0),
            min_timestamp(0), max_timestamp(0) {}

        int64 len() const { return num_objects; }        
        
        
        void add (const minimal::Element& obj) {

            if ((num_objects==0) || ((int64) obj.id < min_id)) {
                min_id=obj.id;
            }
            if ((num_objects==0) || ((int64) obj.id > max_id)) {
                max_id=obj.id;
            }
            if ((num_objects==0) || (((int64)obj.timestamp) < min_timestamp)) {
                min_timestamp=obj.timestamp;
            }
            if ((num_objects==0) || (((int64)obj.timestamp) > max_timestamp)) {
                max_timestamp=obj.timestamp;
            }

            num_objects++;
        }

        void expand(const CountElement& other) {
            if (!other.num_objects) {
                return;
            }

            if ((num_objects==0) || (other.min_id < min_id)) {
                min_id=other.min_id;
            }

            if ((num_objects==0) || (other.max_id > max_id)) {
                max_id=other.max_id;
            }
            if ((num_objects==0) || (other.min_timestamp < min_timestamp)) {
                min_timestamp=other.min_timestamp;
            }
            if ((num_objects==0) || (other.max_timestamp > max_timestamp)) {
                max_timestamp=other.max_timestamp;
            }
            num_objects+=other.num_objects;
        }

        std::string str() const {
            std::stringstream ss;
            ss << num_objects << " objects: " << min_id << " => " << max_id;
            if (max_timestamp > 0) {
                ss << " [" << date_str(min_timestamp) << " => " << date_str(max_timestamp) << "]";
            }
            return ss.str();
        }
};

class CountNode : public CountElement {


    public:
        int64 min_lon, min_lat, max_lon, max_lat;

        CountNode() : CountElement(),
            min_lon(1800000000), min_lat(900000000),
            max_lon(-1800000000), max_lat(-900000000) {}

        
        void add (const minimal::Node& obj) {
            CountElement::add(obj);
            if (obj.lon < min_lon) {
                min_lon=obj.lon;
            }
            if (obj.lon > max_lon) {
                max_lon=obj.lon;
            }

            if (obj.lat < min_lat) {
                min_lat=obj.lat;
            }
            if (obj.lat > max_lat) {
                max_lat=obj.lat;
            }
        }

        void expand(const CountNode& other) {
            CountElement::expand(other);
            if (other.min_lon < min_lon) {
                min_lon=other.min_lon;
            }
            if (other.max_lon > max_lon) {
                max_lon=other.max_lon;
            }

            if (other.min_lat < min_lat) {
                min_lat=other.min_lat;
            }
            if (other.max_lat > max_lat) {
                max_lat=other.max_lat;
            }
        }

        std::string str() const {
            std::stringstream ss;
            ss << CountElement::str();
            ss << " {" << min_lon << ", " << min_lat << ", ";
            ss <<         max_lon << ", " << max_lat << "}";
            return ss.str();
        }
};

class CountWay : public CountElement {

    public:
        int64 num_refs, min_ref, max_ref, max_num_refs;

        CountWay() : CountElement(),
            num_refs(0), min_ref(0),
            max_ref(0), max_num_refs(0) {}

        
        void add (const minimal::Way& obj) {
            CountElement::add(obj);

            int64 nr=0;
            int64 curr=0;
            size_t pos=0;
            while ( pos < obj.refs_data.size()) {
                curr += readVarint(obj.refs_data,pos);
                nr++;
                if ((min_ref==0) || (curr<min_ref)) {
                    min_ref=curr;
                }
                if ((max_ref==0) || (curr > max_ref)) {
                    max_ref=curr;
                }
            }
            num_refs += nr;

            if (nr > max_num_refs) {
                max_num_refs=nr;
            }

            
        }

        void expand(const CountWay& other) {
            CountElement::expand(other);
            if (other.num_refs==0) {
                return;
            }

            num_refs+=other.num_refs;

            if (other.max_num_refs > max_num_refs) {
                max_num_refs=other.max_num_refs;
            }

            if ((min_ref==0) || (other.min_ref < min_ref)) {
                min_ref=other.min_ref;
            }
            if (other.max_ref > max_ref) {
                max_ref=other.max_ref;
            }
        }

        std::string str() const {
            std::stringstream ss;
            ss << CountElement::str();
            ss << " {" << num_refs << " refs, " << min_ref << " to ";
            ss <<         max_ref << ". Longest: " << max_num_refs << "}";
            return ss.str();
        }
};
class CountRelation : public CountElement {

    public:
        int64 num_nodes, num_ways, num_rels, num_empties, max_len;

        CountRelation() : CountElement(),
            num_nodes(0), num_ways(0), num_rels(0),
            num_empties(0), max_len(0) {}

        
        void add (const minimal::Relation& obj) {
            CountElement::add(obj);


            int64 nm=0;
            int64 curr=0;
            size_t pos=0;
            size_t pos_ty=0;
            while ( pos < obj.refs_data.size()) {
                uint64 ty=readUVarint(obj.tys_data, pos_ty);

                curr += readVarint(obj.refs_data,pos);
                nm++;
                if (ty==0) { num_nodes++; }
                if (ty==1) { num_ways++; }
                if (ty==2) { num_rels++; }


            }


            if (nm > max_len) {
                max_len=nm;
            }



        }

        void expand(const CountRelation& other) {
            CountElement::expand(other);
            num_nodes+=other.num_nodes;
            num_ways+=other.num_ways;
            num_rels+=other.num_rels;

            num_empties+=other.num_empties;

            if (other.max_len > max_len) {
                max_len=other.max_len;
            }

        }

        std::string str() const {
            std::stringstream ss;
            ss << CountElement::str();
            ss << " {" << num_nodes << " N, " << num_ways << " W, ";
            ss <<         num_rels << " R. Longest: " << max_len << ", ";
            ss <<         num_empties << " empties}";
            return ss.str();
        }
};



struct BlockSummary {
    size_t idx;
    int64 quadtree, len;
    size_t num_nodes, num_ways, num_relations, num_geometries;
};

template <class T>
std::string change_str(const std::map<changetype,T>& oo) {
    return std::string("\n")
        + "  Delete:    " + ((oo.count(changetype::Delete)>0) ? oo.at(changetype::Delete).str() : "empty")+ "\n"
        + "  Remove:    " + ((oo.count(changetype::Remove)>0) ? oo.at(changetype::Remove).str() : "empty") + "\n"
        + "  Unchanged: " + ((oo.count(changetype::Unchanged)>0) ? oo.at(changetype::Unchanged).str() : "empty") + "\n"
        + "  Modify:    " + ((oo.count(changetype::Modify)>0) ? oo.at(changetype::Modify).str() : "empty") + "\n"
        + "  Create:    " + ((oo.count(changetype::Create)>0) ? oo.at(changetype::Create).str() : "empty") + "\n";
}

template <class T>
int64 len_sum(const T& oo) {
    int64 r=0;
    for (const auto& o: oo) { r+=o.second.len(); }
    return r;
}

class Count {
    
    public:

        Count(int index_, bool tiles_, bool geom_, bool change_) : index(index_),storetiles(tiles_), geom(geom_), change(change_), numblocks(0),progress(0),uncomp(0),total(0) {
            /*if (geom) {
                gg.resize(4);
            }
            if (change) {
                nn.resize(6);
                ww.resize(6);
                rr.resize(6);
            } else {
                nn.resize(1);
                ww.resize(1);
                rr.resize(1);
            }*/
                
        }

        void add(size_t i, minimal::BlockPtr block) {
            numblocks++;
            if (block->file_progress>progress) { progress=block->file_progress; }
            uncomp+=block->uncompressed_size;

            for (const auto& nd:  block->nodes) {
                nn[(changetype) nd.changetype].add(nd);
                total++;
            }
            for (const auto& wy: block->ways) {
                ww[(changetype) wy.changetype].add(wy);
                total++;
            }
            for (const auto& rl: block->relations) {
                rr[(changetype) rl.changetype].add(rl);
                total++;
            }
            
            if (geom) {
                for (const auto& gm: block->geometries) {
                    
                    if ((gm.ty<3) || (gm.ty>=7)) { 
                        Logger::Message() << "wrong element type?? " << gm.ty << " " << gm.id;
                        throw std::domain_error("wrong element type??");
                    }
                    gg[std::make_pair((ElementType)gm.ty, changetype::Normal)].add(gm);
                    total++;
                }
            }
            
            if (storetiles) {
                tiles.push_back(BlockSummary{i, block->quadtree, block->file_position, block->nodes.size(), block->ways.size(), block->relations.size()});
            }
        }

        void expand(const Count& other) {
            auto other_summary = other.summary();
            
            numblocks+=std::get<0>(other_summary);
            if (std::get<1>(other_summary)>progress) { progress=std::get<1>(other_summary); }
            uncomp+=std::get<2>(other_summary);
            total += std::get<3>(other_summary);
            
            
            for (const auto& on: other.nodes()) {
                nn[on.first].expand(on.second);
            }
            
            for (const auto& on: other.ways()) {
                ww[on.first].expand(on.second);
            }
            
            for (const auto& on: other.relations()) {
                rr[on.first].expand(on.second);
            }
            
            for (const auto& on: other.geometries()) {
                gg[on.first].expand(on.second);
            }
            /*
            for (size_t i=0; i < (change ? 6 : 1); i++) {
                nn.at((changetype) i).expand(other.nodes().at(i));
                ww.at((changetype) i).expand(other.ways().at(i));
                rr.at((changetype) i).expand(other.relations().at(i));
            }
            if (geom) {
                for (size_t i=0; i < 4; i++) {
                    gg.at({(ElementType) i).expand(other.geometries().at(i));
                }
            }*/
            if (storetiles) {
                std::copy(other.blocks().begin(), other.blocks().end(), std::back_inserter(tiles));
            }
        }
        bool prog(int t) { return (numblocks%t)==0; }

        std::string short_str() {
            std::stringstream ss;
            ss  << "[" << index << "]: " << numblocks
            << " (" << std::setw(6) << std::fixed << std::setprecision(1) << progress
            << ", " << std::setw(8) << std::fixed << std::setprecision(1) << double(uncomp)/1024./1024.0
            << ") [" << len_sum(nn) << ", " << len_sum(ww) << ", " << len_sum(rr) << ", " << len_sum(gg) << "]";
            return ss.str();
        }

        std::string long_str() {
            std::stringstream ss;
            ss  << "[" << index << "]: " << numblocks
            << " (" << std::setw(6) << std::fixed << std::setprecision(1) << progress
            << ", " << std::setw(8) << std::fixed << std::setprecision(1) << double(uncomp)/1024./1024.0
            << ")\n";
            ss << "nodes: " << (change ? change_str(nn) : ((nn.count(changetype::Normal)>0) ? nn.at(changetype::Normal).str() : "empty")) << "\n";
            ss << "ways:  " << (change ? change_str(ww) : ((ww.count(changetype::Normal)>0) ? ww.at(changetype::Normal).str() : "empty")) << "\n";
            ss << "rels:  " << (change ? change_str(rr) : ((rr.count(changetype::Normal)>0) ? rr.at(changetype::Normal).str() : "empty"));
            if (geom) {
                ss << "\n";
                if (gg.count(std::make_pair(ElementType::Point,changetype::Normal))>0) {
                    ss << "points:            " << gg.at(std::make_pair(ElementType::Point,changetype::Normal)).str() << "\n";
                }
                if (gg.count(std::make_pair(ElementType::Linestring,changetype::Normal))>0) {
                    ss << "linestrings:       " << gg.at(std::make_pair(ElementType::Linestring,changetype::Normal)).str() << "\n";
                }
                if (gg.count(std::make_pair(ElementType::SimplePolygon,changetype::Normal))>0) {
                    ss << "simple polygons:   " << gg.at(std::make_pair(ElementType::SimplePolygon,changetype::Normal)).str() << "\n";
                }
                if (gg.count(std::make_pair(ElementType::ComplicatedPolygon,changetype::Normal))>0) {
                    ss << "complicated polys: " << gg.at(std::make_pair(ElementType::ComplicatedPolygon,changetype::Normal)).str();
                }
            }
                
            return ss.str();
        }
        
        
        const std::map<changetype, CountNode>& nodes() const { return nn; }
        const std::map<changetype, CountWay>& ways() const { return ww; }
        const std::map<changetype, CountRelation>& relations() const { return rr; }
        const std::map<std::pair<ElementType,changetype>, CountElement>& geometries() const { return gg; }
        
        const std::vector<BlockSummary>& blocks() const { return tiles; }
        
        
        std::tuple<int64,double,int64,size_t> summary() const { return std::make_tuple(numblocks,progress,uncomp,total); }
        
    private:
        int index;
        bool storetiles;
        bool geom;
        bool change;
        
        int64 numblocks;
        double progress;
        int64 uncomp;
        
        size_t total;
        std::map<changetype, CountNode> nn;
        std::map<changetype, CountWay> ww;
        std::map<changetype, CountRelation> rr;
        std::map<std::pair<ElementType, changetype>, CountElement> gg;
        
        std::vector<BlockSummary> tiles;
};

std::shared_ptr<Count> run_count(const std::string& fn, size_t numchan, bool tiles, bool geom, size_t objflags);
}
#endif //COUNT_HPP
