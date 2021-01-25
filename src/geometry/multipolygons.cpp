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

#include "oqt/geometry/multipolygons.hpp"
#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"
#include "oqt/utils/logger.hpp"
#include <deque>
namespace oqt {
namespace geometry {


bool hasr(primblock_ptr p) {
    for (auto o : p->Objects()) {
        if ((o->Type()==ElementType::Way) || (o->Type()==ElementType::Relation)) { return true; }
    }
    return false;
}



bool is_ring(std::shared_ptr<WayWithNodes> w) {
    return (w->Refs().size()>2) && (w->Refs().front()==w->Refs().back());
}



template <class T>
int64 first_point(const T& r) {
    const Ring::Part& r0 = r.front();
    if (r0.reversed) {
        return r0.refs.back();
    }
    return r0.refs.front();
}

template <class T>
int64 last_point(const T& r) {
    const Ring::Part& r1 = r.back();
    if (r1.reversed) {
        return r1.refs.front();
    }
    return r1.refs.back();
}


Ring::Part make_ringpart(std::shared_ptr<WayWithNodes> w, bool d) {
    return Ring::Part{w->Id(),w->Refs(),w->LonLats(), d};
}

std::ostream& ringstr(std::ostream& strm, const Ring& ring) {
    strm << ring.parts.size() << " parts, ";
    for (const auto& p: ring.parts) {
        strm << p.orig_id << " [" << p.refs.size() << "]";
        if (p.reversed) { strm << "r"; }
        strm << " ";
    }
    return strm;
}
std::ostream& tagstr(std::ostream& strm, const tagvector& tgs) {
    strm << "{";
    bool first=true;
    for (const auto& t: tgs) {
        if (!first) { strm << ", ";}
        strm << t.key << "='" << t.val << "'";
        first =false;
    }
    return strm;
}
typedef std::pair<bool,std::deque<Ring::Part>> tempring;
typedef std::vector<tempring> tempringvec;

void add_to_rings(tempringvec& rings, std::shared_ptr<WayWithNodes> w) {
    if (is_ring(w)) {
        rings.push_back(std::make_pair(true,std::deque<Ring::Part>{make_ringpart(w,false)}));
        return;
    }
    if (!rings.empty()) {
        int64 a=w->Refs().front();
        int64 b=w->Refs().back();

        for (auto& r: rings) {
            if (r.first) {
                //skip
            } else if (a == last_point(r.second)) {
                r.second.push_back(make_ringpart(w,false));
                r.first= first_point(r.second)==last_point(r.second);
                return;
            } else if (b == last_point(r.second)) {
                r.second.push_back(make_ringpart(w, true));
                r.first= first_point(r.second)==last_point(r.second);
                return;
            } else if (a==first_point(r.second)) {
                r.second.push_front(make_ringpart(w,true));
                r.first= first_point(r.second)==last_point(r.second);
                return;
            } else if (b==first_point(r.second)) {
                r.second.push_front(make_ringpart(w,false));
                r.first= first_point(r.second)==last_point(r.second);
                return;
            }
        }
    }
    rings.push_back(std::make_pair(false,std::deque<Ring::Part>{make_ringpart(w,false)}));
}

void extend_rings(tempringvec& rings, const tempring& rp) {
    int64 a=first_point(rp.second);
    int64 b=last_point(rp.second);

    if (a==b) {
        rings.push_back(rp);
        return;
    }
    if (!rings.empty()) {

        for (auto& r: rings) {
            if (r.first) {
                //skip
            } else if (a == last_point(r.second)) {
                for (size_t i = 0; i < rp.second.size(); i++) {
                    r.second.push_back(rp.second[i]);
                }
                r.first= first_point(r.second)==last_point(r.second);
                return;
            } else if (b == last_point(r.second)) {
                for (size_t i = 0; i < rp.second.size(); i++) {
                    auto q = rp.second[rp.second.size()-1-i];
                    q.reversed = !q.reversed;
                    r.second.push_back(q);
                }
                r.first= first_point(r.second)==last_point(r.second);
                return;
            } else if (a==first_point(r.second)) {
                for (size_t i = 0; i < rp.second.size(); i++) {
                    auto q = rp.second[i];
                    q.reversed = !q.reversed;
                    r.second.push_front(q);
                }
                r.first= first_point(r.second)==last_point(r.second);
                return;
            } else if (b==first_point(r.second)) {
                for (size_t i = 0; i < rp.second.size(); i++) {
                    auto q = rp.second[rp.second.size()-1-i];
                    r.second.push_front(q);
                }
                r.first= first_point(r.second)==last_point(r.second);
                return;
            }
        }
    }
    rings.push_back(rp);
}

std::vector<std::pair<bool,std::deque<Ring::Part>>> merge_rings(const std::vector<std::pair<bool,std::deque<Ring::Part>>>& in) {

    std::vector<std::pair<bool,std::deque<Ring::Part>>> rings;
    for (auto r : in) {
        extend_rings(rings, r);
    }
    return rings;
}


bool is_ring(const Ring& ring) {
    auto ll=ringpart_refs(ring);
    return ((ll.size()>3) && (ll.front()==ll.back()));
}

std::pair<std::vector<Ring>,std::vector<std::pair<bool,Ring>>> make_rings(const std::vector<std::pair<bool,std::shared_ptr<WayWithNodes>>>& ways, bool is_inner, const bbox& box) {
    std::vector<std::pair<bool,std::deque<Ring::Part>>> rings;
    for (auto w : ways) {
        if (w.first==is_inner) {
            add_to_rings(rings, w.second);
        }
        
    }
    size_t rl = rings.size();
    rings = merge_rings(rings);
    while (rl!= rings.size()) {
        rl = rings.size();
        rings = merge_rings(rings);
    }


    std::vector<Ring> result;
    std::vector<std::pair<bool,Ring>> passes;
    for (auto& r: rings) {
        bbox ringbox;
        for (auto p : r.second) {
            for (auto ll : p.lonlats) {
                expand_point(ringbox, ll.lon,ll.lat);
            }
        }
        if (r.first) {
            if (overlaps(box, ringbox)) {
                auto rng = Ring{std::vector<Ring::Part>(r.second.begin(),r.second.end())};
                if (!is_ring(rng)) {
                    passes.push_back(std::make_pair(true, rng));
                } else {
                    result.push_back(rng);
                }
                
            } else {
                passes.push_back(std::make_pair(true,Ring{std::vector<Ring::Part>(r.second.begin(),r.second.end())}));
            }
        } else {
            passes.push_back(std::make_pair(false,Ring{std::vector<Ring::Part>(r.second.begin(),r.second.end())}));
        }
    }
    return std::make_pair(result,passes);
}

bool ring_contains(const std::vector<LonLat>& outer, const std::vector<LonLat>& inner) {
    for (auto ll : inner) {
        if (!point_in_poly(outer,ll)) {
            return false;
        }
    }
    return true;
}


    
bool check_parts(const std::pair<std::vector<LonLat>,std::vector<Ring>>& part) {
    if (part.first.size()<4) { return false; }
    if (!part.second.empty()) {
        for (const auto& r: part.second) {
            if (!is_ring(r)) {
                return false;
            }
        }
    }
    return true;
}   



class MakeMultiPolygons : public BlockHandler {
    struct pendingrel {
        int64 tile_quadtree;
        std::shared_ptr<Relation> rel;
    };

