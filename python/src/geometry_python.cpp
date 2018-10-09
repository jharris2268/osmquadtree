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

#include "oqt_python.hpp"
#include "oqt/utils/splitcallback.hpp"

#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"

#include <cmath> 
using namespace oqt;
std::string join_strs(const std::vector<std::string>& strs, std::string jj) {
    std::stringstream out;
    for (const auto& s : strs) {
        out << s << jj;
    }
    return out.str();
}


typedef std::vector<std::tuple<std::string,bool,bool,bool>> coltag_spec;


class find_network {
    public:
        find_network(size_t maxnd_, std::set<std::pair<std::string,std::string>> tgs_, bool locs_) : maxnd(maxnd_), tgs(tgs_), numnodes(0), locs(locs_) {
            netnodes.resize(maxnd);
        }

        bool add_blocks(std::vector<PrimitiveBlockPtr> bls) {
            py::gil_scoped_release r;
            try {

                if (allnodes.empty()) {
                    std::cout << "resize allnodes to " << maxnd << std::endl;
                    allnodes.resize(maxnd);
                }
                //std::cout << "have " << bls.size() << " blocks" << std::endl;
                size_t a=0;
                for (auto bl : bls) {
                    if (!bl->Objects().empty()) {
                        for (auto o : bl->Objects()) {
                            if (check_tags(o)) {
                                if (o->Type()==ElementType::Way) {
                                    auto w = std::dynamic_pointer_cast<Way>(o);
                                    check_nodes(w->Refs(),{});
                                    a++;
                                } else if (o->Type()==ElementType::Linestring) {
                                    auto ls = std::dynamic_pointer_cast<geometry::Linestring>(o);
                                    check_nodes(ls->Refs(),ls->LonLats());
                                    a++;
                                } else if (o->Type()==ElementType::SimplePolygon) {
                                    auto sp = std::dynamic_pointer_cast<geometry::SimplePolygon>(o);
                                    check_nodes(sp->Refs(),sp->LonLats());
                                    a++;
                                } else if (o->Type()==ElementType::WayWithNodes) {
                                    auto ls = std::dynamic_pointer_cast<geometry::WayWithNodes>(o);
                                    check_nodes(ls->Refs(),ls->LonLats());
                                    a++;
                                }
                            }
                        }
                    }
                }
                //std::cout << "added " << a << " objs, have " << size() << " netnodes " << std::endl;
            } catch (std::exception& ex) {
                std::cout << "failed " << ex.what() << std::endl;
            }
            return true;
        }
        bool check_tags(ElementPtr o) {
            if (tgs.empty()) { return true; }
            for (const auto& t : o->Tags()) {
                if (tgs.count(std::make_pair(t.key,t.val))>0) {
                    return true;
                }
            }
            return false;
        }
        void check_nodes(const refvector& rfs, const lonlatvec& lls) {
            bool cl = locs && (lls.size()==rfs.size());
            for (size_t i=0; i < rfs.size(); i++) {
                auto r = rfs.at(i);
                if (r >= maxnd) {
                    allnodes.resize(r + 1024*1024);
                    netnodes.resize(r + 1024*1024);
                }
                if (allnodes[r]) {
                    addnode(r);
                    if (cl) { locsmap[r] = lls.at(i); }
                }
                allnodes[r] = true;
                
            }
            addnode(rfs.front());
            addnode(rfs.back());
            if (cl) {
                locsmap[rfs.front()] = lls.front();
                locsmap[rfs.back()] = lls.back();
            }
            
        }
        void addnode(int64 r) {
            if (!netnodes[r]) {
                netnodes[r]=true;
                numnodes++;
            }
        }

        bool contains(int64 r) {
            if (!allnodes.empty()) {
                std::vector<bool> b;
                allnodes.swap(b);
            }
            if (r<0) { return false; }
            if (r>= maxnd) { return false; }
            return netnodes[r];
        }
        size_t size() { return numnodes; }

        std::set<int64> to_set() {
            std::set<int64> res;
            for (int64 i=0; i < maxnd; i++) {
                if (netnodes[i]) {
                    res.insert(i);
                }
            }
            return res;
        }
        
        lonlat location(int64 r) {
            auto it = locsmap.find(r);
            if (it==locsmap.end()) {
                return lonlat{0,0};
            }
            return it->second;
        }

    private:
        std::vector<bool> allnodes;
        std::vector<bool> netnodes;
        int64 maxnd;

        std::set<std::pair<std::string,std::string>> tgs;
        size_t numnodes;
        bool locs;
        std::map<int64,lonlat> locsmap;
};

refvector element_refs(ElementPtr ele) {
    if (ele->Type()==ElementType::Linestring) {
        auto ls = std::dynamic_pointer_cast<geometry::Linestring>(ele);
        if (!ele) { throw std::domain_error("not a linestring"); }
        return ls->Refs();
    }
    if (ele->Type()==ElementType::SimplePolygon) {
        auto ls = std::dynamic_pointer_cast<geometry::SimplePolygon>(ele);
        if (!ele) { throw std::domain_error("not a simplepolygon"); }
        return ls->Refs();
    }
    if (ele->Type()==ElementType::WayWithNodes) {
        auto ls = std::dynamic_pointer_cast<geometry::WayWithNodes>(ele);
        if (!ele) { throw std::domain_error("not a WayWithNodes"); }
        return ls->Refs();
    }
    throw std::domain_error("no refs");
    return {};
}

lonlatvec element_lonlats(ElementPtr ele) {
    if (ele->Type()==ElementType::Linestring) {
        auto ls = std::dynamic_pointer_cast<geometry::Linestring>(ele);
        if (!ele) { throw std::domain_error("not a linestring"); }
        return ls->LonLats();
    }
    if (ele->Type()==ElementType::SimplePolygon) {
        auto ls = std::dynamic_pointer_cast<geometry::SimplePolygon>(ele);
        if (!ele) { throw std::domain_error("not a simplepolygon"); }
        return ls->LonLats();
    }
    if (ele->Type()==ElementType::WayWithNodes) {
        auto ls = std::dynamic_pointer_cast<geometry::WayWithNodes>(ele);
        if (!ele) { throw std::domain_error("not a WayWithNodes"); }
        return ls->LonLats();
    }
    throw std::domain_error("no refs");
    return {};
}



class lines_graph : public std::enable_shared_from_this<lines_graph> {
    public:
        struct edge {
            int64 from, to;
            int64 keyi;
            ElementPtr ele;
            int64 firstnode, lastnode;
            double length;
            refvector refs() {
                refvector res;
                auto oo=element_refs(ele);
                if (lastnode > firstnode) {
                    for (int64 i=firstnode; i <= lastnode; i++) {
                        res.push_back(oo.at(i));
                    }
                } else {
                    for (int64 i=firstnode; i >= lastnode; i--) {
                        res.push_back(oo.at(i));
                    }
                }
                return res;
            }

