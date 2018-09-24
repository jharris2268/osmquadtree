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

#include "oqt/pbfformat/readblockscaller.hpp"

#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/utils/pbf/protobuf.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/date.hpp"

#include "oqt/utils/multithreadedcallback.hpp"


#include "picojson.h"
#include "oqt/sorting/mergechanges.hpp"
#include "oqt/sorting/tempobjs.hpp"
#include "oqt/sorting/final.hpp"
#include "oqt/sorting/splitbyid.hpp"

#include "oqt/elements/relation.hpp"

#include <regex>
#include <set>
#include <iterator>

#include <atomic>
namespace oqt {


class idset_calc : public idset {
    public:
        virtual void insert(elementtype ty, int64 id)=0;
        virtual std::string str()=0;
};

class filter_idset : public idset_calc {
    public:
        filter_idset() {}
        virtual ~filter_idset() {}
        
        virtual bool contains(elementtype ty, int64 id) const {
            if (ty==Node) { return nodes.count(id)>0; }
            if (ty==Way) { return ways.count(id)>0; }
            if (ty==Relation) { return relations.count(id)>0; }
            return false;
        };
        
        virtual std::string str() {
            std::stringstream ss;
            ss << "filter_idset with " << nodes.size() << " nodes, " << ways.size() << " ways, " << relations.size() << " relations";
            return ss.str();
        }
        virtual void insert(elementtype ty, int64 id) {
            if (ty==Node) { nodes.insert(id); }
            if (ty==Way) { ways.insert(id); }
            if (ty==Relation) { relations.insert(id); }
        }
    private:
        std::set<int64> nodes;
        std::set<int64> ways;
        std::set<int64> relations;
        
        
};

class vecbool_set {
    public:
        vecbool_set(int64 max) : count(0) { 
            vec.resize(max+1);
        }
        
        void insert(int64 i) {
            if (i<0) { throw std::range_error("id must be >= 0"); }
            if ((size_t)i >= vec.size()) {
                vec.resize(i+1000);
            }
            if (!vec.at(i)) {
                vec.at(i)=true;
                count++;
            }
        }
        
        size_t size() const { return count; }
        
        bool contains(int64 i) const {
            if (i<0) { return false; }
            if ((size_t)i >=vec.size()) { return false; }
            return vec.at(i);
        }
        
        void clear() {
            vec.clear();
            std::vector<bool> x;
            vec.swap(x);
        }
            
    private:
        size_t count;
        std::vector<bool> vec;
};
            

class filter_idset_vec : public idset_calc {
    public:
        filter_idset_vec(int64 maxn, int64 maxw, int64 maxr) : nodes(maxn), ways(maxw), relations(maxr) {}
        virtual ~filter_idset_vec() {
            nodes.clear();
            ways.clear();
            relations.clear();
        }
        
        virtual bool contains(elementtype ty, int64 id) const {
            
            if (ty==Node) { return nodes.contains(id); }
            if (ty==Way) { return ways.contains(id); }
            if (ty==Relation) { return relations.contains(id); }
            return false;
        };
        
        std::string str() {
            std::stringstream ss;
            ss << "filter_idset_vec with " << nodes.size() << " nodes, " << ways.size() << " ways, " << relations.size() << " relations";
            return ss.str();
        }
        virtual void insert(elementtype ty, int64 id) {
            if (ty==Node) { nodes.insert(id); }
            if (ty==Way) { ways.insert(id); }
            if (ty==Relation) { relations.insert(id); }
        }
    private:
        vecbool_set nodes;
        vecbool_set ways;
        vecbool_set relations;
};


        
    

class CalculateFilterIdset {
    public:
        CalculateFilterIdset(std::shared_ptr<idset_calc> ids_, bbox box_, bool check_full_, const lonlatvec& poly_) : ids(ids_), box(box_), check_full(check_full_), poly(poly_) {
            if (!poly.empty()) { logger_message() << "CalculateFilterIdset with poly [" << poly.size() << " verts]"; }
            notinpoly=0;
        }
        