    struct pendingway {
        int64 tile_quadtree;
        std::shared_ptr<WayWithNodes> wy;
        //std::set<int64> rels;
        int rels;
        //std::set<std::string> finished_tags;
    };

    std::map<int64,pendingrel> pendingrels;
    std::map<int64,pendingway> pendingways;
    
    std::function<void(mperrorvec&)> errors_callback;
    mperrorvec errors;
    
    GeometryTagsParams params;
    
    bbox box;
    int64 maxqt;
    int compcount;
    bool boundary, multipolygon;
    int64 max_number_errors;
    //std::string extra_tags_key;
    bool allow_empty_role;
    public:
        MakeMultiPolygons(
            std::function<void(mperrorvec&)> errors_callback_,
            const GeometryTagsParams& params_,
            const bbox& box_,
            bool boundary_,
            bool multipolygon_,
            int64 max_number_errors_) : 
                errors_callback(errors_callback_),
                params(params_),
                box(box_), maxqt(-1),
                compcount(0), boundary(boundary_),
                multipolygon(multipolygon_),
                max_number_errors(max_number_errors_)
            {
                
                
                allow_empty_role=true;
            }

        virtual primblock_vec process(primblock_ptr in) {

            
            primblock_vec res;
            if (in->Quadtree() > maxqt) {
                res = check_finished(in->Quadtree());
                maxqt = in->Quadtree();
            }

            

            std::vector<ElementPtr> tempobjs;
            tempobjs.reserve(in->size());
            for (auto o : in->Objects()) {
                if (o->Type()==ElementType::Relation) {
                    auto ty=get_tag(o,"type");

                    if ((boundary && (ty=="boundary")) || (multipolygon && (ty=="multipolygon"))) {

                        auto r = std::dynamic_pointer_cast<Relation>(o);
                        if (r->Members().size()==0) { continue; }

                        for (const auto& m : r->Members()) {
                            if (m.type==ElementType::Way) {
                                auto& pw = pendingways[m.ref];
                                pw.rels+=1;
                            }
                        }
                        pendingrels[o->Id()] = pendingrel{in->Quadtree(),r};
                    }
                } else {
                    
                    
                    tempobjs.push_back(o);
                }

            }
            
            for (auto& o: tempobjs) {
                if ((o->Type()==ElementType::WayWithNodes) && (pendingways.count(o->Id())>0)) {
                    pendingways[o->Id()].tile_quadtree=in->Quadtree();
                    pendingways[o->Id()].wy = std::dynamic_pointer_cast<WayWithNodes>(o);

                }
            }
        
        
            in->Objects().swap(tempobjs);
            res.push_back(in);
            return res;
        }
        virtual ~MakeMultiPolygons() {}

