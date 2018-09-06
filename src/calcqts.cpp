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
#include "oqt/simplepbf.hpp"
#include "oqt/pbfformat/readminimal.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/waynodes.hpp"
#include <algorithm>
#include <chrono>
namespace oqt {
struct qtsblock {
    size_t idx;
    int64 ty;
    std::vector<int64> refs;
    std::vector<int64> qts;
};

std::string pack_qtsblock_data(std::shared_ptr<qtsblock> bl) {
    std::list<PbfTag> mm;
    if (bl->ty==0) {
        std::string ids = writePackedDelta(bl->refs);
        std::string qts = writePackedDelta(bl->qts);
        std::string lns(bl->refs.size(),0);
        
        std::string result(ids.size()+qts.size()+2*lns.size()+40,0);
        size_t pos=0;

        pos = writePbfData(result,pos,1,ids);
        pos = writePbfData(result,pos,8,lns);
        pos = writePbfData(result,pos,9,lns);
        pos = writePbfData(result,pos,20,qts);
        result.resize(pos);

        mm.push_back(PbfTag{2,0,result});
    } else {
        for (size_t i=0; i < bl->refs.size(); i++) {
            std::string res(20,0);
            size_t pos = writePbfValue(res,0,1,uint64(bl->refs.at(i)));
            pos = writePbfValue(res,pos,20,zigZag(bl->qts.at(i)));
            res.resize(pos);
            size_t x = 2 + bl->ty;
            mm.push_back(PbfTag{x, 0, res});
        }       
    }
    
    return packPbfTags({PbfTag{2,0,packPbfTags(mm)}});    
    
}


keystring_ptr pack_qtsblock(std::shared_ptr<qtsblock> qq) {
    
    if (!qq) { return nullptr; }
    
    std::string bl = pack_qtsblock_data(qq);
    
    auto fb = prepareFileBlock("OSMData", bl, -1);
    return std::make_shared<keystring>(qq->idx,fb);
}   

class CollectQts {
    public:
        typedef std::shared_ptr<qtsblock> qtsblock_ptr;
        typedef std::function<void(qtsblock_ptr)> callback;
        
        CollectQts(std::vector<callback> callbacks_, size_t blocksize_) :
            callbacks(callbacks_), blocksize(blocksize_), idx(0) {
            
            resetcurr(0);
            
        }
        
        void add(int64 t, int64 i, int64 qt) {
            if (t!=curr->ty) {
                sendcurr();
                resetcurr(t);
            }
            curr->refs.push_back(i);
            curr->qts.push_back(qt);
            if (curr->refs.size()==blocksize) {
                sendcurr();
                resetcurr(t);
            }
        }
        void finish() {
            if (!curr->refs.empty()) {
                sendcurr();
            }
            for (auto cb: callbacks) {
                cb(nullptr);
            }
        }
        
        
        
    private:
        void resetcurr(int64 t) {
            curr=std::make_shared<qtsblock>();
            curr->idx=idx;
            curr->ty = t;
            curr->refs.reserve(blocksize);
            curr->qts.reserve(blocksize);
        }
        void sendcurr() {
            callbacks[idx%callbacks.size()](curr);
            idx++;
            
        }
        