        void call(std::shared_ptr<minimalblock> mb) {
            if (!mb) {
                logger_message() << ids->str();
                logger_message() << "CalculateFilterIdset finished: have " << extra_nodes.size() << " extra nodes and " << relmems.size() << " relmems; " <<notinpoly << " nodes in box but not poly";
                
                for (auto& n : extra_nodes) {
                    ids->insert(elementtype::Node, n);
                }
                extra_nodes.clear();
                
                for (size_t i=0; i < 5; i++) {
                    for (const auto& rr : relmems) {
                        if ((i==0) || (std::get<0>(rr)==2)) {
                            if (ids->contains(std::get<0>(rr),std::get<1>(rr))) {
                                ids->insert(elementtype::Relation, std::get<2>(rr));
                            }
                        }
                    }
                }
                std::vector<std::tuple<elementtype,int64,int64>> e;
                relmems.swap(e);
                return;
            }
            bool box_contains = false;
            if (check_full && (mb->quadtree>0)) {
                auto qbx = quadtree::bbox(mb->quadtree,0.05);
                box_contains = bbox_contains(box, qbx);
                
               if (check_full && box_contains && (!poly.empty())) {
                    box_contains  =
                        point_in_poly(poly, lonlat{qbx.minx,qbx.miny}) &&
                        point_in_poly(poly, lonlat{qbx.minx,qbx.maxy}) &&
                        point_in_poly(poly, lonlat{qbx.maxx,qbx.miny}) &&
                        point_in_poly(poly, lonlat{qbx.maxx,qbx.maxy});
                    
                }
                
            }
            
            
            for (const auto& n: mb->nodes) {
                check_node(n,box_contains);
            }
            
            for (const auto& w: mb->ways) {
                check_way(w,box_contains);
            }
            for (const auto& r: mb->relations) {
                check_relation(r,box_contains);
            }
        }
        
        bool check_point(int64 lon, int64 lat) {
            if (contains_point(box, lon, lat)) {
                if (poly.empty()) { return true; }
                if (!point_in_poly(poly, lonlat{lon,lat})) {
                    notinpoly++;
                    return false;
                } else {
                    return true;
                }
            }
            return false;
        }
        
        void check_node(const minimalnode& n,bool box_contains) {
            if (box_contains || check_point(n.lon, n.lat)) {
                
                ids->insert(elementtype::Node, n.id);
            }
        }
        void check_way(const minimalway& w,bool box_contains) {
            auto wns = readPackedDelta(w.refs_data);
            bool found=box_contains || [this, &wns]() {
                for (const auto& r: wns) {
                    if (ids->contains(elementtype::Node,r)) {
                        return true;
                    }
                }
                return false;
            }();
            if (!found) {
                return; 
            }
            ids->insert(elementtype::Way, w.id);
            for (const auto& r: wns) {
                if (!ids->contains(elementtype::Node,r)) {
                    extra_nodes.insert(r);
                }
            }
        }
        
        void check_relation(const minimalrelation& r, bool box_contains) {
            //if (box_contains) {
            //    ids->relations.insert(r.id);
           // }
            auto tys = readPackedInt(r.tys_data);
            auto rfs = readPackedDelta(r.refs_data);
            
            bool found=[&]() {
                for (size_t i=0; i < tys.size(); i++) {
                    if (ids->contains((elementtype) tys[i],rfs[i])) {
                        return true;
                    }
                }
                return false;
            }();
            if (found) {
                ids->insert(elementtype::Relation, r.id);
            } else {
                for (size_t i=0; i < tys.size(); i++) {
                    relmems.push_back(std::make_tuple((elementtype) tys[i],rfs[i],r.id));
                }
            }
        }
    
    
    private:
        std::shared_ptr<idset_calc> ids;
        bbox box;
        bool check_full;
        lonlatvec poly;
        