            lonlatvec lonlats() {
                lonlatvec res;
                auto oo=element_lonlats(ele);
                if (lastnode > firstnode) {
                    for (int64 i=firstnode; i <= lastnode; i++) {
                        res.push_back(oo.at(i));
                    }
                } else {
                    for (int64 i=firstnode; i >= lastnode; i--) {
                        res.push_back(oo.at(i));
                    }
                }
                return res;
            }
        };
        typedef std::multimap<int64,size_t>::const_iterator mapiter;
        class edge_iter {
            public:
                edge_iter(std::shared_ptr<lines_graph> lg_, lines_graph::mapiter itBeg_, lines_graph::mapiter itEnd_, const std::set<int64>& keys)
                    : lg(lg_), itBeg(itBeg_), itEnd(itEnd_) {
                        reset();
                        
                    check = [](const edge& e) { return true; };
                    if (!keys.empty()) {
                        check = [keys](const edge& e) { return keys.count(e.keyi)>0; };
                    }
                }
                
                void reset() {
                    it = itBeg;
                }
                
                std::vector<lines_graph::edge> next() {
                    std::vector<lines_graph::edge> res;
                    while (it != itEnd) {
                        int64 k = it->first;
                        while ((it!= itEnd) && (it->first==k)) {
                            auto e = lg->edge_at(it->second);
                            if (check(e)) {
                                res.push_back(e);
                            }
                            ++it;
                        }
                        if (!res.empty()) {
                            return res;
                        }
                    }
                    throw py::stop_iteration();
                    
                }
                
                
            private:
                std::shared_ptr<lines_graph> lg;
                lines_graph::mapiter itBeg, itEnd, it;
                
                std::function<bool(const edge&)> check;
        };
    
        lines_graph(std::shared_ptr<find_network> net_) : net(net_) {}
        
        size_t add_lines(const std::string& key, PrimitiveBlockPtr block, bool incpolys) {
            if (!block || block->Objects().empty()) { return 0; }
            size_t cc=0;
            for (auto ele: block->Objects()) {
                if (ele->Type()==ElementType::Linestring) {
                    auto ls = std::dynamic_pointer_cast<geometry::Linestring>(ele);
                    if (!ls) { throw std::domain_error("?? not a line"); }
                    cc += add_line(key, ele, ls->Refs());
                } else if (incpolys&& (ele->Type()==ElementType::SimplePolygon)) {
                    auto sp = std::dynamic_pointer_cast<geometry::SimplePolygon>(ele);
                    if (!sp) { throw std::domain_error("?? not a line"); }
                    cc += add_line(key, ele, sp->Refs());
                }
            }
            return cc;
        }
                  
        size_t add_line(const std::string& key, ElementPtr ele, const refvector& refs) {
            
            int64 keyi = find_key(key);
            
            auto pls = find_places(refs);
            if (pls.size()<2) { return 0; }
            for (size_t i=0; i < (pls.size()-1); i++) {
                add_edge(pls[i].first, pls[i+1].first, keyi, ele, pls[i].second, pls[i+1].second, true);
            }
            return pls.size()-1;
        }
        
        void add_edge(int64 from, int64 to, int64 keyi, ElementPtr ele, int64 a, int64 b, bool rev) {
            size_t ei = edges.size();
            
            edge e{from,to,keyi,ele,a,b,0};
            e.length = geometry::calc_line_length(e.lonlats());
            
            edges.push_back(e);
            forward.insert(std::make_pair(from,ei));
            backward.insert(std::make_pair(to,ei));
            
            
            edges.push_back(edge{e.to,e.from,e.keyi,e.ele,e.lastnode,e.firstnode,e.length});
            forward.insert(std::make_pair(to,ei+1));
            backward.insert(std::make_pair(from,ei+1));
            
            
        }
        
        std::vector<edge> forward_edges(int64 from, std::function<bool(const edge&)> check) {
            
            std::vector<edge> res;
            auto it = forward.lower_bound(from);
            
            while ((it != forward.end()) && (it->first == from)) {
                auto e = edges.at(it->second);
                if (check(e)) {
                    res.push_back(e);
                }
                ++it;
            }
            return res;
        }
        
        std::vector<edge> backward_edges(int64 to, std::function<bool(const edge&)> check) {
            
            std::vector<edge> res;
            auto it = backward.lower_bound(to);
            
            while ((it != backward.end()) && (it->first == to)) {
                auto e = edges.at(it->second);
                if (check(e)) {
                    res.push_back(e);
                }
                ++it;
            }
            return res;
        }
        
        size_t num_edges() { return edges.size(); }
        size_t num_nodes() {
            size_t c=0;
            int64 n=0;
            for (const auto& p: forward) {
                if (p.first != n) {
                    c++;
                }
                n=p.first;
            }
            return c;
        }
        
        int64 find_key(const std::string& k) {
            auto it = keysmap.find(k);
            if (it!=keysmap.end()) { return it->second; }
            
            int64 r=keys.size();
            keysmap[k]=r;
            keys.push_back(k);
            return r;
        }
        
        std::string key(int64 i) { return keys.at(i); }
        
        edge edge_at(size_t i) {
            return edges.at(i);
        }
        
        
        edge_iter forward_iter(std::set<int64> keys) { return edge_iter(shared_from_this(), forward.begin(), forward.end(), keys); }
        edge_iter backward_iter(std::set<int64> keys) { return edge_iter(shared_from_this(), backward.begin(), backward.end(), keys); }
        
        
        const std::map<std::string,int64>& get_keysmap() { return keysmap; }
        
        
        std::vector<std::set<int64>> connected_components(std::set<int64> keys) {
            std::vector<std::set<int64>> result;
            std::set<int64> seen;
            std::function<bool(const edge&)> check = [](const edge& e) { return true; };
            if (!keys.empty()) {
                check = [&keys](const edge& e) { return keys.count(e.keyi)>0; };
            }
            
            
            for (const auto& p: forward) {
                auto e = edge_at(p.second);
                if (check(e) && seen.count(e.from)==0) {
                    auto vv = plain_bfs(e.from, check);
                    std::copy(vv.begin(),vv.end(),std::inserter(seen,seen.end()));
                    result.push_back(vv);
                }
            }
            
            std::sort(result.begin(),result.end(),[](const std::set<int64>& l,std::set<int64>& r) { return (-1*l.size())<(-1*r.size()); });
            return result;
        }
        std::set<int64> plain_bfs(int64 source, std::function<bool(const edge&)> check) {
            std::set<int64> seen;
            std::set<int64> nextlevel; nextlevel.insert(source);
            
            while (!nextlevel.empty()) {
                std::set<int64> thislevel;
                thislevel.swap(nextlevel);
                for (auto i: thislevel) {
                    if (seen.count(i)==0) {
                        seen.insert(i);
                        for (auto e: forward_edges(i,check)) {
                            if (seen.count(e.to)==0 ) {
                                nextlevel.insert(e.to);
                            }
                        }
                    }
                }
            }
            return seen;
        }
        
    private:
        std::shared_ptr<find_network> net;
    
        std::vector<edge> edges;
        std::multimap<int64,size_t> forward;
        std::multimap<int64,size_t> backward;
        
        std::map<std::string,int64> keysmap;
        std::vector<std::string> keys;
        
        std::vector<std::pair<int64,int64>> find_places(const refvector& fr) {
            std::vector<std::pair<int64,int64>> pls;
            for (size_t i=0; i < fr.size(); i++) {
                if (net->contains(fr.at(i))) {
                    pls.push_back(std::make_pair(fr.at(i),i));
                }
            }
            return pls;
        }
            
};

        
        