        std::vector<callback> callbacks;
        size_t blocksize;
        size_t idx;
        qtsblock_ptr curr;
};
template <class T>
bool cmp_id(const T& l, const T& r) {
    return l.id < r.id;
}





std::vector<size_t> find_index(const std::vector<minimalnode>& nodes, size_t nsp) {
    
    std::vector<size_t> nodes_ii;
    for (int64 i = 0; i < nodes.back().id; i+=nsp) {
        if (i==0) {
            nodes_ii.push_back(0);
        } else {
            auto it = std::lower_bound(nodes.begin()+nodes_ii.back(), nodes.end(), minimalnode{i,0,0,0,0}, cmp_id<minimalnode>);
            if (it<nodes.end()) {
                nodes_ii.push_back(it-nodes.begin());
            }
        }
    }
    nodes_ii.push_back(nodes.size());
    return std::move(nodes_ii);
}

std::vector<minimalnode>::iterator find_indexed(int64 id, std::vector<minimalnode>& nodes, const std::vector<size_t>& nodes_idx, size_t nsp) {
    size_t lb = nodes_idx.at(id/nsp);
    size_t ub = nodes_idx.at((id/nsp)+1);
    minimalnode n; n.id=id;
    return std::lower_bound(nodes.begin()+lb,nodes.begin()+ub,n,cmp_id<minimalnode>);
}

std::vector<minimalnode>::const_iterator find_indexed_const(int64 id, const std::vector<minimalnode>& nodes, const std::vector<size_t>& nodes_idx, size_t nsp) {
    size_t lb = nodes_idx.at(id/nsp);
    size_t ub = nodes_idx.at((id/nsp)+1);
    minimalnode n; n.id=id;
    return std::lower_bound(nodes.begin()+lb,nodes.begin()+ub,n,cmp_id<minimalnode>);
}
            
class indexed_nodes {
    public:
        indexed_nodes(std::vector<minimalnode>& nodes_, size_t nsp_) :
            nodes(nodes_), nsp(nsp_) {
            
            idx = find_index(nodes,nsp);
        }
        
        std::vector<minimalnode>::iterator find(int64 id) {
            return std::move(find_indexed(id,nodes,idx,nsp));
        }
    private:
        std::vector<minimalnode>& nodes;
        size_t nsp;
        std::vector<size_t> idx;
};            


class ObjQts {
    public:
        virtual int64 find(int64 id) const = 0;
        virtual ~ObjQts() {}
        
};

class ObjQtsMinimalWays : public ObjQts {
    public:
        ObjQtsMinimalWays(const std::vector<minimalway>& ways_) : ways(ways_) {}
        
        virtual int64 find(int64 id) const {
            minimalway w; w.id=id;
            auto it = std::lower_bound(ways.begin(),ways.end(),w,cmp_id<minimalway>);
            
            if ((it!=ways.end()) && (it->id==id)) {
                return it->quadtree;
            }
            return -1;
        }
        
        virtual ~ObjQtsMinimalWays() {}
        
    private:
        const std::vector<minimalway>& ways;
};
    
class ObjQtsMinimalNodes : public ObjQts {
    public:
        ObjQtsMinimalNodes(const std::vector<minimalnode>& nodes_, const std::vector<size_t>& idx_, size_t nsp_) : nodes(nodes_), idx(idx_), nsp(nsp_) {}
        
        virtual int64 find(int64 id) const {
            auto it = find_indexed_const(id, nodes, idx, nsp);
            
            if ((it!=nodes.end()) && (it->id==id)) {
                return it->quadtree;
            }
            return -1;
        }
        
        virtual ~ObjQtsMinimalNodes() {}
        