        std::set<int64> extra_nodes;
        std::vector<std::tuple<elementtype,int64,int64>> relmems;
        std::set<int64> xx;
        size_t notinpoly;
};


class FilterRels { 
    public:
        FilterRels(std::shared_ptr<idset> filter_) : filter(filter_) {tr=0; tnx=0; tnm=0;}
        
        
        ~FilterRels() {
            logger_message() << "filtered " << tr << " relations, removed " << tnm << " completely, removed " << tnx << " members";
        }
        primitiveblock_ptr call(primitiveblock_ptr pb) {
            
            if (!pb) {
                return pb;
            }
            
            std::map<size_t,std::shared_ptr<relation>> tt;
            size_t nm=0; size_t nx=0;
            for (size_t i=0; i < pb->size(); i++) {
                
                if (pb->at(i)->Type()==Relation) {
                    auto rel = std::dynamic_pointer_cast<relation>(pb->at(i));
                    
                    size_t x=rel->Members().size();
                    if (rel->filter_members(filter)) {
                        nx += (x-rel->Members().size());
                        if (rel->Members().empty()) {
                            nm++;
                            tt.insert(std::make_pair(i,nullptr));
                        } else {
                            
                            tt.insert(std::make_pair(i,rel));
                        }
                    }
                }
            }
            tr += tt.size();
            tnm += nm; tnx += nx;
            if (tt.empty()) {
                
                return pb;
            }
            
            auto pb_new = std::make_shared<primitiveblock>(pb->index, pb->size()-nm);
            pb_new->quadtree=pb->quadtree;
            pb_new->startdate=pb->startdate;
            pb_new->enddate=pb->enddate;
            
            
            for (size_t i=0; i < pb->size(); i++) {
                if (tt.count(i)==0) {
                    pb_new->add(pb->at(i));
                } else {
                    if (tt[i]) {
                        pb_new->add(tt[i]);
                    }
                }
            }
            
            return pb_new;
        }
        
        static std::vector<primitiveblock_callback> make(
            std::shared_ptr<idset> filter,
            std::vector<primitiveblock_callback> cbs) {
            
            auto fr = std::make_shared<FilterRels>(filter);//,cbs);
            std::vector<primitiveblock_callback> res;
            for (auto cb: cbs) {
                res.push_back([fr,cb](primitiveblock_ptr pb) { cb(fr->call(pb)); });
            }
            return res;
        }
        
    private:
        std::shared_ptr<idset> filter;
        
        std::atomic<size_t> tr, tnm, tnx;
                
};


class progress {
    public:
        progress(primitiveblock_callback cb_) : i(0), nt(1), cb(cb_) {}
        
