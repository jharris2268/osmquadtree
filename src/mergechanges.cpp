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

#include "oqt/readpbffile.hpp"
#include "oqt/writepbffile.hpp"
#include "oqt/writefile.hpp"
#include "oqt/writeblock.hpp"
#include "oqt/simplepbf.hpp"
#include "oqt/utils.hpp"
#include "oqt/picojson.h"
#include "oqt/sortfile.hpp"
#include "oqt/sorting/tempobjs.hpp"

#include "oqt/elements/relation.hpp"

#include <regex>
#include <iterator>

#include <atomic>
namespace oqt {
inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::shared_ptr<header> getHeaderBlock_filezz(std::ifstream& infile) {
    auto fb = readFileBlock(0,infile);
    if (fb->blocktype!="OSMHeader") {
        throw std::domain_error("first block not a header");
    }
    int64 p = infile.tellg();
    //infile.close();
    //std::string dd = decompress(std::get<2>(fb),std::get<3>(fb));
    return readPbfHeader(fb->get_data(),p);
}

std::shared_ptr<header> getHeaderBlockzz(const std::string& fn) {
    std::ifstream infile(fn, std::ios::binary | std::ios::in);
    if (!infile.good()) { throw std::domain_error("not a file?? "+fn); }
    auto r= getHeaderBlock_filezz(infile);
    infile.close();
    return r;
}


std::pair<std::vector<std::string>,int64> read_filenames(const std::string& prfx, int64 enddate) {
    std::ifstream filelist(prfx+"filelist.json", std::ios::in);
    if (!filelist.good()) {
        throw std::domain_error("?? "+prfx+"filelist.json");
    }


    picojson::value v;
    filelist >> v;
    std::string err = picojson::get_last_error();

    if (! err.empty()) {
      logger_message() << "filelist errors: " << err;
    }
    if (!v.is<picojson::array>()) {
        throw std::domain_error("not an array");
    }
    auto varr = v.get<picojson::array>();
    logger_message() << "filelist: " << varr.size() << "entries";

    std::vector<std::string> result;
    int64 last_date=0;
    
    for (auto& vlv : varr) {

        auto vl = vlv.get<picojson::object>();
        std::string vl_date_in = vl["EndDate"].get<std::string>();
        int64 vl_date=read_date(vl_date_in);
        if (vl_date==0) {
            throw std::domain_error("can't parse "+vl_date_in);
        }
        if ((enddate>0) && (vl_date > enddate)) {
            logger_message() << "skip entry for " << vl_date_in << "(" << vl_date << ">" << enddate;
            continue;
        }
        
        std::string fn = prfx+vl["Filename"].get<std::string>();
        if (vl_date>last_date) {
            last_date=vl_date;
        }
        result.push_back(fn);
    }
    
    return std::make_pair(result, last_date);
}
typedef std::function<void(std::shared_ptr<minimalblock>)> minimalblock_callback;
class ReadBlocksCaller {
    public:
        virtual void read_primitive(std::vector<primitiveblock_callback> cbs, std::shared_ptr<idset> filter)=0;
        //virtual void read_packed(std::vector<packedblock_callback> cbs, std::shared_ptr<idset> filter)=0;
        virtual void read_minimal(std::vector<minimalblock_callback> cbs, std::shared_ptr<idset> filter)=0;
        virtual size_t num_tiles()=0;
};



class ReadBlocksSingle : public ReadBlocksCaller {
    public:
        ReadBlocksSingle(const std::string& fn_, bbox filter_box, const lonlatvec& poly) : fn(fn_) {
            if (!box_empty(filter_box)) {
                auto head = getHeaderBlockzz(fn);
                if (head && (!head->index.empty())) {
                    for (const auto& l : head->index) {
                        if (overlaps_quadtree(filter_box,std::get<0>(l))) {
                            if (poly.empty() || polygon_box_intersects(poly, quadtree::bbox(std::get<0>(l), 0.05))) {
                            
                            
                                locs.push_back(std::get<1>(l));
                            }
                        }
                    }
                    if (locs.empty()) {
                        throw std::domain_error("no tiles match filter box");
                    }
                }
            }
        }
        void read_primitive(std::vector<primitiveblock_callback> cbs, std::shared_ptr<idset> filter) {
            read_blocks_split_primitiveblock(fn, cbs, locs, filter, false, 15);
        }
        
        void read_minimal(std::vector<minimalblock_callback> cbs, std::shared_ptr<idset> filter)  {
            read_blocks_split_minimalblock(fn, cbs, locs, 15);
        }
        