        virtual primblock_vec finish() {
            auto ff = check_finished(-1);
            errors_callback(errors);
            return ff;
        }
    private:
        primblock_vec check_finished(int64 qt) {
           

            std::map<int64,primblock_ptr> finished;
            size_t cc=0;

            for (auto it=pendingrels.cbegin(); it!=pendingrels.cend(); /*no ince*/) {
                int64 tq=it->second.tile_quadtree;
                if (finished.count(tq)==0) {
                    finished[tq]=std::make_shared<PrimitiveBlock>(0,0);
                }
                
                if ((qt<0) || (quadtree::common(tq,qt)!=tq)) {
                    
                    cc += finish_relation(it->second, finished);
                    it=pendingrels.erase(it);


                } else {
                    it++;
                }
            }


            if (qt<0) {
                if (!pendingways.empty()) {
                    Logger::Message() << "at null, have " << pendingways.size() << " remaining";
                }

            }
            
            
            size_t mm=0;
            primblock_vec result;
            for (const auto& ff: finished) {
                ff.second->SetQuadtree(ff.first);
                if (ff.second->size()>0) {
                    result.push_back(ff.second);
                    mm+=ff.second->size();
                }
            }
            
            
            return result;
        }



        size_t finish_relation(const pendingrel& pr, std::map<int64,primblock_ptr>& finished) {
            auto tq=pr.tile_quadtree;
            size_t cc=0;
            
            auto r=pr.rel;
            //bool is_bp = get_tag(r,"type")=="boundary";
            
            auto ways = collect_ways(r);
            
            
            auto res = process_multipolygon(params, box, r, ways);
            
            for (auto& m: pr.rel->Members()) {
                if (m.type==ElementType::Way) {
                    auto jt = pendingways.find(m.ref);
                    if (jt==pendingways.end()) {
                        Logger::Message() << "relation " << pr.rel->Id() << " missing way " << m.ref;
                    } else {
                        jt->second.rels--;
                        if (jt->second.rels==0) {
                            
                            pendingways.erase(jt);
                            cc++;
                        }
                    }



                }
            }
            
            if (res.first) {
                finished[tq]->add(res.first);
            }
            if (res.second) {
                errors.count+=1;
                if ((int64) errors.errors.size() < max_number_errors) {
                    errors.errors.push_back(*res.second);
                }
            }
            return cc;
        }
            