        void call(primitiveblock_ptr bl) {
            if (!bl) {
                logger_progress(100.0) << "{" << ts << "}" << std::setw(6) << i << std::setw(18) << " ";
                
                cb(bl);
                return;
            }
            //if ((i%100)==1 ) {
            
            logger_progress(bl->file_progress) << "{" << ts << "}" << std::setw(6) << i << std::setw(18) << " "
                << " " << std::setw(6) << bl->index
                << " " << std::setw(18) << quadtree::string(bl->quadtree);
                
            i=bl->index;
            cb(bl);
        }
    private:
        size_t i;
        time_single ts;
        double nt;
        primitiveblock_callback cb;
};

primitiveblock_callback log_progress(primitiveblock_callback cb) {
    auto pg = std::make_shared<progress>(cb);
    
    return [pg](primitiveblock_ptr bl) { pg->call(bl); };
}


std::shared_ptr<idset> calc_idset_filter(std::shared_ptr<ReadBlocksCaller> read_blocks_caller, const bbox& filter_box, const lonlatvec& poly, size_t numchan) {
    double boxarea = (filter_box.maxx-filter_box.minx)*(filter_box.maxy-filter_box.miny) / 10000000.0 / 10000000.0;
    logger_message() << "filter_box=" << filter_box << ", area=" << boxarea;
    
    std::shared_ptr<idset_calc> filter_impl;
    if (boxarea > 5) {
        filter_impl = std::make_shared<filter_idset_vec>(1ll<<34, 1ll<<24, 1ll<<18);
    } else {
        filter_impl = std::make_shared<filter_idset>();
    }
    auto cfi = std::make_shared<CalculateFilterIdset>(filter_impl, filter_box, poly.empty(), poly);
    auto rc = multi_threaded_callback<minimalblock>::make([cfi](std::shared_ptr<minimalblock> mb) { cfi->call(mb); }, numchan);
    read_blocks_caller->read_minimal(rc, nullptr);
    
    logger_message() << filter_impl->str();
    
    return filter_impl;
}
        

void run_mergechanges(
    const std::string& infile_name,
    const std::string& outfn,
    size_t numchan, bool sort_objs, bool filter_objs,
    bbox filter_box, const lonlatvec& poly, int64 enddate,
    const std::string& tempfn, size_t blocksize, bool sortfile, bool inmem) {
    
    
    auto lg=get_logger();
    auto read_blocks_caller = make_read_blocks_caller(infile_name,filter_box,poly, enddate);
    
    lg->time("filter file locs");
    
    std::shared_ptr<idset> filter;
    
    
    if (filter_objs) {
        filter=calc_idset_filter(read_blocks_caller, filter_box, poly, numchan);
        lg->time("find ids filter");
    }
    
    
    
    
    
    auto outfile_header = std::make_shared<header>();
    outfile_header->box = filter_box;    
    auto outfile_writer = make_pbffilewriter(outfn, outfile_header, !sort_objs);
    auto packers = make_final_packers_sync(outfile_writer, numchan, enddate, !sort_objs, true);
    
    auto read_data = [read_blocks_caller,filter,numchan,filter_objs](std::vector<primitiveblock_callback> addobjs) {
        if (filter_objs) {
            
            addobjs = FilterRels::make(filter,addobjs);
        }
        read_blocks_caller->read_primitive(addobjs, filter);
    };
        
    if (sort_objs) {
        if (inmem) {
            std::vector<element_ptr> all;
            auto add_all = multi_threaded_callback<primitiveblock>::make([&all](primitiveblock_ptr bl) {
                if (!bl) { return; }
                for (size_t i=0; i < bl->size(); i++) {
                    all.push_back(bl->at(i));
                }
                
            },numchan);
            
            read_data(add_all);
            
            
            lg->time("read file");
            std::sort(all.begin(),all.end(),element_cmp);
            lg->time("sort data");
            
            
            
            auto collect = make_collectobjs(packers,8000);
            for (auto& o: all) {
                collect(o);
            }
            collect(nullptr);
            
            lg->time("objs written");
        } else {
            
            auto blobs = (read_blocks_caller->num_tiles() > 50000)
                ? make_blobstore_filesplit(tempfn, 8000/25)  //changed 32 to 50
                : make_blobstore_file(tempfn, sortfile);
            
            auto temps = make_tempobjs(blobs, numchan);
            
            
            
            std::vector<primitiveblock_callback> splits;
            for (size_t i=0; i < numchan; i++) {
                //splits.push_back(make_splitbyid_callback(temps->add_func(i), blocksize, 2000000, (1ll)<<20));
                splits.push_back(make_splitbyid_callback(temps->add_func(i), blocksize, 2000000, (1ll)<<19));
            }
            read_data(splits);
            temps->finish();
            lg->time("read file");
        
        
                        
            auto collect = make_collectobjs(packers,8000);
            auto collect_cb = [collect](primitiveblock_ptr p) {
                if (p) {
                    for (auto o: p->objects) {
                        collect(o);
                    }
                } else {
                    collect(nullptr);
                }
            };
            
            auto collect_split = multi_threaded_callback<primitiveblock>::make(collect_cb,numchan);
            
            
            std::vector<primitiveblock_callback> merged_sorted;
            time_single ts;
            bool isf=true;
            for (auto mm: collect_split) {
                merged_sorted.push_back(
                    [mm,&ts,isf](primitiveblock_ptr oo) {
                        if (!oo) { return mm(nullptr); }
                        auto& objs = oo->objects;
                        std::sort(objs.begin(), objs.end(), element_cmp);
                        if (isf) {
                            logger_progress lp(oo->file_progress);
                            lp << "{" << TmStr{ts.since(),6,1} << "} merged " << std::setw(5) << oo->index << " " << std::setw(8) << objs.size() << " objs";
                            if (objs.size()>0) {
                                auto f=objs.front();
                                lp << " " << f->Type() << " " << std::setw(10) << f->Id();
                                if (objs.size()>1) {
                                    auto b=objs.back();
                                    lp << " to " << b->Type() << " " << std::setw(10) << b->Id();
                                }
                            }
                        }
                        
                        mm(oo);
                    }
                );
                isf=false;
            }
            temps->read(merged_sorted);
        
            lg->time("objs written");
        }
    } else {
        
        packers[0] = log_progress(packers[0]);
        
        read_data(packers);
        lg->time("written blocks");
    }    
    
    outfile_writer->finish();
    lg->time("finished outfile");
    
}
}