        size_t num_tiles() { return locs.size(); }
    private:
        std::string fn;
        std::vector<int64> locs;
};
        
class ReadBlocksMerged : public ReadBlocksCaller {
    public:
        ReadBlocksMerged(const std::string& prfx, bbox filter_box_, const lonlatvec& poly, int64 enddate_) : enddate(enddate_),filter_box(filter_box_)  {
            std::tie(filenames,enddate) = read_filenames(prfx, enddate);
            if (filenames.empty()) { throw std::domain_error("no filenames!"); }
            bbox top_box;
            bool empty_box = box_empty(filter_box);
            for (size_t file_idx=0; file_idx < filenames.size(); file_idx++) {
                const auto& fn = filenames.at(file_idx);
                auto head = getHeaderBlockzz(fn);
                if (!head) { throw std::domain_error("file "+fn+" has no header"); }
                if (file_idx==0) { top_box=head->box; }
                if (head->index.empty()) { throw std::domain_error("file "+fn+" has no tile index"); }
                
                for (const auto& l : head->index) {
                    if (file_idx>0) {
                        if (locs.count(std::get<0>(l))>0) {
                            locs[std::get<0>(l)].push_back(std::make_pair(file_idx, std::get<1>(l)));
                        }
                    } else {
                        
                        if (empty_box || overlaps_quadtree(filter_box,std::get<0>(l))) {
                            if (poly.empty() || polygon_box_intersects(poly, quadtree::bbox(std::get<0>(l), 0.05))) {
                        
                                locs[std::get<0>(l)].push_back(std::make_pair(file_idx, std::get<1>(l)));
                            }
                        }
                    }
                }
                
            }
            if (box_empty(filter_box) || bbox_contains(filter_box, top_box)) {
                filter_box = top_box;
            }
            buffer = 1<<14;
            //buffer = 0;
            
        }
        
        void read_primitive(std::vector<primitiveblock_callback> cbs, std::shared_ptr<idset> filter) {
            logger_message() << "ReadBlocksMerged::read_primitive";
            
            read_blocks_split_merge<primitiveblock>(filenames, cbs, locs, filter, 7, buffer);
        }
        void read_minimal(std::vector<std::function<void(std::shared_ptr<minimalblock>)>> cbs, std::shared_ptr<idset> filter)  {
            read_blocks_split_merge<minimalblock>(filenames, cbs, locs, filter, 7, buffer);
        }
        
        /*void read_packed(std::vector<packedblock_callback> cbs, std::shared_ptr<idset> filter)  {
            read_blocks_split_merge<packedblock>(filenames, cbs, locs, filter, 7, buffer);
        }*/
        int64 actual_enddate() { return enddate; }
        bbox actual_filter_box() { return filter_box; }
        size_t num_tiles() { return locs.size(); }
        
    private:
        std::vector<std::string> filenames;
        src_locs_map locs;
        int64 enddate;
        bbox filter_box;
        size_t buffer;
};
        
std::shared_ptr<ReadBlocksCaller> make_read_blocks_caller(
        const std::string& infile_name, 
        bbox& filter_box, const lonlatvec& poly, int64& enddate) {
   
    if (ends_with(infile_name, ".pbf")) {
        auto hh = getHeaderBlockzz(infile_name);
        if (box_empty(filter_box) || (!box_empty(hh->box) && bbox_contains(filter_box, hh->box))) {
            filter_box = hh->box;
        }
        return std::make_shared<ReadBlocksSingle>(infile_name, filter_box, poly);
    }
    auto rs = std::make_shared<ReadBlocksMerged>(infile_name, filter_box, poly, enddate);
    enddate = rs->actual_enddate();
    filter_box = rs->actual_filter_box();
    return rs;
}

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
                
                if (box_contains && (!poly.empty())) {
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
    
    double boxarea = (filter_box.maxx-filter_box.minx)*(filter_box.maxy-filter_box.miny) / 10000000.0 / 10000000.0;
    logger_message() << "filter_box=" << filter_box << ", area=" << boxarea;
    if (filter_objs) {
        //throw std::domain_error("not impl");
        std::shared_ptr<idset_calc> filter_impl;
        if (boxarea > 5) {
            filter_impl = std::make_shared<filter_idset_vec>(1ll<<34, 1ll<<24, 1ll<<18);
        } else {
            filter_impl = std::make_shared<filter_idset>();
        }
        auto cfi = std::make_shared<CalculateFilterIdset>(filter_impl, filter_box, poly.empty(), poly);
        auto rc = multi_threaded_callback<minimalblock>::make([cfi](std::shared_ptr<minimalblock> mb) { cfi->call(mb); }, numchan);
        read_blocks_caller->read_minimal(rc, nullptr);
        filter = filter_impl;
        logger_message() << filter_impl->str();
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