ElementPtr make_point(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt, int64 lon, int64 lat, int64 layer, int64 minzoom) {
    return std::make_shared<geometry::Point>(id, qt, inf, tags, lon, lat, layer, minzoom);
}

ElementPtr make_linestring(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt, refvector refs, lonlatvec lonlats, int64 zorder, int64 layer, int64 minzoom) {
    double ln = geometry::calc_line_length(lonlats);
    bbox bounds = geometry::lonlats_bounds(lonlats);
    return std::make_shared<geometry::Linestring>(id, qt, inf, tags, refs, lonlats, zorder, layer, ln, bounds, minzoom);
}

ElementPtr make_simplepolygon(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt, refvector refs, lonlatvec lonlats, int64 zorder, int64 layer, int64 minzoom) {
    bool reversed=false;
    double ar = geometry::calc_ring_area(lonlats);
    if (ar < 0) {
        ar*=-1;
        reversed=true;
    }
    bbox bounds = geometry::lonlats_bounds(lonlats);
    return std::make_shared<geometry::SimplePolygon>(id, qt, inf, tags, refs, lonlats, zorder, layer, ar, bounds, minzoom,reversed);
}

ElementPtr make_complicatedpolygon(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt,
    int64 part, geometry::Ring exterior, std::vector<geometry::Ring> interiors, int64 zorder, int64 layer, int64 minzoom) {
    double ar = geometry::calc_ring_area(exterior);
    if (!interiors.empty()) {
        for (const auto& ii : interiors) {
            ar -= geometry::calc_ring_area(ii);
        }
    }
    bbox bounds = geometry::lonlats_bounds(geometry::ringpart_lonlats(exterior));
    return std::make_shared<geometry::ComplicatedPolygon>(id, qt, inf, tags, part, exterior, interiors, zorder, layer, ar, bounds, minzoom);
}




double res_zoom(double res) {
    if (std::abs(res)<0.001) { return 20; }
    return log(earth_width*2/res/256) / log(2.0);
}
        

int64 area_zoom(double area, double minarea) {
    double zm = res_zoom(std::sqrt(area/minarea));
    return (int64) zm;
}
int64 length_zoom(double length, double minlen) {
    return res_zoom(length/minlen);
}

       
class findminzoom_onetag : public geometry::findminzoom {
    public:
        typedef std::map<std::tuple<int64,std::string,std::string>,std::pair<int64,int64>> tagmap;
        typedef std::vector<std::tuple<int64,std::string,std::string,int64>> tag_spec;
        findminzoom_onetag(const tag_spec& spec, double ml_, double ma_) :
            minlen(ml_), minarea(ma_) {
            
            for (const auto& t : spec) {
                int64 a,d; std::string b,c;
                std::tie(a,b,c,d) = t;
                tm.insert(std::make_pair(std::make_tuple(a,b,c),std::make_pair(d,0)));
                keys.insert(b);
            }
        }
        
        virtual int64 calculate(ElementPtr ele) {
            //int64 minzoom=100;
            int64 ty = (ele->Type()==ElementType::Point) ? 0 : (ele->Type()==ElementType::Linestring ? 1 : ((ele->Type()==ElementType::SimplePolygon) || (ele->Type()==ElementType::ComplicatedPolygon)) ? 2 : -1);
            if (ty==-1) { return -1; }
            
            auto curr_it = tm.end();
            
            for (const auto& t: ele->Tags()) {
                if (keys.count(t.key)>0) {
                    auto it = tm.find(std::make_tuple(ty,t.key,t.val));
                    if (it!=tm.end()) {
                        if ((curr_it==tm.end()) || (it->second.first < curr_it->second.first)) {
                            curr_it=it;
                        }
                    } else {
                        it = tm.find(std::make_tuple(ty,t.key,"*"));
                        if (it!=tm.end()) {
                            if ((curr_it==tm.end()) || (it->second.first < curr_it->second.first)) {
                                curr_it=it;
                            }
                        }
                    }
                }
            }
            if (curr_it==tm.end()) { return -1; }
            curr_it->second.second++;
            int64 minzoom = curr_it->second.first;
        
            int64 area_minzoom = areaminzoom(ele);
            if (area_minzoom > minzoom) {
                return area_minzoom;
            }
            return minzoom;
        }
        
        int64 areaminzoom(ElementPtr ele) {
        
            
            if ((ele->Type()==ElementType::Linestring) && (minlen>0)) {
                auto ls=std::dynamic_pointer_cast<geometry::Linestring>(ele);
                if (!ls) { throw std::domain_error("not a Linestring"); }
                double len = ls->Length();
                return length_zoom(len, minlen);
            }
            if ((ele->Type()==ElementType::SimplePolygon) && (minarea>0)) {
                auto sp = std::dynamic_pointer_cast<geometry::SimplePolygon>(ele);
                if (!sp) { throw std::domain_error("not a SimplePolygon"); }
                double area = sp->Area();
                return area_zoom(area, minarea);
            }
            if ((ele->Type()==ElementType::ComplicatedPolygon) && (minarea>0)) {
                auto cp=std::dynamic_pointer_cast<geometry::ComplicatedPolygon>(ele);
                if (!cp) { throw std::domain_error("not a ComplicatedPolygon"); }
                double area = cp->Area();
                return area_zoom(area, minarea);
            }
            return 0;
        }
        const tagmap& categories() { return tm; }
        
    private:
        tagmap tm;
        double minlen;
        double minarea;
        std::set<std::string> keys;
};

typedef std::function<void(PrimitiveBlockPtr)> block_callback;
typedef std::function<bool(std::vector<PrimitiveBlockPtr>)> external_callback;


void call_all(block_callback callback, geometry::primblock_vec bls) {
    for (auto bl : bls) {
        callback(bl);
    }
}

class blockhandler_callback_time {
    public:
        blockhandler_callback_time(const std::string& name_, std::shared_ptr<geometry::BlockHandler> handler_, block_callback callback_)
            : name(name_), handler(handler_), callback(callback_), wait(0), exec(0), cbb(0) {}
        
        void call(PrimitiveBlockPtr bl) {
            wait += ts.since_reset();
            if (!bl) {
                auto res = handler->finish();
                exec += ts.since_reset();
                call_all(callback, res);
                callback(nullptr);
                cbb += ts.since_reset();
                
                std::cout << name << ": wait=" << TmStr{wait,7,1} << ", exec=" << TmStr{exec,7,1} << ", call cb=" << TmStr{cbb,7,1} << std::endl;
                return;
            }
            
            auto res = handler->process(bl);
            exec += ts.since_reset();
            call_all(callback,res);
            cbb += ts.since_reset();
        }
        
        static block_callback make(const std::string& name, std::shared_ptr<geometry::BlockHandler> handler, block_callback callback) {
            auto bct = std::make_shared<blockhandler_callback_time>(name,handler,callback);
            return [bct](PrimitiveBlockPtr bl) {
                bct->call(bl);
            };
        }
        
    private:
        std::string name;
        std::shared_ptr<geometry::BlockHandler> handler;
        block_callback callback;
        
        double wait, exec, cbb;
        TimeSingle ts;
};
            

