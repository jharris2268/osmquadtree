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

#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/readminimal.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/writeblock.hpp"

#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/calcqts/writewaynodes.hpp"
#include "oqt/calcqts/qtstore.hpp"
#include "oqt/calcqts/qtstoresplit.hpp"
#include "oqt/calcqts/writeqts.hpp"


#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/pbf/packedint.hpp"
#include "oqt/utils/invertedcallback.hpp"

#include <algorithm>
#include <chrono>
namespace oqt { 
    
    
template <class T>
bool cmp_id(const T& l, const T& r) {
    return l.id < r.id;
}





std::vector<size_t> find_index(const std::vector<minimal::Node>& nodes, size_t nsp) {
    
    std::vector<size_t> nodes_ii;
    for (int64 i = 0; i < nodes.back().id; i+=nsp) {
        if (i==0) {
            nodes_ii.push_back(0);
        } else {
            auto it = std::lower_bound(nodes.begin()+nodes_ii.back(), nodes.end(), minimal::Node{i,0,0,0,0}, cmp_id<minimal::Node>);
            if (it<nodes.end()) {
                nodes_ii.push_back(it-nodes.begin());
            }
        }
    }
    nodes_ii.push_back(nodes.size());
    return std::move(nodes_ii);
}

std::vector<minimal::Node>::iterator find_indexed(int64 id, std::vector<minimal::Node>& nodes, const std::vector<size_t>& nodes_idx, size_t nsp) {
    size_t lb = nodes_idx.at(id/nsp);
    size_t ub = nodes_idx.at((id/nsp)+1);
    minimal::Node n; n.id=id;
    return std::lower_bound(nodes.begin()+lb,nodes.begin()+ub,n,cmp_id<minimal::Node>);
}

std::vector<minimal::Node>::const_iterator find_indexed_const(int64 id, const std::vector<minimal::Node>& nodes, const std::vector<size_t>& nodes_idx, size_t nsp) {
    size_t lb = nodes_idx.at(id/nsp);
    size_t ub = nodes_idx.at((id/nsp)+1);
    minimal::Node n; n.id=id;
    return std::lower_bound(nodes.begin()+lb,nodes.begin()+ub,n,cmp_id<minimal::Node>);
}

class ObjQts {
    public:
        virtual int64 find(int64 id) const = 0;
        virtual ~ObjQts() {}
        
};

class ObjQtsMinimalWays : public ObjQts {
    public:
        ObjQtsMinimalWays(const std::vector<minimal::Way>& ways_) : ways(ways_) {}
        
        virtual int64 find(int64 id) const {
            minimal::Way w; w.id=id;
            auto it = std::lower_bound(ways.begin(),ways.end(),w,cmp_id<minimal::Way>);
            
            if ((it!=ways.end()) && (it->id==id)) {
                return it->quadtree;
            }
            return -1;
        }
        
        virtual ~ObjQtsMinimalWays() {}
        
    private:
        const std::vector<minimal::Way>& ways;
};
    
class ObjQtsMinimalNodes : public ObjQts {
    public:
        ObjQtsMinimalNodes(const std::vector<minimal::Node>& nodes_, const std::vector<size_t>& idx_, size_t nsp_) : nodes(nodes_), idx(idx_), nsp(nsp_) {}
        
        virtual int64 find(int64 id) const {
            auto it = find_indexed_const(id, nodes, idx, nsp);
            
            if ((it!=nodes.end()) && (it->id==id)) {
                return it->quadtree;
            }
            return -1;
        }
        
        virtual ~ObjQtsMinimalNodes() {}
        
