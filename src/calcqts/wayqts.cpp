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

#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/calcqts/waynodes.hpp"
#include "oqt/calcqts/waynodesfile.hpp"

#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/pbf/packedint.hpp"
#include "oqt/utils/invertedcallback.hpp"

#include <algorithm>
#include <chrono>
namespace oqt {
    

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
/*
class CombineWaynodes {
    public:
        typedef std::function<void(std::shared_ptr<wnls>)> callback;
        
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
                
                

                int64 node_ref = curr->node_at(ni);
                
                while ((nd) && (nd->id < node_ref)) {
                    nd = next_node();
                }
                if ((!nd) || (nd->id > node_ref)) {
                    logger_message lm; lm << "\rmissing node " << node_ref;
                    if (nd) { lm << " @ nd " << nd->id << " " << nd->lon << ", " << nd->lat; }
                    nmissing+=1;
                    if (nmissing>500) { throw std::domain_error("??"); }
                } else {
                    int64 way_ref = curr->way_at(ni);
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

*/

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
        /*   
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
                
                int64 a=wns->way_at(0); int64 b=wns->node_at(0);
                size_t li=wns->size()-1;
                int64 c=wns->way_at(li); int64 d=wns->node_at(li);
                
                
                int64 now=getmemval(pid);
                
                logger_progress(50) << "[" << now*1.0/1024.0/1024.0 << "/" << std::setw(12) << (now-was)*1.0/1024.0/1024.0 << "]"
                    << " way_nodes: " << wns->key() << " " << wns->size()
                    << " [" << std::setw(12) << a << ", " << std::setw(12) << b << "=>" << std::setw(12) << c << ", " << std::setw(12) << d << "]";
                
                was=now;
            }
            
            
            int64 w,n;
            
            auto merged = make_wnls(wns->key(), wns->size());
            
            for (size_t pos = 0; pos < wns->size(); pos++) {
                w= wns->way_at(pos);
                n= wns->node_at(pos);
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
*/            
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
            w= block->way_at(0); n=block->node_at(0);
            
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
                w= block->way_at(pos); n=block->node_at(pos);
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

}