block_callback blockhandler_callback(std::shared_ptr<geometry::BlockHandler> handler, block_callback callback) {
    
    
    return [handler,callback](PrimitiveBlockPtr bl) {
        if (!bl) {
            call_all(callback, handler->finish());
            callback(nullptr);
            return;
        }
        call_all(callback, handler->process(bl));
        
    };
}

            
block_callback process_geometry_blocks(
    std::vector<block_callback> final_callbacks,
    const geometry::style_info_map& style, bbox box,
    const geometry::parenttag_spec_map& apt_spec, bool add_rels, bool add_mps,
    bool recalcqts, std::shared_ptr<geometry::findminzoom> findmz,
    std::function<void(geometry::mperrorvec&)> errors_callback) {
    
    
    auto errs = std::make_shared<geometry::mperrorvec>();
    
    std::vector<block_callback> makegeoms(final_callbacks.size());
    for (size_t i=0; i < final_callbacks.size(); i++) {
        auto finalcb = final_callbacks[i];
        if (i==0) {
            finalcb = [finalcb, errs, errors_callback](PrimitiveBlockPtr bl) {
                if (!bl && errs && (!errs->empty())) {
                    errors_callback(*errs);
                }
                finalcb(bl);
            };
        }
       
        makegeoms[i]  =threaded_callback<PrimitiveBlock>::make(
            blockhandler_callback_time::make("MakeGeometries["+std::to_string(i)+"]",
                geometry::make_geometryprocess(style,box,recalcqts,findmz),
                finalcb
            )
        );
        
        
    }
    block_callback makegeoms_split;
    if (final_callbacks.size()==1) {
        makegeoms_split = makegeoms[0];
    } else {
        makegeoms_split = split_callback<PrimitiveBlock>::make(makegeoms);
    }
    
    block_callback make_mps = makegeoms_split;
    if (add_mps) {
        
        bool bounds = style.count("boundary")>0;
        bool mps = false;
        for (const auto& s: style) {
            if ((s.first!="boundary") && (s.second.IsArea)) {
                mps=true;
                break;
            }
        }
        make_mps = threaded_callback<PrimitiveBlock>::make(
            blockhandler_callback_time::make("MultiPolygons", //blockhandler_callback(
                geometry::make_multipolygons(errs,style,box,bounds,mps),
                makegeoms_split
            )
        );
    }
            
    
    
    block_callback reltags = make_mps;
    if (add_rels) {
        bool add_bounds = style.count("boundary")>0;
        bool add_admin_levels=style.count("min_admin_level")>0;
        std::set<std::string> routes;
        for (const auto& st : style) {
            auto k = st.first;
            if ((k.size()>7) && (k.compare(k.size()-7, 7, "_routes")==0)) {
                routes.insert(k.substr(0,k.size()-7));
            }
        }
        std::cout << "make_handlerelations("
            << "add_bounds=" << (add_bounds ? "t" : "f") << ", "
            << "add_admin_levels=" << (add_admin_levels ? "t" : "f") << ", "
            << "routes={";
        bool f=true;
        for (const auto& r : routes) {
            if (!f) { std::cout << ", ";}
            std::cout << r;
            f=false;
        }
        std::cout <<"})" <<std::endl;
        
        reltags = threaded_callback<PrimitiveBlock>::make(
            blockhandler_callback_time::make("HandleRelations", //blockhandler_callback(
                geometry::make_handlerelations(add_bounds, add_admin_levels, routes),
                make_mps
            )
        );
    }        
    
    block_callback apt = reltags;
    if (!apt_spec.empty()) {
        apt = threaded_callback<PrimitiveBlock>::make(
            blockhandler_callback_time::make("AddParentTags", //blockhandler_callback(
                geometry::make_addparenttags(apt_spec),
                reltags
            )
        );
    }
        
    return threaded_callback<PrimitiveBlock>::make(geometry::make_addwaynodes_cb(apt));
        

    
}


block_callback process_geometry_blocks_nothread(
    block_callback final_callback,
    const geometry::style_info_map& style, bbox box,
    const geometry::parenttag_spec_map& apt_spec, bool add_rels, bool add_mps,
    bool recalcqts, std::shared_ptr<geometry::findminzoom> findmz,
    std::function<void(geometry::mperrorvec&)> errors_callback) {
    
    auto errs = std::make_shared<geometry::mperrorvec>();
    
    block_callback make_geom = [&style, box, final_callback, errs, errors_callback, recalcqts, findmz](PrimitiveBlockPtr bl) {
        if (!bl) {
            std::cout << "make_geom done" << std::endl;
            final_callback(bl);
            if ((errs) && (!errs->empty())) {
                errors_callback(*errs);
            }
            return;
        }
                
        auto gg = geometry::make_geometries(style,box, bl);
        if (recalcqts) {
            geometry::recalculate_quadtree(gg, 18, 0.05);
        }
        if (findmz) {
            geometry::calculate_minzoom(gg, findmz);
        }
        final_callback(gg);
            
    };
    
    block_callback make_mps = make_geom;
    if (add_mps) {
        
        bool bounds = style.count("boundary")>0;
        bool mps = false;
        for (const auto& s: style) {
            if ((s.first!="boundary") && (s.second.IsArea)) {
                mps=true;
                break;
            }
        }
        make_mps = blockhandler_callback(
            geometry::make_multipolygons(errs,style,box,bounds,mps),
            make_geom);
    }
    
        
    block_callback reltags = make_mps;
    if (add_rels) {
        bool add_bounds = style.count("boundary")>0;
        bool add_admin_levels=style.count("min_admin_level")>0;
        std::set<std::string> routes;
        for (const auto& st : style) {
            auto k = st.first;
            if ((k.size()>7) && (k.compare(k.size()-7, 7, "_routes")==0)) {
                routes.insert(k.substr(0,k.size()-7));
            }
        }
        std::cout << "make_handlerelations("
            << "add_bounds=" << (add_bounds ? "t" : "f") << ", "
            << "add_admin_levels=" << (add_admin_levels ? "t" : "f") << ", "
            << "routes={";
        bool f=true;
        for (const auto& r : routes) {
            if (!f) { std::cout << ", ";}
            std::cout << r;
            f=false;
        }
        std::cout <<"})" <<std::endl;
        
        reltags = blockhandler_callback(
                geometry::make_handlerelations(add_bounds, add_admin_levels, routes),
                make_mps
            );
    }        
    
    block_callback apt = reltags;
    if (!apt_spec.empty()) {
        apt = blockhandler_callback(
                geometry::make_addparenttags(apt_spec),
                reltags
            );
    }
        
    return geometry::make_addwaynodes_cb(apt);
   
}




        
    

