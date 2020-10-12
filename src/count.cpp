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

#include "oqt/elements/minimalblock.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"

#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/readblockscaller.hpp"
#include "oqt/utils/date.hpp"
#include "oqt/utils/string.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/utils/pbf/varint.hpp"

#include "oqt/count.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace oqt {
    
int64 CountElement::len() const { return num_objects; }        
        
        
void CountElement::add(ElementPtr obj) {
    if (!obj) { throw std::domain_error("??"); }
    if ((num_objects==0) || (obj->Id() < min_id)) {
        min_id=obj->Id();
    }
    if ((num_objects==0) || (obj->Id() > max_id)) {
        max_id=obj->Id();
    }
    if ((num_objects==0) || (obj->Info().timestamp < min_timestamp)) {
        min_timestamp=obj->Info().timestamp;
    }
    if ((num_objects==0) || (obj->Info().timestamp > max_timestamp)) {
        max_timestamp=obj->Info().timestamp;
    }

    num_objects++;
}

void CountElement::add (const minimal::Element& obj) {

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

void CountElement::expand(const CountElement& other) {
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

std::string CountElement::str() const {
    std::stringstream ss;
    ss << num_objects << " objects: " << min_id << " => " << max_id;
    if (max_timestamp > 0) {
        ss << " [" << date_str(min_timestamp) << " => " << date_str(max_timestamp) << "]";
    }
    return ss.str();
}



void CountNode::add(ElementPtr obj) {
    CountElement::add(obj);
    auto nd = std::dynamic_pointer_cast<Node>(obj);
    if (!nd) { throw std::domain_error("not a node??"); }
    if (nd->Lon() < min_lon) {
        min_lon=nd->Lon();
    }
    if (nd->Lon() > max_lon) {
        max_lon=nd->Lon();
    }

    if (nd->Lat() < min_lat) {
        min_lat=nd->Lat();
    }
    if (nd->Lat() > max_lat) {
        max_lat=nd->Lat();
    }
}

void CountNode::add(const minimal::Node& obj) {
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

void CountNode::expand(const CountNode& other) {
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

std::string CountNode::str() const {
    std::stringstream ss;
    ss << CountElement::str();
    ss << " {" << min_lon << ", " << min_lat << ", ";
    ss <<         max_lon << ", " << max_lat << "}";
    return ss.str();
}




void CountWay::add (ElementPtr obj) {
    CountElement::add(obj);
    auto wy = std::dynamic_pointer_cast<Way>(obj);
    if (!wy) { throw std::domain_error("not a way?"); }
    
    for (auto rf: wy->Refs()) {
        if ((min_ref==0) || (rf<min_ref)) {
            min_ref=rf;
        }
        if ((max_ref==0) || (rf > max_ref)) {
            max_ref=rf;
        }
    }
    num_refs += wy->Refs().size();

    if (((int64) wy->Refs().size()) > max_num_refs) {
        max_num_refs=wy->Refs().size();
    }

    
}


void CountWay::add (const minimal::Way& obj) {
    CountElement::add(obj);

    int64 nr=0;
    int64 curr=0;
    size_t pos=0;
    while ( pos < obj.refs_data.size()) {
        curr += read_varint(obj.refs_data,pos);
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

void CountWay::expand(const CountWay& other) {
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

std::string CountWay::str() const {
    std::stringstream ss;
    ss << CountElement::str();
    ss << " {" << num_refs << " refs, " << min_ref << " to ";
    ss <<         max_ref << ". Longest: " << max_num_refs << "}";
    return ss.str();
}




void CountRelation::add (ElementPtr obj) {
    CountElement::add(obj);

    auto rel = std::dynamic_pointer_cast<Relation>(obj);
    if (!rel) { throw std::domain_error("not a relation?"); }
    for (const auto& mem: rel->Members()) {
    
        if (mem.type==ElementType::Node) { num_nodes++; }
        if (mem.type==ElementType::Way) { num_ways++; }
        if (mem.type==ElementType::Relation) { num_rels++; }


    }
    if (rel->Members().empty()) {
        num_empties++;
    }

    if (((int64) rel->Members().size()) > max_len) {
        max_len=rel->Members().size();
    }



}


void CountRelation::add (const minimal::Relation& obj) {
    CountElement::add(obj);


    int64 nm=0;
    
    
    if (obj.refs_data.empty()) {
        num_empties++;
    } else {
        int64 curr=0;
        size_t pos=0;
        size_t pos_ty=0;
        while ( pos < obj.refs_data.size()) {
            uint64 ty=read_unsigned_varint(obj.tys_data, pos_ty);

            curr += read_varint(obj.refs_data,pos);
            nm++;
            if (ty==0) { num_nodes++; }
            if (ty==1) { num_ways++; }
            if (ty==2) { num_rels++; }


        }
    }

    if (nm > max_len) {
        max_len=nm;
    }



}

void CountRelation::expand(const CountRelation& other) {
    CountElement::expand(other);
    num_nodes+=other.num_nodes;
    num_ways+=other.num_ways;
    num_rels+=other.num_rels;

    num_empties+=other.num_empties;

    if (other.max_len > max_len) {
        max_len=other.max_len;
    }

}

std::string CountRelation::str() const {
    std::stringstream ss;
    ss << CountElement::str();
    ss << " {" << num_nodes << " N, " << num_ways << " W, ";
    ss <<         num_rels << " R. Longest: " << max_len << ", ";
    ss <<         num_empties << " empties}";
    return ss.str();
}




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

    
    
void Count::add(size_t i, minimal::BlockPtr block) {
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

void  Count::add(size_t i, PrimitiveBlockPtr block) {
    numblocks++;
    if (block->FileProgress()>progress) { progress=block->FileProgress(); }
    //uncomp+=block->UncompressedSize();

    
    size_t n=0,w=0,r=0;

    for (auto obj: block->Objects()) {
        if (obj->Type()==ElementType::Node) {
            nn[obj->ChangeType()].add(obj);
            n++;
        } else if (obj->Type()==ElementType::Way) {
            ww[obj->ChangeType()].add(obj);
            w++;
        } else if (obj->Type()==ElementType::Relation) {
            rr[obj->ChangeType()].add(obj);
            r++;
        } else if (geom) {
            gg[std::make_pair(obj->Type(), changetype::Normal)].add(obj);
        }
        total++;
    }
    
    
    
    if (storetiles) {
        tiles.push_back(BlockSummary{i, block->Quadtree(), block->FilePosition(), n,w,r});
    }
}


void  Count::expand(const Count& other) {
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
    
    if (storetiles) {
        std::copy(other.blocks().begin(), other.blocks().end(), std::back_inserter(tiles));
    }
}
bool Count::prog(int t) { return (numblocks%t)==0; }

std::string Count::short_str() {
    std::stringstream ss;
    ss  << "[" << index << "]: " << numblocks
    << " (" << std::setw(6) << std::fixed << std::setprecision(1) << progress
    << ", " << std::setw(8) << std::fixed << std::setprecision(1) << double(uncomp)/1024./1024.0
    << ") [" << len_sum(nn) << ", " << len_sum(ww) << ", " << len_sum(rr) << ", " << len_sum(gg) << "]";
    return ss.str();
}

std::string Count::long_str() {
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
    
    
    

class CountProgress {
    public:
        CountProgress(size_t step_) : step(step_), nb(0),nn(0),ww(0),rr(0),gg(0),pp(0) {
            nexttot=step/2;
        }
        
        void call(minimal::BlockPtr mb) {
            if (!mb) {
                message(true);
                return;
            }
            nb++;
            nn+=mb->nodes.size();
            ww+=mb->ways.size();
            rr+=mb->relations.size();
            gg+=mb->geometries.size();
            
            if (mb->file_progress > pp) {
                pp=mb->file_progress;
            }
            
            message(false);
        }
        
        void call(PrimitiveBlockPtr bl) {
            if (!bl) {
                message(true);
                return;
            }
            nb++;
            for (const auto& obj: bl->Objects()) {
                if (obj->Type()==ElementType::Node) {
                    nn++;
                } else if (obj->Type()==ElementType::Way) {
                    ww++;
                } else if (obj->Type()==ElementType::Relation) {
                    rr++;
                } else {
                    gg++;
                }
            
            }
            if (bl->FileProgress() > pp) {
                pp=bl->FileProgress();
            }
            message(false);
        }
        
        void message(bool atend) {
            if (atend || ((nn+ww+rr+gg)>nexttot)) {
                                
                Logger::Progress(pp) << std::setw(8) << nb
                    << " (" << std::setw(6) << std::fixed << std::setprecision(1) << pp
                    << ") [" << std::setw(12) << nn
                    << ", " << std::setw(12) << ww
                    << ", " << std::setw(12) << rr
                    << ", " << std::setw(12) << gg << "]";
                    
                nexttot += step;
            }
        }
    private:
        size_t step,nexttot;
        size_t nb,nn,ww,rr,gg;
        double pp;
};
            
    
    
std::shared_ptr<Count> run_count(std::shared_ptr<ReadBlocksCaller> rbc, bool change, size_t numchan, bool tiles, bool geom, ReadBlockFlags objflags, bool use_minimal) {
    
    
    auto& lg=Logger::Get();
    
    
    
    
    auto result = std::make_shared<Count>(0,tiles,geom,change);
    
    
    size_t step = 14000000;
    auto cp = std::make_shared<CountProgress>(step);
    
    if (use_minimal) {
        auto cb = [result,&cp](minimal::BlockPtr mb) {
            cp->call(mb);
            if (mb){
                result->add(mb->index, mb);
            }
        };
        if (numchan==0) {

            //read_blocks_nothread_minimalblock(fn, cb, {}, objflags);
            rbc->read_minimal_nothread(cb,objflags,nullptr);
        } else {
            //read_blocks_minimalblock(fn, cb, {}, numchan, objflags);
            auto cbs = multi_threaded_callback<minimal::Block>::make(cb, numchan);
            rbc->read_minimal(cbs,objflags,nullptr);
        }
        
        
    } else {
        
        
        if (numchan>0) {
            auto cp_callback = threaded_callback<PrimitiveBlock>::make(
                [cp](PrimitiveBlockPtr bl) { cp->call(bl); }, numchan);
                
            std::vector<std::function<void(PrimitiveBlockPtr)>> cbs;
            std::vector<std::shared_ptr<Count>> result_parts; 
            for (size_t i=0; i < numchan; i++) {
                auto rr=std::make_shared<Count>(i,tiles,geom,change);
                cbs.push_back([rr,cp_callback](PrimitiveBlockPtr bl) {
                    if (bl) {
                        rr->add(bl->Index(), bl);
                    }
                    cp_callback(bl);
                });
                result_parts.push_back(rr);  
            }
            //read_blocks_split_primitiveblock(fn, cbs, {}, nullptr, change, objflags);
            rbc->read_primitive(cbs,objflags,nullptr);
            
            for (auto rr: result_parts) {
                result->expand(*rr);
            }
        } else {
            auto cbx = [result,cp](PrimitiveBlockPtr bl) {
                cp->call(bl);
                if (bl) {
                    result->add(bl->Index(), bl);
                }
            };
            //read_blocks_nothread_primitiveblock(fn, cbx, {}, nullptr, change, objflags);
            rbc->read_primitive_nothread(cbx,objflags,nullptr);
        }
    }
    
    lg.time("count");
    return result;
}
}