        std::vector<std::pair<bool,std::shared_ptr<WayWithNodes>>> collect_ways(std::shared_ptr<Relation> rel) {
            std::vector<std::pair<bool,std::shared_ptr<WayWithNodes>>> ways;
            for (const auto& m: rel->Members()) {
                if ((m.type==ElementType::Way)) {
                    auto w = pendingways[m.ref].wy;
                    if (w) {
                        if (m.role=="inner") { ways.push_back(std::make_pair(true,w)); }
                        else if (m.role=="outer") { ways.push_back(std::make_pair(false,w)); }
                        else if (m.role.empty() && allow_empty_role) { ways.push_back(std::make_pair(false,w)); }
                        else { continue; }
                    }

                }
            }

            return ways;
        }
        

        

};

std::pair<std::shared_ptr<ComplicatedPolygon>, std::optional<mperror>> process_multipolygon(
    const GeometryTagsParams& params, const bbox& box,
    std::shared_ptr<Relation> r,
    const std::vector<std::pair<bool,std::shared_ptr<WayWithNodes>>>& ways) {
    
    
    std::shared_ptr<ComplicatedPolygon> cp;
    std::optional<mperror> error;
    
    
    std::vector<Ring> outers,inners;
    std::vector<std::pair<bool,Ring>> passes;
    std::vector<std::pair<bool,Ring>> passes2;
    
    std::tie(outers,passes) = make_rings(ways,false,box);
    std::tie(inners,passes2) = make_rings(ways,true,box);
    if (!passes2.empty()) {
        std::copy(passes2.begin(),passes2.end(),std::back_inserter(passes));

    }
    

    std::string err="";
    //std::stringstream err;
    //err << "relation " << pr.rel->Id() << " " << is_bp << " ";
    if (!passes.empty()) {
        err = "incomplete rings";
    }
    bool all_outofbox=true;
    for (auto& p : passes) {
        if (!p.first) { all_outofbox=false; }
    }

    if (!outers.empty()) {
        
        
        bool tags_pass; std::vector<Tag> tags; std::optional<int64> layer;
        std::tie(tags_pass, tags, layer) = filter_tags(params.feature_keys, params.other_keys, params.drop_keys, params.all_other_keys, params.all_objs, r->Tags());
        auto z_order=calc_zorder(tags);
        
        if (!params.drop_keys.empty()) {
            std::vector<Tag> tagsf;
            for (const auto& t: tags) {
                if (t.key!="type") {
                    tagsf.push_back(t);
                }
            }
            tags.swap(tagsf);
        }
        //tagvector tgs; bool isring; int64 zorder, layer;
        //std::tie(tgs,isring,zorder,layer) = filter_way_tags(style, r->Tags(),true, is_bp,extra_tags_key);

        //if (!tgs.empty() && isring) {
        if (tags_pass) {

            std::map<size_t,std::pair<std::vector<LonLat>,std::vector<Ring>>> parts;
            for (size_t i=0; i < outers.size(); i++) {
                parts[i] = std::make_pair(ringpart_lonlats(outers[i]),std::vector<Ring>(0));
                
                
            }
            if (!inners.empty()) {
                for (const auto& r : inners) {
                    bool added=false;
                    auto ll = ringpart_lonlats(r);
                    for (auto& pp : parts) {
                        if (!added && ring_contains(pp.second.first, ll)) {
                            pp.second.second.push_back(r);
                            added=true;
                        }
                    }
                    if (!added) {
                        if (!err.empty()) { err+=", "; }
                        err += "orphan inner";
                    }
                }
            }
            
            std::vector<PolygonPart> checked;
            for (const auto& pp : parts) {
                
                if (!check_parts(pp.second)) {
                    Logger::Message() << "invalid polygon part " << r->Id() << " " << pp.first;
                    continue;
                }
                checked.push_back(PolygonPart(checked.size(), outers[pp.first], pp.second.second, 0));
            }
            
            if (checked.empty()) {
                Logger::Message() << "no polygons for " << r->Id();
            } else {
                cp = std::make_shared<ComplicatedPolygon>(r, checked,tags,z_order,layer,std::optional<int64>());
                
            }

            
        } else {
            //err += "?? isring=";
            //if (isring) { err += "y"; } else { err += "n"; }
        }
    } else {
        if (inners.empty() && (all_outofbox || passes.empty())) {
            err="";
        } else {
            err = "no outers!!!";
        }
    }
    
    

    if (!err.empty()) {
        
        error = std::make_tuple(r, err, outers, inners, passes);
    }
    
    return std::make_pair(cp,error);
}
        




std::shared_ptr<BlockHandler> make_multipolygons(
    std::function<void(mperrorvec&)> add_error,
    const std::set<std::string>& feature_keys,  
    const std::set<std::string>& other_keys,
    const std::set<std::string>& drop_keys,
    bool all_other_keys,
    bool all_objs,
    const bbox& box,
    bool boundary, bool multipolygon, int64 max_number_errors) {

    GeometryTagsParams params;
    params.feature_keys=feature_keys;
    params.other_keys=other_keys;
    params.drop_keys=drop_keys;
    params.all_other_keys=all_other_keys;
    params.all_objs=all_objs;
    
    

    return std::make_shared<MakeMultiPolygons>(add_error,params, box,boundary,multipolygon,max_number_errors);
}


}}
