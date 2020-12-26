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
#include "oqt/utils/compress.hpp"

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

class IdSetFilter : public IdSet {
    public:
        virtual ~IdSetFilter() {}
        virtual std::string str()=0;
        virtual void insert(ElementType ty, int64 id)=0;
};


class IdSetFilterSet : public IdSetFilter {
    public:
        IdSetFilterSet() {}
        virtual ~IdSetFilterSet() {}
        
        virtual bool contains(ElementType ty, int64 id) const {
            if (ty==ElementType::Node) { return nodes.count(id)>0; }
            if (ty==ElementType::Way) { return ways.count(id)>0; }
            if (ty==ElementType::Relation) { return relations.count(id)>0; }
            return false;
        };
        
        virtual std::string str() {
            std::stringstream ss;
            ss << "IdSetFilter with " << nodes.size() << " nodes, " << ways.size() << " ways, " << relations.size() << " relations";
            return ss.str();
        }
        virtual void insert(ElementType ty, int64 id) {
            if (ty==ElementType::Node) { nodes.insert(id); }
            if (ty==ElementType::Way) { ways.insert(id); }
            if (ty==ElementType::Relation) { relations.insert(id); }
        }
    private:
        std::set<int64> nodes;
        std::set<int64> ways;
        std::set<int64> relations;
        
        
};