    private:
        const std::vector<minimal::Node>& nodes;
        const std::vector<size_t>& idx;
        size_t nsp;
};

void calculate_relation_quadtrees(
    std::vector<minimal::Relation>& relations, 
    std::shared_ptr<ObjQts> node_qts, std::shared_ptr<ObjQts> way_qts) {
    
    std::vector<std::pair<size_t,size_t>> relrels;
    
    
    for (size_t i=0; i < relations.size(); i++) {
        auto& r = relations.at(i);
        auto tys = readPackedInt(r.tys_data);
        auto rfs = readPackedDelta(r.refs_data);
        if (tys.size()!=rfs.size()) {
            logger_message() << "??? relation " << r.id << " tys!=refs " << tys.size() << " " << rfs.size();
            throw std::domain_error("??? tys != refs");
        }
        
        int64 q=-1;
        bool arr=false;
        if (!tys.empty()) {
            for (size_t j=0; j < tys.size(); j++) {
                if (tys[j]==0) {
                    q = quadtree::common(q, node_qts->find(rfs[j]));
                    /*
                    auto it = std::lower_bound(nodes.begin(),nodes.end(),minimalnode{rfs[j],0,0,0,0},cmp_id<minimalnode>);
                    if ((it!=nodes.end()) && (it->id==rfs[j])) {
                        q = quadtree::common(q, it->quadtree);
                    }*/
                } else if (tys[j]==1) {
                    q = quadtree::common(q, way_qts->find(rfs[j]));
                    /*auto it = std::lower_bound(ways.begin(),ways.end(),minimalway{rfs[j],0,0,"a"},cmp_id<minimalway>);
                    if ((it!=ways.end()) && (it->id==rfs[j])) {
                        q = quadtree::common(q, it->quadtree);
                    }*/
                } else {
                    minimal::Relation r; r.id=rfs[j];
                    auto it = std::lower_bound(relations.begin(),relations.end(),r,cmp_id<minimal::Relation>);
                    if ((it!=relations.end()) && (it->id==rfs[j])) {
                        relrels.push_back(std::make_pair(i, it-relations.begin()));
                        arr=true;
                    }
                }
            }
        }
        if ((q==-1) && !arr) {
            q=0;
        }
        r.quadtree=q;
        
    }
    logger_message() << "have " << relrels.size() << " relrels";
    for (size_t i=0; i < 5; i++) {
        for (const auto& rr : relrels) {
            int64 a = relations[rr.first].quadtree;
            int64 b = quadtree::common(a,relations[rr.second].quadtree);
            
            if (a!=b) {
                relations[rr.first].quadtree = b;
            }
        }
    }
    size_t nz=0;
    for (auto& r : relations) {
        if (r.quadtree<0) {
            r.quadtree=0;
            nz++;
        }
    }
    logger_message() << "set " << nz << " null quadtrees to zero";
}
    


int run_calcqts_inmem(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool resort) {
    
    auto lg=get_logger();
    std::vector<minimal::Node> nodes;
    std::vector<minimal::Way> ways;
    std::vector<minimal::Relation> relations;
    
    
    auto read_objs = [&nodes,&ways,&relations](minimal::BlockPtr mb) {
        if (!mb) { return; }
        if (!mb->nodes.empty()) {
            
            for (auto n : mb->nodes) {
                n.quadtree=-1;
                nodes.push_back(n);
                
            }
        }
        if (!mb->ways.empty()) {
            for (auto w : mb->ways) {
                w.quadtree=-1;
                ways.push_back(w);
            }
        }
        if (!mb->relations.empty()) {
            for (auto r : mb->relations) {
                r.quadtree=-1;
                relations.push_back(r);
            }
        }
    };
    
    read_blocks_minimalblock(origfn, read_objs, {},  numchan, 7);
    lg->time("read data");    
    
    
    std::sort(nodes.begin(),nodes.end(),cmp_id<minimal::Node>);
    std::sort(ways.begin(),ways.end(),cmp_id<minimal::Way>);
    std::sort(relations.begin(),relations.end(),cmp_id<minimal::Relation>);
    
    //indexed_nodes nodes_idx(nodes, 256);
    std::vector<size_t> nodes_idx = find_index(nodes,256);
    
    
    lg->time("sort data");
    size_t nmissing=0;
    for (auto& w : ways) {
        auto wns = readPackedDelta(w.refs_data);
        std::vector<size_t> ni;
        ni.reserve(wns.size());
        bbox bx;
        for (const auto& n: wns) {
            
            
            //auto it = nodes_idx.find(n);
            auto it = find_indexed(n,nodes,nodes_idx,256);
            
            if ((it==nodes.end()) || (it->id!=n)) {
                logger_message() << "missing node " << n << " [way " << w.id << "]";
                nmissing+=1;
                ni.push_back(nodes.size());
            } else {
            
                bx.expand_point(it->lon,it->lat);
                ni.push_back(it-nodes.begin());
            }
        }
        int64 q = quadtree::calculate(bx.minx,bx.miny,bx.maxx,bx.maxy,0.05,18);
        w.quadtree = q;
        
        
        for (const auto& ii : ni) {
            if (ii<nodes.size()) {
                auto& n = nodes[ii];
                n.quadtree = quadtree::common(n.quadtree,q);
            }
        }
        
    }
    logger_message() << "have " << nmissing << " missing waynodes";
    lg->time("calculate way qts");
    
    
    
    
    auto out = make_collectqts(qtsfn, numchan, 8000);
    //CollectQts out(packers, 8000);
    
    for (auto& n: nodes) {
        if (n.quadtree==-1) {
            n.quadtree = quadtree::calculate(n.lon,n.lat, n.lon,n.lat, 0.05,18);
        }
        out->add(0,n.id,n.quadtree);
    }
    
    lg->time("calculate node qts");
    
    for (auto& w : ways) {
        out->add(1,w.id,w.quadtree);
    }
    
    lg->time("wrote way qts");
    
    calculate_relation_quadtrees(relations, std::make_shared<ObjQtsMinimalNodes>(nodes,nodes_idx,256), std::make_shared<ObjQtsMinimalWays>(ways));
    
    lg->time("calculate relation qts");
    for (auto& r : relations) {
        
        out->add(2,r.id,r.quadtree);
    }
    
    out->finish();
    
    lg->time("wrote relation qts");
    
    return 1;
}
}