    private:
        const std::vector<minimalnode>& nodes;
        const std::vector<size_t>& idx;
        size_t nsp;
};

void calculate_relation_quadtrees(
    std::vector<minimalrelation>& relations, 
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
                    minimalrelation r; r.id=rfs[j];
                    auto it = std::lower_bound(relations.begin(),relations.end(),r,cmp_id<minimalrelation>);
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
    std::vector<minimalnode> nodes;
    std::vector<minimalway> ways;
    std::vector<minimalrelation> relations;
    
    
    auto read_objs = [&nodes,&ways,&relations](std::shared_ptr<minimalblock> mb) {
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
    
    
    std::sort(nodes.begin(),nodes.end(),cmp_id<minimalnode>);
    std::sort(ways.begin(),ways.end(),cmp_id<minimalway>);
    std::sort(relations.begin(),relations.end(),cmp_id<minimalrelation>);
    
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
    
    auto write_file_obj = make_pbffilewriter(qtsfn, nullptr, false);
    
    auto writeblocks = multi_threaded_callback<keystring>::make(
        [write_file_obj](std::shared_ptr<keystring> ks) {
            if (ks) {
                write_file_obj->writeBlock(ks->first,ks->second);
            }
        }, numchan);
    
    std::vector<std::function<void(std::shared_ptr<qtsblock>)>> packers;
    for (size_t i=0; i < numchan; i++) {
        auto wb = writeblocks[i];
        packers.push_back(threaded_callback<qtsblock>::make(
            [wb](std::shared_ptr<qtsblock> qv) {
                if (!qv) {
                    wb(nullptr);
                } else {
                    wb(pack_qtsblock(qv));
                }
            }
        ));
    }
    
    CollectQts out(packers, 8000);
    
    for (auto& n: nodes) {
        if (n.quadtree==-1) {
            n.quadtree = quadtree::calculate(n.lon,n.lat, n.lon,n.lat, 0.05,18);
        }
        out.add(0,n.id,n.quadtree);
    }
    
    lg->time("calculate node qts");
    
    for (auto& w : ways) {
        out.add(1,w.id,w.quadtree);
    }
    
    lg->time("wrote way qts");
    
    calculate_relation_quadtrees(relations, std::make_shared<ObjQtsMinimalNodes>(nodes,nodes_idx,256), std::make_shared<ObjQtsMinimalWays>(ways));
    
    lg->time("calculate relation qts");
    for (auto& r : relations) {
        
        out.add(2,r.id,r.quadtree);
    }
    
    out.finish();
    
    lg->time("wrote relation qts");
    
    write_file_obj->finish();
    lg->time("finished file");
    return 1;
}

struct wnl {
    int64 ref;
    int32 lon;
    int32 lat;
};

struct wnls {
    int64 key;
    std::vector<wnl> vals;
};
std::shared_ptr<wnls> make_wnls(int64 key, size_t sz) {
    auto r = std::make_shared<wnls>();
    r->key=key;
    r->vals.reserve(sz);
    return r;
};

class CombineWaynodes {
    public:
        typedef std::function<void(std::shared_ptr<wnls/*way_node_locs*/>)> callback;
        
        CombineWaynodes(std::function<std::shared_ptr<minimalblock>()> nodes_, callback cb_) //std::vector<callback> cbs_)
            :nodes(nodes_), cb(cb_) { //cbs(cbs_) {
            
            
            curr_nb = nodes();
            curr_nbi = 0;
            nmissing=0;
            
            
        }
        
        void call(std::shared_ptr<way_nodes> curr) {
            if (!curr) {
                size_t nn=0;
                auto nd = next_node();
                while (nd) {
                    nn++;
                    nd = next_node();
                }
                logger_message() << "\rhad " << nmissing << " missing nodes, " << nn << " nodes at end";
                cb(nullptr);
                return;
            }
            
            //auto wnls = make_way_node_locs(curr->key(), curr->size());
            auto out = make_wnls(curr->key(),curr->size());
            
            auto nd = next_node();
            for (size_t ni = 0; ni < curr->size(); ni++) {
                
                const auto& wnd = curr->at(ni);
                int64 way_ref = wnd.first;//way_ref;
                int64 node_ref = wnd.second;//node_ref;
                
                while ((nd) && (nd->id < node_ref)) {
                    nd = next_node();
                }
                if ((!nd) || (nd->id > node_ref)) {
                    logger_message lm; lm << "\rmissing node " << node_ref;
                    if (nd) { lm << " @ nd " << nd->id << " " << nd->lon << ", " << nd->lat; }
                    nmissing+=1;
                    if (nmissing>500) { throw std::domain_error("??"); }
                } else {
                    //curr->set_loc(ni, nd->lon, nd->lat);
                    //wnls->add(way_ref,node_ref,nd->lon, nd->lat);
                    out->vals.push_back(wnl{way_ref,nd->lon,nd->lat});
                }
            }
            //cb(wnls);
            cb(out);
            
        }
        
    private:
        const minimalnode* next_node() {
            if (!curr_nb) { return nullptr; }
            if (curr_nbi == curr_nb->nodes.size()) {
                curr_nb = nodes();
                curr_nbi=0;
                return next_node();
            }
            return &(curr_nb->nodes[curr_nbi++]);
        }
        std::function<std::shared_ptr<minimalblock>()> nodes;
        //std::vector<callback> cbs;
        callback cb;
        
        
        std::shared_ptr<minimalblock> curr_nb;
        size_t curr_nbi;
        size_t nmissing=0;
};


class WaynodesQts {
    public:
        typedef std::function<void(std::shared_ptr<qtstore>)> callback;
        
        WaynodesQts(callback cb_, std::shared_ptr<qtstore> qts_)
            : qts(qts_), cb(cb_), nmissing(0) {}
        
        void call(std::shared_ptr<way_nodes> curr) {
            if (!curr) {
                logger_message() << "had " << nmissing << " missing ways";
                cb(nullptr);
                return;
            }
            if (curr->size()==0) {
                return;
            }
            
            
            int64 mn = curr->key() * (1<<20);
            int64 mx = mn + (1<<20);
            
            
            auto out = make_qtstore_arr(mn, mx, curr->key());
            
            for (size_t i=0; i < curr->size(); i++) {
                
                const auto& n = curr->at(i);
                auto q = qts->at(n.first/*way_ref*/);
                if (q==-1) {
                    logger_message() << "missing way " << n.first;
                    nmissing+=1;
                } else {
                    out->expand(n.second/*node_ref*/, q);
                }
            }
            
            cb(out);               
            
        }
        
    private:
        
        std::shared_ptr<qtstore> qts;
        callback cb;
        size_t nmissing;
};


struct wb {
    int32 minx;
    int32 miny;
    int32 maxx;
    int32 maxy;
};

template <size_t SIZE>
class wbtile {
    public:
        wbtile(int64 key_) :
                key(key_), size(1ull<<SIZE), boxes(size, wb{2000000000,1000000000,-2000000000,-1000000000})  {
                //key(key_), size(1ull<<SIZE) {
            min = key*size;
            max = (key+1)*size;
            //boxes.fill(wb{2000000000,1000000000,-2000000000,-1000000000});
        }
        
                
        void expand(int64 id, int32 x, int32 y) {
            if ((id<min) || (id>= max)) { throw std::domain_error("wtf"); }
            
            size_t i = id-min;
            wb& box = boxes[i];
            if (x < box.minx) { box.minx=x; }
            if (y < box.miny) { box.miny=y; }
            if (x > box.maxx) { box.maxx=x; }
            if (y > box.maxy) { box.maxy=y; }
            
        }
        void reset() {
            boxes.clear();
            std::vector<wb> empty;
            boxes.swap(empty);
        }
        
        std::shared_ptr<qtstore> calculate(double buffer, int64 maxdepth) {
            
            std::vector<int64> qts(size,-1);
            size_t count=0;
            for (size_t i=0; i < size; i++) {
                const wb& box = boxes[i];
                if (box.minx < 2000000000) {
                    count++;
                    qts[i] = quadtree::calculate(box.minx, box.miny, box.maxx, box.maxy, buffer, maxdepth);
                }
            }
            reset();
            return make_qtstore_arr_alt(std::move(qts), min, count, key);
        }
        
    
        int64 key;
    private:
        size_t size;
        int64 min;
        int64 max;
        std::vector<wb> boxes;
        //std::array<wb,1<<SIZE> boxes;
                
};

std::string getmem(size_t);
int64 getmemval(size_t);        
           
class next_node {
    public:
        next_node(std::function<std::shared_ptr<minimalblock>()> read_nodes_) :
            read_nodes(read_nodes_), pos(0) {
                
            
            block =read_nodes();
        }
        
        const minimalnode* next() {
            while (block && (pos == block->nodes.size())) {
                block=read_nodes();
                pos=0;
            }
            if (!block) {
                return nullptr;
            }
            const minimalnode& n = block->nodes[pos];
            pos++;
            return &n;
        };
    private:
        
        std::shared_ptr<minimalblock> block;
        std::function<std::shared_ptr<minimalblock>()> read_nodes;
        size_t pos;
};


class WayNodeLocs {
    public:
        WayNodeLocs(const std::string& source_filename,const std::vector<int64>& source_locs, size_t numchan, std::function<void(std::shared_ptr<wnls>)> expand_all_) :
            expand_all(expand_all_), missing(0) {
                
                
            read_node = std::make_shared<next_node>(inverted_callback<minimalblock>::make([source_filename,numchan,&source_locs](std::function<void(std::shared_ptr<minimalblock>)> cb) {
                read_blocks_minimalblock(source_filename, cb, source_locs, numchan, 49);
            }));
                
            curr=read_node->next();
            pid = getpid();
            
            
            atend=0;
                
        }
        
        
        static std::function<void(std::shared_ptr<way_nodes>)> make(const std::string& source_filename,const std::vector<int64>& source_locs, size_t numchan, std::function<void(std::shared_ptr<wnls>)> expand_all) {
            auto ww = std::make_shared<WayNodeLocs>(source_filename,source_locs,numchan,expand_all);
            return [ww](std::shared_ptr<way_nodes> wns) { ww->call(wns); };
        }
        
        void call(std::shared_ptr<way_nodes> wns) {
            if (!wns) {
                while (curr) {
                    atend++;
                    curr=read_node->next();
                }
                
                expand_all(nullptr);
                int64 now=getmemval(pid);
                
                
                
                logger_progress(100) << "["  << now*1.0/1024.0/1024.0 << "/" << (now-was)*1.0/1024.0/1024.0 << "]"
                    << " " << atend << " nodes at end, " << missing << " missing";
                
                was=now;
                return;
            }
            if (wns->size()==0) { return; }
            
            if ((wns->key() % 100)==0) {
                int64 a,b,c,d;
                std::tie(a,b) = wns->at(0);
                std::tie(c,d) = wns->at(wns->size()-1);
                int64 now=getmemval(pid);
                
                logger_progress(50) << "[" << now*1.0/1024.0/1024.0 << "/" << std::setw(12) << (now-was)*1.0/1024.0/1024.0 << "]"
                    << " way_nodes: " << wns->key() << " " << wns->size()
                    << " [" << std::setw(12) << a << ", " << std::setw(12) << b << "=>" << std::setw(12) << c << ", " << std::setw(12) << d << "]";
                
                was=now;
            }
            
            
            int64 w,n;
            
            auto merged = make_wnls(wns->key(), wns->size());
            
            for (size_t pos = 0; pos < wns->size(); pos++) {
                std::tie(w,n) = wns->at(pos);
                while ((curr) && curr->id < n) {
                    curr=read_node->next();
                }
                
                if ((!curr) || (curr->id > n)) {
                    missing++;
                } else {
                    //expand(w, curr->lon, curr->lat);
                    merged->vals.push_back(wnl{w,curr->lon, curr->lat});
                }
            }
            
            expand_all(merged);
            
        }
    private:
        std::shared_ptr<next_node> read_node;
        std::function<void(std::shared_ptr<wnls>)> expand_all;
        size_t missing;
        const minimalnode* curr;
        size_t pid;
        int64 was;
        
        size_t atend;
};
            
class WayNodeLocsAlt {
    public:
        WayNodeLocsAlt(std::shared_ptr<WayNodesFile> wnf, std::function<void(std::shared_ptr<wnls>)> expand_all_, int64 minway, int64 maxway) :
            expand_all(expand_all_), missing(0), atend(0) {
        
            next_block = inverted_callback<way_nodes>::make([wnf,minway,maxway](std::function<void(std::shared_ptr<way_nodes>)> ww) {
                wnf->read_waynodes(ww, minway, maxway);
            });
            pid = getpid();
            was=getmemval(pid);
            
            block = next_block();
            while (block && (block->size()==0)) {
                block = next_block();
            }
            if (!block) {
                throw std::domain_error("no way nodes???");
            }
            pos=0;
            std::tie(w,n) = block->at(0);
            
            curr = make_wnls(block->key(), block->size());
            
            
        }
        
        
        void call(std::shared_ptr<minimalblock> mb) {
            
            if (!mb) {
                while (n>0) {
                    next_wn();
                    missing++;
                }
                logger_message() << "finished, have " << missing << " missing, " << atend << " atend";
                expand_all(nullptr);
                return;
            }
            if (mb->nodes.empty()) { return; }
            
            if ((mb->index % 100) == 0) {
                int64 now=getmemval(pid);
                
                logger_progress lp(mb->file_progress);
                lp << "[" << now*1.0/1024.0/1024.0 << "/" << std::setw(12) << (now-was)*1.0/1024.0/1024.0 << "]"
                          << "node block " << mb->index << " " << mb->nodes.front().id << " => " << mb->nodes.back().id << ", block ";
                if (block) {
                    lp << block->key() << "@" << pos;
                } else {
                    lp << "FINISHED";
                }
                lp << " {" << w << ", " << n << " }";
                
                was=now;
            }
            
            for (const auto& nd: mb->nodes) {
                while ((n>0) && (nd.id > n)) {
                    missing++;
                    next_wn();
                }
                if (n<0) {
                    atend++;
                } else if (nd.id < n) {
                    //pass
                } else {
                    
                    while (nd.id == n) {
                    
                        curr->vals.push_back(std::move(wnl{w, nd.lon, nd.lat}));
                        next_wn();
                    }
                }
                
            }
        }
    private:
        std::function<void(std::shared_ptr<wnls>)> expand_all;
        size_t missing;
        size_t atend;
        std::function<std::shared_ptr<way_nodes>()> next_block;
        std::shared_ptr<way_nodes> block;
        size_t pos;
        std::shared_ptr<wnls> curr;
        
        int64 w,n;
        
        void next_wn() {
            pos++;
            while (block && (pos >= block->size())) {
                if (! curr->vals.empty()) { expand_all(curr); }
                block = next_block();
                if (block) {
                    curr = make_wnls(block->key(), block->size());
                }
                pos=0;
            }
            
            if (block) {
                std::tie(w,n) = block->at(pos);
            } else {
                w=-1; n=-1;
            }
        }
        
        int64 pid;
        int64 was;
        
        
};
        

class ExpandBoxes {
    public:
        ExpandBoxes(size_t split_, size_t nt) : split(split_), tiles(nt) {
            pid = getpid();
            cc=0;
            mn=1ll<<61; mx=0;
        }
        
        virtual ~ExpandBoxes() { logger_message() << "~ExpandBoxes"; }
        
        void expand(int64 w, int32 x, int32 y) {
            if (w<0) { throw std::domain_error("wtf"); }
            int64 k = (w/split);
            
            if (((size_t) k)>=tiles.size()) {
                logger_message() << "resize ExpandBoxes to " << k+1 << " {" << 16*(k+1) << "mb}, RSS=" << getmemval(getpid())/1024.0/1024;
                tiles.resize(k+1);
            }
            if (!tiles[k]) {
                
                
                cc++;
                try {
                    tiles[k] = std::make_unique<wbtile<20>>(k);
                    //logger_message() << "ExpandBoxes added tile " << k << " [have " << cc << "], RSS=" << getmemval(getpid())/1024.0/1024;
                } catch(std::exception& ex) {
                    logger_message() << "ExpandBoxes addin tile " << k << " failed, RSS=" << getmemval(getpid())/1024.0/1024;
                    throw ex;
                }
                
            }
            
            tiles[k]->expand(w,x,y);
        }
        
        void expand_all(std::shared_ptr<wnls> ww) {
            if (!ww) { return; }
            bool mm=false;
            for (const auto& w: ww->vals) {
                expand(w.ref, w.lon, w.lat);
                
                if (w.ref<mn) {
                    mm=true; mn=w.ref;
                }
                if (w.ref>mx) {
                    mm=true; mx=w.ref;
                }
            }
            if (mm) {
                //logger_message() << "range " << mn << " [" << (mn/split) << "] to " << mx << " [" << (mx/split) << "]";
            }
        }
        
        
        
        void calculate(std::shared_ptr<qtstore_split> qts, double buffer, size_t maxdepth) {
            int64 now=getmemval(pid);
            logger_message() << "[" << std::setw(8)  << cc << " tiles, " << std::setw(12) << now*1.0/1024.0/1024.0  << "]";
            int64 was=now;
           
            
            for (size_t i=0; i < tiles.size(); i++) {
                if (tiles[i]) {
                    logger_progress(100.0*i/tiles.size()) <<  "calc tile " << i;
                    
                    qts->add_tile(i,tiles[i]->calculate(buffer, maxdepth));
                    tiles[i].reset();
                    
                }
            }
            
            
            tiles.clear();
            
            now=getmemval(pid);
            logger_message() << "[" << cc << " tiles, " << now*1.0/1024.0/1024.0 << "/" << (now-was)*1.0/1024.0/1024.0 << "]";
            
            
        }
    private:
       
        size_t split;
        size_t pid;
        
        std::vector<std::unique_ptr<wbtile<20>>> tiles;
        size_t cc;
        
        int64 mn, mx;
};          
        
        

void find_way_quadtrees(
    const std::string& source_filename,
    const std::vector<int64>& source_locs, 
    size_t numchan,
    std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns,
    double buffer, size_t max_depth,int64 minway, int64 maxway) {
    
    
    
    size_t nb=580;
    
    logger_message() << "find way qts " << minway << " to " << maxway << ", RSS=" << getmemval(getpid())/1024.0/1024;
    auto expand=std::make_shared<ExpandBoxes>(1<<20, nb);
    auto expand_all = [expand](std::shared_ptr<wnls> ww) { expand->expand_all(ww); };
    
    
    //auto wnlocs = WayNodeLocs::make(source_filename, source_locs, numchan, expand_all); 
    
    //wns->read_waynodes(wnlocs, minway, maxway);
    
    
    auto wnla = std::make_shared<WayNodeLocsAlt>(wns, expand_all, minway, maxway);
    read_blocks_minimalblock(source_filename, [wnla](std::shared_ptr<minimalblock> mb) { wnla->call(mb); }, source_locs, numchan, 1 | 48);
    
    
    get_logger()->time("expand way bboxes");
    
    
    expand->calculate(way_qts, buffer, max_depth);
    get_logger()->time("calculate way qts");
}
    



void write_qts_file(const std::string& qtsfn, const std::string& nodes_fn, size_t numchan,
    const std::vector<int64>& node_locs, std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns, std::shared_ptr<calculate_relations> rels, double buffer, size_t max_depth) {
    
    
    auto node_qts = inverted_callback<qtstore>::make([wns,way_qts](std::function<void(std::shared_ptr<qtstore>)> cb) {
        auto nn = std::make_shared<WaynodesQts>(cb, way_qts);
        auto nncb = threaded_callback<way_nodes>::make([nn](std::shared_ptr<way_nodes> x) { nn->call(x); });
        wns->read_waynodes(nncb, 0, 0);
    });
    
    auto write_file_obj = make_pbffilewriter(qtsfn, nullptr, false);
    
    auto writeblocks = multi_threaded_callback<keystring>::make(
        [write_file_obj](std::shared_ptr<keystring> ks) {
            if (ks) {
                write_file_obj->writeBlock(ks->first,ks->second);
            }
        }, numchan);
    
    std::vector<std::function<void(std::shared_ptr<qtsblock>)>> packers;
    for (size_t i=0; i < numchan; i++) {
        auto wb = writeblocks[i];
        packers.push_back(threaded_callback<qtsblock>::make(
            [wb](std::shared_ptr<qtsblock> qv) {
                if (!qv) {
                    wb(nullptr);
                } else {
                    wb(pack_qtsblock(qv));
                }
            }
        ));
    }
    
    
    CollectQts out(packers, 8000);
    
    
    auto nq = node_qts();
    int64 nql = nq->ref_range().second;
    
    auto calc_node_qts = threaded_callback<minimalblock>::make([rels,&out,buffer,max_depth](std::shared_ptr<minimalblock> nds) {
        if (!nds) { return; }
        for (auto& n : nds->nodes) {
            if (n.quadtree==-1) {
                n.quadtree = quadtree::calculate(n.lon,n.lat,n.lon,n.lat,buffer, max_depth);
            }
            out.add(0,n.id,n.quadtree);
        }
        rels->add_nodes(nds);
    });
            
            
    
    auto add_nodeway_qts = [&nq,&nql,node_qts,&calc_node_qts](std::shared_ptr<minimalblock> nds) {
        if (!nds) {

            calc_node_qts(nullptr);
            return;
        }
        logger_progress(nds->file_progress) << "calculate node quadtrees";
        for (auto& n : nds->nodes) {

            while (nq && (n.id >= nql)) {
                nq=node_qts();
                if (nq) {
                    nql = nq->ref_range().second;
                } else {
                    nql = -1;
                }
            }
            int64 q=-1;
            if (nq) { q = nq->at(n.id); }
            n.quadtree=q;
        }
        calc_node_qts(nds);
    };
    read_blocks_minimalblock(nodes_fn, add_nodeway_qts, node_locs, 4, 1 | 48);
    
    
    auto lg=get_logger();
    
    lg->time("calculate node qts");            
    rels->add_ways(way_qts);
    lg->time("added rel way qts");
    for (size_t ti = 0; ti < way_qts->num_tiles(); ti++) {
        auto wq = way_qts->tile(ti);
        if (!wq) { continue; }
        
        int64 f = wq->first();
        logger_progress(100.0*ti/way_qts->num_tiles()) << "write ways tile " << wq->key() << " first " << f;
        
        while (f>0 ) {
            out.add(1, f, wq->at(f));
            f= wq->next(f);
        }
    }
    lg->time("wrote way qts");
    
    rels->finish_alt([&out](int64 i, int64 q) { out.add(2,i,q); });
    
    
    out.finish();
    
    lg->time("wrote relation qts");
    
    write_file_obj->finish();
    lg->time("finished file");
    
}



int run_calcqts(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool splitways, bool resort, double buffer, size_t max_depth) {
    
    auto lg=get_logger();
    std::string waynodes_fn = qtsfn+"-waynodes";
    
    std::shared_ptr<calculate_relations> rels;
    std::shared_ptr<WayNodesFile> wns;
     
    std::string nodes_fn;
    std::vector<int64> node_locs;
    std::tie(wns,rels,nodes_fn,node_locs) = write_waynodes(origfn, waynodes_fn, 1/*numchan*/, resort, lg);    
    
    
    
    
    logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
   // logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
    //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
    
    std::shared_ptr<qtstore_split> way_qts = make_qtstore_split(1<<20,true);
    
    
    
    if (splitways) {
        int64 midway = 320<<20;
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, midway); //find_way_quadtrees_test
        
        logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        //logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
        //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, midway, 0);
        
        logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        //logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
        //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        
    } else {
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, 0);
        logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        //logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
        //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
    }
    
    
    write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth);
    return 1;
}



}
    
    
        
    
    
            