std::vector<block_callback> pack_and_write_callback(
    std::vector<block_callback> callbacks,
    std::string filename, bool indexed, bbox box, size_t numchan,
    bool writeqts, bool writeinfos, bool writerefs) {
        


    auto head = std::make_shared<Header>();
    head->SetBBox(box);
    
    write_file_callback write = make_pbffilewriter_callback(filename,head,indexed);
    
    auto write_split = threaded_callback<std::pair<int64,std::string>>::make(write, numchan);
    
    std::vector<block_callback> pack(numchan);
    
    for (size_t i=0; i < numchan; i++) {
        auto write_i = write_split;//[i];
        block_callback cb;
        if (!callbacks.empty()) { cb = callbacks[i]; }
        pack[i] = //threaded_callback<primitiveblock>::make(
            [write_i, cb, writeqts,writeinfos,writerefs,i](PrimitiveBlockPtr bl) {
                if (cb) {
                    cb(bl);
                }
                if (!bl) {
                    std::cout << "pack_and_write_callback finish " << i << std::endl;
                    write_i(keystring_ptr());
                    return;
                } else {
                    auto data = writePbfBlock(bl, writeqts, false, writeinfos, writerefs);
                    auto packed = prepareFileBlock("OSMData", data);
                    write_i(std::make_shared<keystring>(bl->Quadtree(),packed));
                }
            };
        //);
    }
    return pack;
}



block_callback pack_and_write_callback_nothread(
    block_callback callback,
    std::string filename, bool indexed, bbox box,
    bool writeqts, bool writeinfos, bool writerefs) {
        


    auto head = std::make_shared<Header>();
    head->SetBBox(box);
    
    write_file_callback write = make_pbffilewriter_callback(filename,head,indexed);
    
    return [write, callback, writeqts,writeinfos,writerefs](PrimitiveBlockPtr bl) {
        if (callback) {
            callback(bl);
        }
        if (!bl) {
            write(keystring_ptr());
            return;
        } else {
            auto data = writePbfBlock(bl, writeqts, false, writeinfos, writerefs);
            
            auto packed = prepareFileBlock("OSMData", data);
            write(std::make_shared<keystring>(bl->Quadtree(),packed));
        }
    };
}
    


block_callback make_pack_csvblocks_callback(block_callback cb, std::function<void(std::shared_ptr<geometry::csv_block>)> wr, geometry::pack_csvblocks::tagspec tags,bool with_header) {
    auto pc = geometry::make_pack_csvblocks(tags,with_header);
    return [cb, wr, pc](PrimitiveBlockPtr bl) {
        if (!bl) {
            //std::cout << "pack_csvblocks done" << std::endl;
            wr(std::shared_ptr<geometry::csv_block>());
            if (cb) {
                cb(bl);
            }
            return;
        }
        if (cb) { cb(bl); }
        //std::cout << "call pack_csvblocks ... " << std::endl;
        auto cc = pc->call(bl);
        //std::cout << "points.size()=" << cc->points.size() << ", lines.size()=" << cc->lines.size() << ", polys.size()=" << cc->polys.size() << std::endl;
        wr(cc);
        return;
    };
}
                
                            

std::vector<block_callback> write_to_postgis_callback(
    std::vector<block_callback> callbacks, size_t numchan,
    const std::string& connection_string, const std::string& table_prfx,
    const geometry::pack_csvblocks::tagspec& coltags,
    bool with_header) {
        
    
    auto writers = multi_threaded_callback<geometry::csv_block>::make(geometry::make_postgiswriter_callback(connection_string, table_prfx, with_header), numchan);
    
    std::vector<block_callback> res(numchan);
    
    for (size_t i=0; i < numchan; i++) {
        
        
        auto writer_i = writers[i];
        block_callback cb;
        if (!callbacks.empty()) {
            cb = callbacks[i];
        }
        res[i]=make_pack_csvblocks_callback(cb, writer_i, coltags, with_header);
    }
    
    return res;
}
        
block_callback write_to_postgis_callback_nothread(
    block_callback callback,
    const std::string& connection_string, const std::string& table_prfx,
    const geometry::pack_csvblocks::tagspec& coltags,
    bool with_header) {
        
    
    auto writer = geometry::make_postgiswriter_callback(connection_string, table_prfx,with_header);
    return make_pack_csvblocks_callback(callback,writer,coltags,with_header);
}
    
class geom_progress {
    public:
        geom_progress(const src_locs_map& locs) : nb(0), npt(0), nln(0), nsp(0), ncp(0), maxqt(0) {
            double p=100.0 / locs.size(); size_t i=0;
            for (const auto& l: locs) {
                progs[l.first] = p*i;
                i++;
            }
        }
        
        void call(PrimitiveBlockPtr bl) {
            
            if (bl) {
                nb++;
                if (bl->Quadtree() > maxqt) { maxqt = bl->Quadtree(); }
                for (auto o: bl->Objects()) {
                    if (o->Type() == ElementType::Point) { npt++; }
                    else if (o->Type() == ElementType::Linestring) { nln++; }
                    else if (o->Type() == ElementType::SimplePolygon) { nsp++; }
                    else if (o->Type() == ElementType::ComplicatedPolygon) { ncp++; }
                }
            }
            if (( (nb % 1024) == 0) || !bl) {
                std::cout << "\r" << std::setw(8) << nb
                    << std::fixed << std::setprecision(1) << std::setw(5) << progs[maxqt] << "%"
                    << " " << TmStr{ts.since(),8,1} <<  " "
                    << "[" << std::setw(18) << quadtree::string(maxqt) << "]: "
                    << std::setw(8) << npt << " "
                    << std::setw(8) << nln << " "
                    << std::setw(8) << nsp << " "
                    << std::setw(8) << ncp
                    << std::flush;
            }
        }
    private:
        int64 nb, npt, nln, nsp, ncp, maxqt;
        std::map<int64,double> progs;
        TimeSingle ts;
};

struct geometry_parameters {
    geometry_parameters() 
        : numchan(1), numblocks(512), box(), add_rels(false),
            add_mps(false), recalcqts(false), outfn(""), indexed(false),
            connstring(""), tableprfx("") {}
    
    std::vector<std::string> filenames;
    external_callback callback;
    src_locs_map locs;
    size_t numchan;
    size_t numblocks;
    geometry::style_info_map style;
    bbox box;
    geometry::parenttag_spec_map apt_spec;
    bool add_rels;
    bool add_mps;
    bool recalcqts;
    std::shared_ptr<geometry::findminzoom> findmz;
    std::string outfn;
    bool indexed;
    std::string connstring;
    std::string tableprfx;
    geometry::pack_csvblocks::tagspec coltags;
    std::shared_ptr<QtTree> groups;
    std::function<void(std::shared_ptr<geometry::csv_block>)> csvblock_callback;
};
    
    


