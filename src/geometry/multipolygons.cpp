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



bool is_ring(std::shared_ptr<way_withnodes> w) {
    return (w->Refs().size()>2) && (w->Refs().front()==w->Refs().back());
}



template <class T>
int64 first_point(const T& r) {
    const ringpart& r0 = r.front();
    if (r0.reversed) {
        return r0.refs.back();
    }
    return r0.refs.front();
}

template <class T>
int64 last_point(const T& r) {
    const ringpart& r1 = r.back();
    if (r1.reversed) {
        return r1.refs.front();
    }
    return r1.refs.back();
}


ringpart make_ringpart(std::shared_ptr<way_withnodes> w, bool d) {
    return ringpart{w->Id(),w->Refs(),w->LonLats(), d};
}

std::ostream& ringstr(std::ostream& strm, const ringpartvec& ring) {
    strm << ring.size() << " parts, ";
    for (const auto& p: ring) {
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
typedef std::pair<bool,std::deque<ringpart>> tempring;
typedef std::vector<tempring> tempringvec;

void add_to_rings(tempringvec& rings, std::shared_ptr<way_withnodes> w) {
    if (is_ring(w)) {
        rings.push_back(std::make_pair(true,std::deque<ringpart>{make_ringpart(w,false)}));
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
    rings.push_back(std::make_pair(false,std::deque<ringpart>{make_ringpart(w,false)}));
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

std::vector<std::pair<bool,std::deque<ringpart>>> merge_rings(const std::vector<std::pair<bool,std::deque<ringpart>>>& in) {

    std::vector<std::pair<bool,std::deque<ringpart>>> rings;
    for (auto r : in) {
        extend_rings(rings, r);
    }
    return rings;
}



std::pair<std::vector<ringpartvec>,std::vector<std::pair<bool,ringpartvec>>> make_rings(const std::vector<std::shared_ptr<way_withnodes>>& ways, const bbox& box) {
    std::vector<std::pair<bool,std::deque<ringpart>>> rings;
    for (auto w : ways) {
        add_to_rings(rings, w);
    }
    size_t rl = rings.size();
    rings = merge_rings(rings);
    while (rl!= rings.size()) {
        rl = rings.size();
        rings = merge_rings(rings);
    }


    std::vector<ringpartvec> result;
    std::vector<std::pair<bool,ringpartvec>> passes;
    for (auto& r: rings) {
        bbox ringbox;
        for (auto p : r.second) {
            for (auto ll : p.lonlats) {
                ringbox.expand_point(ll.lon,ll.lat);
            }
        }
        if (r.first) {
            if (overlaps(box, ringbox)) {
                result.push_back(ringpartvec(r.second.begin(),r.second.end()));
            } else {
                passes.push_back(std::make_pair(true,ringpartvec(r.second.begin(),r.second.end())));
            }
        } else {
            passes.push_back(std::make_pair(false,ringpartvec(r.second.begin(),r.second.end())));
        }
    }
    return std::make_pair(result,passes);
}

bool ring_contains(const std::vector<lonlat>& outer, const std::vector<lonlat>& inner) {
    for (auto ll : inner) {
        if (!point_in_poly(outer,ll)) {
            return false;
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
        std::shared_ptr<way_withnodes> wy;
        //std::set<int64> rels;
        int rels;
        //std::set<std::string> finished_tags;
    };

    std::map<int64,pendingrel> pendingrels;
    std::map<int64,pendingway> pendingways;
    std::shared_ptr<mperrorvec> errors;
    style_info_map style;
    bbox box;
    int64 maxqt;
    int compcount;
    bool boundary, multipolygon;
    std::string extra_tags_key;
    bool allow_empty_role;
    public:
        MakeMultiPolygons(
            std::shared_ptr<mperrorvec> errors_,
            const style_info_map& style_,
            const bbox& box_,
            bool boundary_,
            bool multipolygon_) : 
                errors(errors_), style(style_), box(box_), maxqt(-1),
                compcount(0), boundary(boundary_),
                multipolygon(multipolygon_)
            {
                //all_tags = style.count("*")!=0;
                extra_tags_key = "";
                for (const auto& st: style) {
                    if (st.second.IsOtherTags) {
                        extra_tags_key=st.first;
                    }
                }
                
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
                    pendingways[o->Id()].wy = std::dynamic_pointer_cast<way_withnodes>(o);

                }
            }
        
        
            in->Objects().swap(tempobjs);
            res.push_back(in);
            return res;
        }
        virtual ~MakeMultiPolygons() {}

        virtual primblock_vec finish() {
            return check_finished(-1);
        }
    private:




        size_t finish_relation(const pendingrel& pr, std::map<int64,primblock_ptr>& finished) {
            auto tq=pr.tile_quadtree;
            size_t cc=0;
            
            auto r=pr.rel;
            bool is_bp = get_tag(r,"type")=="boundary";
            std::vector<ringpartvec> outers,inners;
            std::vector<std::pair<bool,ringpartvec>> passes;
            std::tie(outers,inners,passes) = collect_rings(r);

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
                


                tagvector tgs; bool isring; int64 zorder, layer;
                std::tie(tgs,isring,zorder,layer) = filter_way_tags(style, r->Tags(),true, is_bp,extra_tags_key);

                if (!tgs.empty() && isring) {


                    std::map<size_t,std::pair<lonlatvec,std::vector<ringpartvec>>> parts;
                    for (size_t i=0; i < outers.size(); i++) {
                        parts[i] = std::make_pair(ringpart_lonlats(outers[i]),std::vector<ringpartvec>(0));
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

                    for (const auto& pp : parts) {
                        auto cp = std::make_shared<complicatedpolygon>(r, pp.first, outers[pp.first], pp.second.second,tgs,zorder,layer,-1);
                        finished[tq]->add(cp);

                        
                    }
                } else {
                    err += "?? isring=";
                    if (isring) { err += "y"; } else { err += "n"; }
                }
            } else {
                if (inners.empty() && (all_outofbox || passes.empty())) {
                    err="";
                } else {
                    err = "no outers!!!";
                }
            }



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

            if (!err.empty()) {
                if (errors && (errors->size()<10000)) {
                    errors->push_back(std::make_tuple(pr.rel,err,outers,inners,passes));
                }
            }
            return cc;
        }





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

        std::tuple<std::vector<ringpartvec>, std::vector<ringpartvec>,std::vector<std::pair<bool,ringpartvec>>> collect_rings(std::shared_ptr<Relation> rel) {
            std::vector<std::shared_ptr<way_withnodes>> outers, inners;
            for (const auto& m: rel->Members()) {
                if ((m.type==ElementType::Way)) {
                    auto w = pendingways[m.ref].wy;
                    if (w) {
                        if (m.role=="inner") { inners.push_back(w); }
                        else if (m.role=="outer") { outers.push_back(w); }
                        else if (m.role.empty() && allow_empty_role) { outers.push_back(w); }
                        else { continue; }
                    }

                }
            }
            auto oo = make_rings(outers,box);
            auto ii = make_rings(inners,box);
            if (!ii.second.empty()) {
                std::copy(ii.second.begin(),ii.second.end(),std::back_inserter(oo.second));

            }
            return std::make_tuple(oo.first,ii.first,oo.second);
            
        }




};

std::shared_ptr<BlockHandler> make_multipolygons(
    std::shared_ptr<mperrorvec> multipolygon_errors,
    const style_info_map& style, const bbox& box,
    bool boundary, bool multipolygon) {

    return std::make_shared<MakeMultiPolygons>(multipolygon_errors,style,box,boundary,multipolygon);
}


}}
