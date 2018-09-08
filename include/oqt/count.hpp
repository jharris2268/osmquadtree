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
namespace oqt {
class count_objs {

    public:
        int64 min_id, max_id, num_objects, min_timestamp, max_timestamp;

        count_objs() : min_id(0), max_id(0), num_objects(0),
            min_timestamp(0), max_timestamp(0) {}

        int64 len() const { return num_objects; }        
        
        template<class T>
        void add (const T& obj) {

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

        void expand(const count_objs& other) {
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

class count_nodes : public count_objs {


    public:
        int64 minlon, minlat, maxlon, maxlat;

        count_nodes() : count_objs(),
            minlon(1800000000), minlat(900000000),
            maxlon(-1800000000), maxlat(-900000000) {}

        template<class T>
        void add (const T& obj) {
            count_objs::add(obj);
            if (obj.lon < minlon) {
                minlon=obj.lon;
            }
            if (obj.lon > maxlon) {
                maxlon=obj.lon;
            }

            if (obj.lat < minlat) {
                minlat=obj.lat;
            }
            if (obj.lat > maxlat) {
                maxlat=obj.lat;
            }
        }

        void expand(const count_nodes& other) {
            count_objs::expand(other);
            if (other.minlon < minlon) {
                minlon=other.minlon;
            }
            if (other.maxlon > maxlon) {
                maxlon=other.maxlon;
            }

            if (other.minlat < minlat) {
                minlat=other.minlat;
            }
            if (other.maxlat > maxlat) {
                maxlat=other.maxlat;
            }
        }

        std::string str() const {
            std::stringstream ss;
            ss << count_objs::str();
            ss << " {" << minlon << ", " << minlat << ", ";
            ss <<         maxlon << ", " << maxlat << "}";
            return ss.str();
        }
};

class count_ways : public count_objs {

    public:
        int64 num_refs, min_ref, max_ref, max_num_refs;

        count_ways() : count_objs(),
            num_refs(0), min_ref(0),
            max_ref(0), max_num_refs(0) {}

        template<class T>
        void add (const T& obj) {
            count_objs::add(obj);

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

        void expand(const count_ways& other) {
            count_objs::expand(other);
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
            ss << count_objs::str();
            ss << " {" << num_refs << " refs, " << min_ref << " to ";
            ss <<         max_ref << ". Longest: " << max_num_refs << "}";
            return ss.str();
        }
};
class count_relations : public count_objs {

    public:
        int64 num_nodes, num_ways, num_rels, num_empties, max_len;

        count_relations() : count_objs(),
            num_nodes(0), num_ways(0), num_rels(0),
            num_empties(0), max_len(0) {}

        template<class T>
        void add (const T& obj) {
            count_objs::add(obj);


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

        void expand(const count_relations& other) {
            count_objs::expand(other);
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
            ss << count_objs::str();
            ss << " {" << num_nodes << " N, " << num_ways << " W, ";
            ss <<         num_rels << " R. Longest: " << max_len << ", ";
            ss <<         num_empties << " empties}";
            return ss.str();
        }
};



struct count_tile {
    size_t idx;
    int64 quadtree, len;
    size_t num_nodes, num_ways, num_relations, num_geometries;
};

template <class T>
std::string change_str(const std::vector<T>& oo) {
    return std::string("\n")
        + "  Delete:    " + oo.at(1).str() + "\n"
        + "  Remove:    " + oo.at(2).str() + "\n"
        + "  Unchanged: " + oo.at(3).str() + "\n"
        + "  Modify:    " + oo.at(4).str() + "\n"
        + "  Create:    " + oo.at(5).str() + "\n";
}

template <class T>
int64 len_sum(const std::vector<T>& oo) {
    int64 r=0;
    for (const auto& o: oo) { r+=o.len(); }
    return r;
}

struct count_block {
    int index;
    bool storetiles;
    bool geom;
    bool change;
    
    int64 numblocks;
    double progress;
    int64 uncomp;
    
    size_t total;
    std::vector<count_nodes> nn;
    std::vector<count_ways> ww;
    std::vector<count_relations> rr;
    std::vector<count_objs> gg;
    
    std::vector<count_tile> tiles;

    count_block(int index_, bool tiles_, bool geom_, bool change_) : index(index_),storetiles(tiles_), geom(geom_), change(change_), numblocks(0),progress(0),uncomp(0),total(0) {
        if (geom) {
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
        }
            
    }

    void add(size_t i, std::shared_ptr<minimalblock> block) {
        numblocks++;
        if (block->file_progress>progress) { progress=block->file_progress; }
        uncomp+=block->uncompressed_size;

        for (const auto& nd:  block->nodes) {
            nn.at(nd.changetype).add(nd);
            total++;
        }
        for (const auto& wy: block->ways) {
            ww.at(wy.changetype).add(wy);
            total++;
        }
        for (const auto& rl: block->relations) {
            rr.at(rl.changetype).add(rl);
            total++;
        }
        
        if (geom) {
            for (const auto& gm: block->geometries) {
                
                if ((gm.ty<3) || (gm.ty>=7)) { 
                    logger_message() << "wrong element type?? " << gm.ty << " " << gm.id;
                    throw std::domain_error("wrong element type??");
                }
                gg.at(gm.ty-3).add<minimalgeometry>(gm);
                total++;
            }
        }
        
        if (storetiles) {
            tiles.push_back(count_tile{i, block->quadtree, block->file_position, block->nodes.size(), block->ways.size(), block->relations.size()});
        }
    }

    void expand(const count_block& other) {
        numblocks+=other.numblocks;
        total += other.total;
        if (other.progress>progress) { progress=other.progress; }
        uncomp+=other.uncomp;
        for (size_t i=0; i < (change ? 6 : 1); i++) {
            nn.at(i).expand(other.nn.at(i));
            ww.at(i).expand(other.ww.at(i));
            rr.at(i).expand(other.rr.at(i));
        }
        if (geom) {
            for (size_t i=0; i < 4; i++) {
                gg.at(i).expand(other.gg.at(i));
            }
        }
        if (storetiles) {
            std::copy(other.tiles.begin(), other.tiles.end(), std::back_inserter(tiles));
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
        ss << "nodes: " << (change ? change_str(nn) : nn.at(0).str()) << "\n";
        ss << "ways:  " << (change ? change_str(ww) : ww.at(0).str()) << "\n";
        ss << "rels:  " << (change ? change_str(rr) : rr.at(0).str());
        if (geom) {
            ss << "\n";
            ss << "points:            " << gg.at(0).str() << "\n";
            ss << "linestrings:       " << gg.at(1).str() << "\n";
            ss << "simple polygons:   " << gg.at(2).str() << "\n";
            ss << "complicated polys: " << gg.at(3).str();
        }
            
        return ss.str();
    }
};

count_block run_count(const std::string& fn, size_t numchan, bool tiles, bool geom, size_t objflags);
}
#endif //COUNT_HPP