class VecBoolSet {
    public:
        VecBoolSet(int64 max) : count(0) { 
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
            

class IdSetFilterVec : public IdSetFilter {
    public:
        IdSetFilterVec(int64 maxn, int64 maxw, int64 maxr) : nodes(maxn), ways(maxw), relations(maxr) {}
        virtual ~IdSetFilterVec() {
            nodes.clear();
            ways.clear();
            relations.clear();
        }
        
        virtual bool contains(ElementType ty, int64 id) const {
            
            if (ty==ElementType::Node) { return nodes.contains(id); }
            if (ty==ElementType::Way) { return ways.contains(id); }
            if (ty==ElementType::Relation) { return relations.contains(id); }
            return false;
        };
        
        std::string str() {
            std::stringstream ss;
            ss << "filter_idset_vec with " << nodes.size() << " nodes, " << ways.size() << " ways, " << relations.size() << " relations";
            return ss.str();
        }
        virtual void insert(ElementType ty, int64 id) {
            if (ty==ElementType::Node) { nodes.insert(id); }
            if (ty==ElementType::Way) { ways.insert(id); }
            if (ty==ElementType::Relation) { relations.insert(id); }
        }
    private:
        VecBoolSet nodes;
        VecBoolSet ways;
        VecBoolSet relations;
};


        
    

class CalculateIdSetFilter {
    public:
        CalculateIdSetFilter(std::shared_ptr<IdSetFilter> ids_, bbox box_, bool check_full_, const std::vector<LonLat>& poly_) : ids(ids_), box(box_), check_full(check_full_), poly(poly_) {
            if (!poly.empty()) { Logger::Message() << "CalculateIdSetFilter with poly [" << poly.size() << " verts]"; }
            notinpoly=0;
        }
        
        void call(minimal::BlockPtr mb) {
            if (!mb) {
                Logger::Message() << ids->str();
                Logger::Message() << "CalculateIdSetFilter finished: have " << extra_nodes.size() << " extra nodes and " << relmems.size() << " relmems; " <<notinpoly << " nodes in box but not poly";
                
                for (auto& n : extra_nodes) {
                    ids->insert(ElementType::Node, n);
                }
                extra_nodes.clear();
                
                for (size_t i=0; i < 5; i++) {
                    for (const auto& rr : relmems) {
                        if ((i==0) || (std::get<0>(rr)==ElementType::Relation)) {
                            if (ids->contains(std::get<0>(rr),std::get<1>(rr))) {
                                ids->insert(ElementType::Relation, std::get<2>(rr));
                            }
                        }
                    }
                }
                std::vector<std::tuple<ElementType,int64,int64>> e;
                relmems.swap(e);
                return;
            }
            bool box_contains = false;
            if (check_full && (mb->quadtree>0)) {
                auto qbx = quadtree::bbox(mb->quadtree,0.05);
                box_contains = bbox_contains(box, qbx);
                
               if (check_full && box_contains && (!poly.empty())) {
                    box_contains  =
                        point_in_poly(poly, LonLat{qbx.minx,qbx.miny}) &&
                        point_in_poly(poly, LonLat{qbx.minx,qbx.maxy}) &&
                        point_in_poly(poly, LonLat{qbx.maxx,qbx.miny}) &&
                        point_in_poly(poly, LonLat{qbx.maxx,qbx.maxy});
                    
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
                if (!point_in_poly(poly, LonLat{lon,lat})) {
                    notinpoly++;
                    return false;
                } else {
                    return true;
                }
            }
            return false;
        }
        
        void check_node(const minimal::Node& n,bool box_contains) {
            if (box_contains || check_point(n.lon, n.lat)) {
                
                ids->insert(ElementType::Node, n.id);
            }
        }
        void check_way(const minimal::Way& w,bool box_contains) {
            auto wns = read_packed_delta(w.refs_data);
            bool found=box_contains || [this, &wns]() {
                for (const auto& r: wns) {
                    if (ids->contains(ElementType::Node,r)) {
                        return true;
                    }
                }
                return false;
            }();
            if (!found) {
                return; 
            }
            ids->insert(ElementType::Way, w.id);
            for (const auto& r: wns) {
                if (!ids->contains(ElementType::Node,r)) {
                    extra_nodes.insert(r);
                }
            }
        }
        
        void check_relation(const minimal::Relation& r, bool box_contains) {
            //if (box_contains) {
            //    ids->relations.insert(r.id);
           // }
            auto tys = read_packed_int(r.tys_data);
            auto rfs = read_packed_delta(r.refs_data);
            
            bool found=[&]() {
                for (size_t i=0; i < tys.size(); i++) {
                    if (ids->contains((ElementType) tys[i],rfs[i])) {
                        return true;
                    }
                }
                return false;
            }();
            if (found) {
                ids->insert(ElementType::Relation, r.id);
            } else {
                for (size_t i=0; i < tys.size(); i++) {
                    relmems.push_back(std::make_tuple((ElementType) tys[i],rfs[i],r.id));
                }
            }
        }
    
    
    private:
        std::shared_ptr<IdSetFilter> ids;
        bbox box;
        bool check_full;
        std::vector<LonLat> poly;
        
        std::set<int64> extra_nodes;
        std::vector<std::tuple<ElementType,int64,int64>> relmems;
        std::set<int64> xx;
        size_t notinpoly;
};

IdSetPtr calc_idset_filter(std::shared_ptr<ReadBlocksCaller> read_blocks_caller, const bbox& filter_box, const std::vector<LonLat>& poly, size_t numchan) {
    double boxarea = (filter_box.maxx-filter_box.minx)*(filter_box.maxy-filter_box.miny) / 10000000.0 / 10000000.0;
    Logger::Message() << "filter_box=" << filter_box << ", area=" << boxarea;
    
    std::shared_ptr<IdSetFilter> filter_impl;
    if (boxarea > 5) {
        filter_impl = std::make_shared<IdSetFilterVec>(1ll<<34, 1ll<<24, 1ll<<18);
    } else {
        filter_impl = std::make_shared<IdSetFilterSet>();
    }
    auto cfi = std::make_shared<CalculateIdSetFilter>(filter_impl, filter_box, poly.empty(), poly);
    auto rc = multi_threaded_callback<minimal::Block>::make([cfi](minimal::BlockPtr mb) { cfi->call(mb); }, numchan);
    read_blocks_caller->read_minimal(rc, ReadBlockFlags::Empty, nullptr);
    
    Logger::Message() << filter_impl->str();
    
    return filter_impl;
}

class FilterRels { 
    public:
        FilterRels(IdSetPtr filter_) : filter(filter_) {tr=0; tnx=0; tnm=0;}
        
        
        ~FilterRels() {
            Logger::Message() << "filtered " << tr << " relations, removed " << tnm << " completely, removed " << tnx << " members";
        }
        PrimitiveBlockPtr call(PrimitiveBlockPtr pb) {
            
            if (!pb) {
                return pb;
            }
            
            std::map<size_t,std::shared_ptr<Relation>> tt;
            size_t nm=0; size_t nx=0;
            for (size_t i=0; i < pb->size(); i++) {
                
                if (pb->at(i)->Type()==ElementType::Relation) {
                    auto rel = std::dynamic_pointer_cast<Relation>(pb->at(i));
                    
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
            
            auto pb_new = std::make_shared<PrimitiveBlock>(pb->Index(), pb->size()-nm);
            pb_new->SetQuadtree(pb->Quadtree());
            pb_new->SetStartDate(pb->StartDate());
            pb_new->SetEndDate(pb->EndDate());
            
            
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
            IdSetPtr filter,
            std::vector<primitiveblock_callback> cbs) {
            
            auto fr = std::make_shared<FilterRels>(filter);//,cbs);
            std::vector<primitiveblock_callback> res;
            for (auto cb: cbs) {
                res.push_back([fr,cb](PrimitiveBlockPtr pb) { cb(fr->call(pb)); });
            }
            return res;
        }
        
    private:
        IdSetPtr filter;
        
        std::atomic<size_t> tr, tnm, tnx;
                
};


class Progress {
    public:
        Progress(primitiveblock_callback cb_) : nexti(0), li(0),lq(0),cb(cb_) {}
        
        void call(PrimitiveBlockPtr bl) {
            if (!bl) {
                Logger::Progress(100.0) << "{" << ts << "}" << std::setw(6) << std::setw(18) << " "
                    << " " << std::setw(6) << li
                    << " " << std::setw(18) << lq;
                
                cb(bl);
                return;
            }
            li=bl->Index();
            lq=bl->Quadtree();
            if (li>nexti) {
            
                Logger::Progress(bl->FileProgress()) << "{" << ts << "}" << std::setw(6) << std::setw(18) << " "
                    << " " << std::setw(6) << li
                    << " " << std::setw(18) << lq;
                    
                nexti=bl->Index()+100;
            }
            cb(bl);
        }
    private:
        int64 nexti;
        int64 li; int64 lq;
        TimeSingle ts;
        
        primitiveblock_callback cb;
};

primitiveblock_callback log_progress(primitiveblock_callback cb) {
    auto pg = std::make_shared<Progress>(cb);
    
    return [pg](PrimitiveBlockPtr bl) { pg->call(bl); };
}


void run_mergechanges(
    const std::string& infile_name,
    const std::string& outfn,
    size_t numchan, bool sort_objs, bool filter_objs,
    bbox filter_box, const std::vector<LonLat>& poly, int64 enddate,
    const std::string& tempfn, size_t blocksize, bool sortfile, bool inmem) {
    
    
    
    auto read_blocks_caller = make_read_blocks_caller(infile_name,filter_box,poly, enddate);
    
    Logger::Get().time("filter file locs");
    
    IdSetPtr filter;
    
    
    if (filter_objs) {
        filter=calc_idset_filter(read_blocks_caller, filter_box, poly, numchan);
        Logger::Get().time("find ids filter");
    }
    
    
    
    
    
    auto outfile_header = std::make_shared<Header>();
    outfile_header->SetBBox(filter_box);
    auto outfile_writer = (sort_objs ? make_pbffilewriter(outfn, outfile_header) : make_pbffilewriter_filelocs(outfn,outfile_header));
    auto packers = make_final_packers_sync(outfile_writer, numchan, enddate, !sort_objs, true);
    
    auto read_data = [read_blocks_caller,filter,numchan,filter_objs](std::vector<primitiveblock_callback> addobjs) {
        if (filter_objs) {
            
            addobjs = FilterRels::make(filter,addobjs);
        }
        read_blocks_caller->read_primitive(addobjs, ReadBlockFlags::Empty, filter);
    };
        
    if (sort_objs) {
        if (inmem) {
            std::vector<ElementPtr> all;
            auto add_all = multi_threaded_callback<PrimitiveBlock>::make([&all](PrimitiveBlockPtr bl) {
                if (!bl) { return; }
                for (size_t i=0; i < bl->size(); i++) {
                    all.push_back(bl->at(i));
                }
                
            },numchan);
            
            read_data(add_all);
            
            
            Logger::Get().time("read file");
            std::sort(all.begin(),all.end(),element_cmp);
            Logger::Get().time("sort data");
            
            
            
            auto collect = make_collectobjs(packers,8000);
            for (auto& o: all) {
                collect(o);
            }
            collect(nullptr);
            
            Logger::Get().time("objs written");
        } else {
            
            auto blobs = (read_blocks_caller->num_tiles() > 50000)
                ? make_blobstore_filesplit(tempfn, 128)  //changed 32 to 50 // 25?
                : make_blobstore_file(tempfn, sortfile);
            
            auto temps = make_tempobjs(blobs, numchan);
            
            
            if (true) {
                std::vector<primitiveblock_callback> splits;
                for (size_t i=0; i < numchan; i++) {
                    //splits.push_back(make_splitbyid_callback(temps->add_func(i), blocksize, 2000000, (1ll)<<20));
                    splits.push_back(make_splitbyid_callback(temps->add_func(i), blocksize, 1500000, (1ll)<<21));
                }
                read_data(splits);
                temps->finish();
                Logger::Get().time("read file");
            }
        
                        
            auto collect = make_collectobjs(packers,8000);
            auto collect_cb = [collect](PrimitiveBlockPtr p) {
                if (p) {
                    for (auto o: p->Objects()) {
                        collect(o);
                    }
                } else {
                    collect(nullptr);
                }
            };
            
            auto collect_split = multi_threaded_callback<PrimitiveBlock>::make(collect_cb,numchan);
            
            
            std::vector<primitiveblock_callback> merged_sorted;
            TimeSingle ts;
            bool isf=true;
            for (auto mm: collect_split) {
                merged_sorted.push_back(
                    [mm,&ts,isf](PrimitiveBlockPtr oo) {
                        if (!oo) { return mm(nullptr); }
                        auto& objs = oo->Objects();
                        std::sort(objs.begin(), objs.end(), element_cmp);
                        if (isf) {
                            Logger::Progress lp(oo->FileProgress());
                            lp << "{" << TmStr{ts.since(),6,1} << "} merged " << std::setw(5) << oo->Index() << " " << std::setw(8) << objs.size() << " objs";
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
        
            Logger::Get().time("objs written");
        }
    } else {
        
        packers[0] = log_progress(packers[0]);
        
        read_data(packers);
        Logger::Get().time("written blocks");
    }    
    
    outfile_writer->finish();
    Logger::Get().time("finished outfile");
    
}
}