std::pair<size_t,geometry::mperrorvec> process_geometry(const geometry_parameters& params) {

    py::gil_scoped_release r;
    
    geometry::mperrorvec errors_res;
    block_callback wrapped;
    if (params.callback) {
        auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(params.callback),params.numblocks);
        wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    } else {
        auto pp = std::make_shared<geom_progress>(params.locs);
        wrapped = [pp](PrimitiveBlockPtr bl) { pp->call(bl); };
    }
    std::vector<block_callback> writer = multi_threaded_callback<PrimitiveBlock>::make(wrapped,params.numchan);
    
        
    if (params.outfn!="") {
        writer = pack_and_write_callback(writer, params.outfn, params.indexed, params.box, params.numchan, true, true, true);
        
    }
    
    if (params.connstring != "") {
        writer = write_to_postgis_callback(writer, params.numchan, params.connstring, params.tableprfx, params.coltags, true);
    }
    
    auto addwns = process_geometry_blocks(
            writer, params.style, params.box, params.apt_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](geometry::mperrorvec& ee) { errors_res.swap(ee); }
    );
    
    read_blocks_merge(params.filenames, addwns, params.locs, params.numchan, nullptr, 7, 1<<14);
      
    
    return std::make_pair(0,errors_res);

}
std::pair<size_t,geometry::mperrorvec> process_geometry_sortblocks(const geometry_parameters& params) {


    py::gil_scoped_release r;
    
    geometry::mperrorvec errors_res;
    
    auto sb = make_sortblocks(20000, params.groups, params.outfn+"-interim",200, params.numchan);
    auto sb_callbacks = sb->make_addblocks_cb(false);
    
    auto addwns = process_geometry_blocks(
            sb_callbacks, params.style, params.box, params.apt_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](geometry::mperrorvec& ee) { errors_res.swap(ee); }
    );
    
    read_blocks_merge(params.filenames, addwns, params.locs, params.numchan, nullptr, 7, 1<<14);
    
    
    sb->finish();
    block_callback wrapped;
    if (params.callback) {
        auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(params.callback),params.numblocks);
        wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    }
    
    std::vector<block_callback> writer;
    if (params.callback) {
        writer = multi_threaded_callback<PrimitiveBlock>::make(wrapped,params.numchan);
    }
    
        
    if (params.outfn!="") {
        writer = pack_and_write_callback(writer, params.outfn, params.indexed, params.box, params.numchan, true, true, true);
        
    }
    sb->read_blocks(writer, true);
    
    
    return std::make_pair(0,errors_res);

}

std::pair<size_t,geometry::mperrorvec> process_geometry_nothread(const geometry_parameters& params) {


    py::gil_scoped_release r;
    
    geometry::mperrorvec errors_res;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(params.callback),params.numblocks);
    block_callback cbc = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };
    block_callback writer=cbc;
    if (params.outfn!="") {
        writer = pack_and_write_callback_nothread(cbc, params.outfn, params.indexed, params.box, true, true, true);
    }
    block_callback postgis=writer;
    if (params.connstring != "") {
        postgis = write_to_postgis_callback_nothread(writer, params.connstring, params.tableprfx, params.coltags, true);
    }
    
    block_callback addwns = process_geometry_blocks_nothread(
            postgis, params.style, params.box, params.apt_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](geometry::mperrorvec& ee) { errors_res.swap(ee); }
    );
    
    read_blocks_merge_nothread(params.filenames, addwns, params.locs, nullptr, 7);
      
    
    return std::make_pair(cb->total(),errors_res);

}

std::pair<size_t,geometry::mperrorvec> process_geometry_csvcallback_nothread(const geometry_parameters& params) {
        
    py::gil_scoped_release r;
    
    geometry::mperrorvec errors_res;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(params.callback),params.numblocks);
    block_callback cbc = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };
    block_callback csvcallback = make_pack_csvblocks_callback(cbc,wrap_callback(params.csvblock_callback),params.coltags, true);
    
    block_callback addwns = process_geometry_blocks_nothread(
            csvcallback, params.style, params.box, params.apt_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](geometry::mperrorvec& ee) { errors_res.swap(ee); }
    );
    
    read_blocks_merge_nothread(params.filenames, addwns, params.locs, nullptr, 7);
      
    
    return std::make_pair(cb->total(),errors_res);

}


std::pair<size_t,geometry::mperrorvec> process_geometry_from_vec(
    std::vector<PrimitiveBlockPtr> blocks,
    external_callback callback,
    size_t numblocks,
    const geometry::style_info_map& style, bbox box,
    const geometry::parenttag_spec_map& apt_spec, bool add_rels, bool add_mps, bool recalcqts,
    std::shared_ptr<geometry::findminzoom> findmz) {


    py::gil_scoped_release r;
    
    geometry::mperrorvec errors_res;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    std::vector<block_callback> cbf;
    cbf.push_back([cb](PrimitiveBlockPtr bl) { cb->call(bl); });
    
    
    
    auto addwns = process_geometry_blocks(
            cbf, style, box, apt_spec, add_rels,
            add_mps, recalcqts, findmz,
            [&errors_res](geometry::mperrorvec& ee) { errors_res.swap(ee); }
    );
    
    for (auto bl : blocks) {
        addwns(bl);
    }
    addwns(PrimitiveBlockPtr());
      
    
    return std::make_pair(cb->total(),errors_res);

}
    
    
PrimitiveBlockPtr read_blocks_geometry_convfunc(std::shared_ptr<FileBlock> fb) {
    if (!fb) { return PrimitiveBlockPtr(); }
    if (fb->blocktype!="OSMData") { return std::make_shared<PrimitiveBlock>(fb->idx,0); }
    return readPrimitiveBlock(fb->idx,fb->get_data(),false,15,nullptr,geometry::readGeometry);
}

std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> make_read_blocks_geometry_convfunc_filter(bbox filt) {
    if (box_planet(filt)) { return read_blocks_geometry_convfunc; }
    if (box_empty(filt)) { return read_blocks_geometry_convfunc; }
    return [filt](std::shared_ptr<FileBlock> fb) {
        auto pb = read_blocks_geometry_convfunc(fb);
        if (!pb) { return pb; }
        
        auto pb2 = std::make_shared<PrimitiveBlock>(pb->Index(), pb->size());
        pb2->SetQuadtree(pb->Quadtree());
        pb2->SetStartDate(pb->StartDate());
        pb2->SetEndDate(pb->EndDate());
        for (auto o: pb->Objects()) {
            auto g = std::dynamic_pointer_cast<BaseGeometry>(o);
            if (overlaps(filt, g->Bounds())) {
                pb2->add(o);
            }
        }
        return pb2;
    };
}       

size_t read_blocks_geometry_py(
    const std::string& filename,
    std::function<bool(std::vector<PrimitiveBlockPtr>)> callback,
    const std::vector<int64>& locs, size_t numchan, size_t numblocks, bbox filt) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    std::function<void(PrimitiveBlockPtr)> cbf = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };
    
    read_blocks_convfunc_primitiveblock(filename, cbf, locs, numchan, make_read_blocks_geometry_convfunc_filter(filt));
    return cb->total();
}
    
                

PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void geometry_defs(py::module& m) {
   


    py::class_<geometry::WayWithNodes, Element, std::shared_ptr<geometry::WayWithNodes>>(m, "WayWithNodes")
        .def_property_readonly("Refs", &geometry::WayWithNodes::Refs)
        .def_property_readonly("LonLats", &geometry::WayWithNodes::LonLats)
        .def_property_readonly("Bounds", &geometry::WayWithNodes::Bounds)
        
    ;
    py::class_<BaseGeometry, Element, std::shared_ptr<BaseGeometry>>(m, "BaseGeometry")
        .def_property_readonly("OriginalType", &BaseGeometry::OriginalType)
        .def_property_readonly("Bounds", &BaseGeometry::Bounds)
        .def_property_readonly("MinZoom", &BaseGeometry::MinZoom)
        .def("SetMinZoom", &BaseGeometry::SetMinZoom)
        .def("Wkb", [](BaseGeometry& p,  bool transform, bool srid) { return py::bytes(p.Wkb(transform,srid)); }, py::arg("transform")=true, py::arg("srid")=true)
    ;
    
    py::class_<GeometryPacked, BaseGeometry, std::shared_ptr<GeometryPacked>>(m,"GeometryPacked")
        .def("unpack_geometry", [](const GeometryPacked& pp) { throw std::domain_error("not implemented"); })
    ;
    
    py::class_<geometry::Point, BaseGeometry, std::shared_ptr<geometry::Point>>(m, "Point")
        .def_property_readonly("LonLat", &geometry::Point::LonLat)
        .def_property_readonly("Layer", &geometry::Point::Layer)
        
        
    ;

    py::class_<geometry::Linestring, BaseGeometry, std::shared_ptr<geometry::Linestring>>(m, "Linestring")
        .def_property_readonly("Refs", &geometry::Linestring::Refs)
        .def_property_readonly("LonLats", &geometry::Linestring::LonLats)
        
        .def_property_readonly("ZOrder", &geometry::Linestring::ZOrder)
        .def_property_readonly("Layer", &geometry::Linestring::Layer)
        .def_property_readonly("Length", &geometry::Linestring::Length)
        
    ;

    py::class_<geometry::SimplePolygon, BaseGeometry, std::shared_ptr<geometry::SimplePolygon>>(m, "SimplePolygon")
        .def_property_readonly("Refs", &geometry::SimplePolygon::Refs)
        .def_property_readonly("LonLats", &geometry::SimplePolygon::LonLats)
        
        .def_property_readonly("Area", &geometry::SimplePolygon::Area)
        .def_property_readonly("ZOrder", &geometry::SimplePolygon::ZOrder)
        .def_property_readonly("Layer", &geometry::SimplePolygon::Layer)
        .def_property_readonly("Reversed", &geometry::SimplePolygon::Reversed)
        
    ;

    py::class_<geometry::ComplicatedPolygon, BaseGeometry, std::shared_ptr<geometry::ComplicatedPolygon>>(m, "ComplicatedPolygon")
    .def_property_readonly("Part", &geometry::ComplicatedPolygon::Part)
        .def_property_readonly("InnerRings", &geometry::ComplicatedPolygon::InnerRings)
        .def_property_readonly("OuterRing", &geometry::ComplicatedPolygon::OuterRing)
       
        .def_property_readonly("Area", &geometry::ComplicatedPolygon::Area)
        .def_property_readonly("ZOrder", &geometry::ComplicatedPolygon::ZOrder)
        .def_property_readonly("Layer", &geometry::ComplicatedPolygon::Layer)
        
    ;

    m.def("make_point", &make_point, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("lon"), py::arg("lat"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_linestring", &make_linestring, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("refs"), py::arg("lonlats"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_simplepolygon", &make_simplepolygon, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("refs"), py::arg("lonlats"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_complicatedpolygon", &make_complicatedpolygon, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("part"), py::arg("exterior"), py::arg("interiors"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));


    py::class_<lonlat>(m,"lonlat")
        .def(py::init<int64,int64>())
        .def_readonly("lon", &lonlat::lon)
        .def_readonly("lat", &lonlat::lat)
        .def_property_readonly("transform", [](const lonlat& ll) { return geometry::forward_transform(ll.lon,ll.lat); })
    ;

    py::class_<geometry::xy>(m,"xy")
        .def(py::init<double,double>())
        .def_readonly("x", &geometry::xy::x)
        .def_readonly("y", &geometry::xy::y)
        .def_property_readonly("transform", [](const geometry::xy& ll) { return geometry::inverse_transform(ll.x,ll.y); })
    ;
    py::class_<geometry::style_info>(m,"style_info")
        .def(py::init<bool,bool,bool,bool,bool>(),py::arg("IsFeature"),py::arg("IsArea"),py::arg("IsWay"),py::arg("IsNode"),py::arg("IsOtherTags"))
        .def_readwrite("IsFeature",&geometry::style_info::IsFeature)
        .def_readwrite("IsNode",&geometry::style_info::IsNode)
        .def_readwrite("IsWay",&geometry::style_info::IsWay)
        .def_readwrite("IsArea",&geometry::style_info::IsArea)
        .def_readwrite("OnlyArea",&geometry::style_info::OnlyArea)
        .def_readwrite("IsOtherTags", &geometry::style_info::IsOtherTags)
        .def_readwrite("ValueList", &geometry::style_info::ValueList)
        .def("addValue", [](geometry::style_info& s, std::string v) { s.ValueList.insert(v); })
        .def("removeValue", [](geometry::style_info& s, std::string v) { s.ValueList.erase(v); })
    ;

    py::class_<geometry::parenttag_spec>(m,"parenttag_spec")
        .def(py::init<std::string,std::string,std::string,std::map<std::string,int>>())
        .def_readwrite("node_tag", &geometry::parenttag_spec::node_tag)
        .def_readwrite("out_tag", &geometry::parenttag_spec::out_tag)
        .def_readwrite("way_tag", &geometry::parenttag_spec::way_tag)
        .def_readwrite("priority", &geometry::parenttag_spec::priority)
    ;

    py::class_<geometry::Ring::Part>(m,"RingPart")
        .def(py::init<int64,const refvector&,const lonlatvec&,bool>())
        .def_readonly("orig_id", &geometry::Ring::Part::orig_id)
        .def_readonly("refs", &geometry::Ring::Part::refs)
        .def_readonly("lonlats", &geometry::Ring::Part::lonlats)
        .def_readonly("reversed", &geometry::Ring::Part::reversed)
    ;
    py::class_<geometry::Ring>(m,"Ring")
        .def(py::init<std::vector<geometry::Ring::Part>>())
        .def_readonly("parts", &geometry::Ring::parts)
    ;
    
    m.def("lonlats_bounds", &geometry::lonlats_bounds);
    m.def("calc_ring_area", [](lonlatvec ll) { return geometry::calc_ring_area(ll); });
    m.def("calc_ring_area", [](geometry::Ring rp) { return geometry::calc_ring_area(rp); });
    m.def("ringpart_refs", &geometry::ringpart_refs);
    m.def("ringpart_lonlats", &geometry::ringpart_lonlats);

    m.def("unpack_geometry_element", &geometry::unpack_geometry_element);
    m.def("unpack_geometry_primitiveblock", &geometry::unpack_geometry_primitiveblock);

    
    py::class_<geometry::csv_block, std::shared_ptr<geometry::csv_block>>(m,"csv_block")
        .def_readonly("points", &geometry::csv_block::points)
        .def_readonly("lines", &geometry::csv_block::lines)
        .def_readonly("polygons", &geometry::csv_block::polygons)
    ;
    py::class_<geometry::csv_rows>(m, "csv_rows")
        .def("__getitem__", &geometry::csv_rows::at)
        .def("__len__", &geometry::csv_rows::size)
        .def("data", [](const geometry::csv_rows& c) { return py::bytes(c.data); })
    ;
    
    py::class_<geometry::findminzoom, std::shared_ptr<geometry::findminzoom>>(m,"findminzoom_base")
        .def("calculate", &geometry::findminzoom::calculate)
    ;
    
    py::class_<findminzoom_onetag, geometry::findminzoom, std::shared_ptr<findminzoom_onetag>>(m,"findminzoom_onetag")
        .def(py::init<findminzoom_onetag::tag_spec, double, double>(), py::arg("spec"), py::arg("minlen"), py::arg("minarea"))
        .def("categories", &findminzoom_onetag::categories)
        .def("areaminzoom", &findminzoom_onetag::areaminzoom)
    ;

    m.def("make_geometries", &geometry::make_geometries, py::arg("style"), py::arg("bbox"), py::arg("block"));
    m.def("filter_node_tags", &geometry::filter_node_tags, py::arg("style"), py::arg("tags"), py::arg("extra_tags_key"));
    m.def("filter_way_tags", &geometry::filter_way_tags, py::arg("style"), py::arg("tags"), py::arg("is_ring"), py::arg("is_bp"), py::arg("extra_tags_key"));


    m.def("pack_tags", &geometry::pack_tags);
    m.def("unpack_tags", [](const std::string& str) {
        std::vector<Tag> tgs;
        geometry::unpack_tags(str, tgs);
        return tgs;
    });
    m.def("convert_packed_tags_to_json", &geometry::convert_packed_tags_to_json);


    py::class_<geometry_parameters>(m, "geometry_parameters")
        .def(py::init<>())
        .def_readwrite("filenames", &geometry_parameters::filenames)
        .def_readwrite("callback", &geometry_parameters::callback)
        .def_readwrite("locs", &geometry_parameters::locs)
        .def_readwrite("numchan", &geometry_parameters::numchan)
        .def_readwrite("numblocks", &geometry_parameters::numblocks)
        .def_readwrite("style", &geometry_parameters::style)
        .def_readwrite("box", &geometry_parameters::box)
        .def_readwrite("apt_spec", &geometry_parameters::apt_spec)
        .def_readwrite("add_rels", &geometry_parameters::add_rels)
        .def_readwrite("add_mps", &geometry_parameters::add_mps)
        .def_readwrite("recalcqts", &geometry_parameters::recalcqts)
        .def_readwrite("findmz", &geometry_parameters::findmz)
        .def_readwrite("outfn", &geometry_parameters::outfn)
        .def_readwrite("indexed", &geometry_parameters::indexed)
        .def_readwrite("connstring", &geometry_parameters::connstring)
        .def_readwrite("tableprfx", &geometry_parameters::tableprfx)
        .def_readwrite("coltags", &geometry_parameters::coltags)
        .def_readwrite("groups", &geometry_parameters::groups)
        .def_readwrite("csvblock_callback", &geometry_parameters::csvblock_callback)
    ;


    m.def("process_geometry", &process_geometry/* ,
       py::arg("filenames"), py::arg("callback"),
        py::arg("locs"), py::arg("numchan"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"),py::arg("outfn"), py::arg("indexed"),
        py::arg("connstring"), py::arg("table_prfx"), py::arg("coltags")*/
    );
    
    m.def("process_geometry_sortblocks", &process_geometry_sortblocks/*, 
        py::arg("filenames"), py::arg("callback"),
        py::arg("locs"), py::arg("numchan"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"),py::arg("outfn"), py::arg("indexed"),
        py::arg("groups")*/
    );
    
    m.def("process_geometry_nothread", &process_geometry_nothread/*,
        py::arg("filenames"), py::arg("callback"),
        py::arg("locs"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"), py::arg("outfn"), py::arg("indexed"),
        py::arg("connstring"), py::arg("table_prfx"), py::arg("coltags")*/
    );
    
    m.def("process_geometry_csvcallback_nothread", &process_geometry_csvcallback_nothread/*,
        py::arg("filenames"), py::arg("callback"),py::arg("csvblock_callback"),
        py::arg("locs"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"), py::arg("coltags")*/
    );
    
    m.def("process_geometry_from_vec", &process_geometry_from_vec,
        py::arg("blocks"), py::arg("callback"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz")
    );
    

    py::class_<find_network, std::shared_ptr<find_network>>(m,"find_network")
        .def(py::init<int64,std::set<std::pair<std::string,std::string>>,bool>())
        .def("add_blocks", &find_network::add_blocks)
        .def("__len__", &find_network::size)
        .def("__contains__", &find_network::contains)
        .def("to_set", &find_network::to_set)
        .def("__getitem__", &find_network::location)
        
    ;
    py::class_<lines_graph, std::shared_ptr<lines_graph>>(m,"lines_graph")
        .def(py::init<std::shared_ptr<find_network>>())
        .def("add_lines", &lines_graph::add_lines)
        .def("add_line", &lines_graph::add_line)
        .def("add_edge", &lines_graph::add_edge)
        .def("forward_edges", &lines_graph::forward_edges)
        .def("backward_edges", &lines_graph::backward_edges)
        .def("num_edges", &lines_graph::num_edges)
        .def("num_nodes", &lines_graph::num_nodes)
        .def("edge_at", &lines_graph::edge_at)
        .def("forward_iter", &lines_graph::forward_iter)
        .def("backward_iter", &lines_graph::backward_iter)
        .def_property_readonly("keys", &lines_graph::get_keysmap)
        .def("connected_components", &lines_graph::connected_components)
    ;
    py::class_<lines_graph::edge>(m,"lines_graph_edge")
        .def_readonly("fr", &lines_graph::edge::from)
        .def_readonly("to", &lines_graph::edge::to)
        .def_readonly("ele", &lines_graph::edge::ele)
        .def_readonly("keyi", &lines_graph::edge::keyi)
        .def_readonly("firstnode", &lines_graph::edge::firstnode)
        .def_readonly("lastnode", &lines_graph::edge::lastnode)
        .def_readonly("length", &lines_graph::edge::length)
        .def_property_readonly("refs", &lines_graph::edge::refs)
        .def_property_readonly("lonlats", &lines_graph::edge::lonlats)
    ;
    
    py::class_<lines_graph::edge_iter>(m,"lines_graph_edge_iter")
        .def("__iter__", [](lines_graph::edge_iter e) { e.reset(); return e; })
        .def("next", &lines_graph::edge_iter::next)
        
    ;
    
    m.def("read_blocks_geometry", &read_blocks_geometry_py,
        py::arg("filename"), py::arg("callback"),
        py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4, py::arg("numblocks")=32, py::arg("bbox")=bbox{1,1,0,0});
    
    m.def("res_zoom", &res_zoom);
    
    
    m.def("point_in_poly", &point_in_poly);
    
    m.def("segment_intersects", &segment_intersects);
    m.def("line_intersects", &line_intersects);
    m.def("line_box_intersects", &line_box_intersects);
    
    py::class_<geometry::PostgisWriter, std::shared_ptr<geometry::PostgisWriter>>(m, "PostgisWriter")
        .def("finish", &geometry::PostgisWriter::finish)
        .def("call", &geometry::PostgisWriter::call)
    ;
    m.def("make_postgiswriter", &geometry::make_postgiswriter);
    
    py::class_<geometry::pack_csvblocks, std::shared_ptr<geometry::pack_csvblocks>>(m, "pack_csvblocks")
        .def("call", &geometry::pack_csvblocks::call)
    ;
    m.def("make_pack_csvblocks", &geometry::make_pack_csvblocks);
    
    
}
