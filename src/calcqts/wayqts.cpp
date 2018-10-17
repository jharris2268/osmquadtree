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
#include "oqt/utils/logger.hpp"

#include <algorithm>
#include <chrono>
namespace oqt {
    

struct WayNodeLocation {
    int64 ref;
    int32 lon;
    int32 lat;
};

struct WayNodeLocationBlock {
    int64 key;
    std::vector<WayNodeLocation> vals;
};
std::shared_ptr<WayNodeLocationBlock> make_waynodelocationblock(int64 key, size_t sz) {
    auto r = std::make_shared<WayNodeLocationBlock>();
    r->key=key;
    r->vals.reserve(sz);
    return r;
};


struct WayBBox {
    int32 minx;
    int32 miny;
    int32 maxx;
    int32 maxy;
};

class WayBBoxTile {
    public:
        WayBBoxTile(int64 key_, size_t size_) :
                key(key_), size(size_), boxes(size_, WayBBox{2000000000,1000000000,-2000000000,-1000000000})  {
                
            min = key*size;
            max = (key+1)*size;
            
        }
        
                
        void expand(int64 id, int32 x, int32 y) {
            if ((id<min) || (id>= max)) { throw std::domain_error("wtf"); }
            
            size_t i = id-min;
            WayBBox& box = boxes[i];
            if (x < box.minx) { box.minx=x; }
            if (y < box.miny) { box.miny=y; }
            if (x > box.maxx) { box.maxx=x; }
            if (y > box.maxy) { box.maxy=y; }
            
        }
        void reset() {
            boxes.clear();
            std::vector<WayBBox> empty;
            boxes.swap(empty);
        }
        
        std::shared_ptr<QtStore> calculate(double buffer, int64 maxdepth, bool use_48bit_quadtrees) {
            
            
            if (use_48bit_quadtrees) {
                auto qtstore = make_qtstore_48bit(min, min+size, key);
                for (size_t i=0; i < size; i++) {
                    const WayBBox& box = boxes[i];
                    if (box.minx < 2000000000) {
                        
                        qtstore->expand(i+min, quadtree::calculate(box.minx, box.miny, box.maxx, box.maxy, buffer, maxdepth));
                    }
                }
                return qtstore;
            }
            
            std::vector<int64> qts(size,-1);
            size_t count=0;
            for (size_t i=0; i < size; i++) {
                const WayBBox& box = boxes[i];
                if (box.minx < 2000000000) {
                    count++;
                    qts[i] = quadtree::calculate(box.minx, box.miny, box.maxx, box.maxy, buffer, maxdepth);
                }
            }
            reset();
            return make_qtstore_vector_move(std::move(qts), min, count, key);
            
        }
        
    
        int64 key;
    private:
        size_t size;
        int64 min;
        int64 max;
        std::vector<WayBBox> boxes;
        
                
};
/*
std::string getmem(size_t);
int64 getmemval(size_t);        
  */      
        
class AddLocationsToWayNodes {
    public:
        AddLocationsToWayNodes(std::shared_ptr<WayNodesFile> wnf, std::function<void(std::shared_ptr<WayNodeLocationBlock>)> expand_all_, int64 minway, int64 maxway) :
            expand_all(expand_all_), missing(0), atend(0) {
        
            next_block = inverted_callback<WayNodes>::make([wnf,minway,maxway](std::function<void(std::shared_ptr<WayNodes>)> ww) {
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
            
            curr = make_waynodelocationblock(block->key(), block->size());
            
            
        }
        
        
        void call(minimal::BlockPtr mb) {
            
            if (!mb) {
                while (n>0) {
                    next_wn();
                    missing++;
                }
                Logger::Message() << "finished, have " << missing << " missing, " << atend << " atend";
                expand_all(nullptr);
                return;
            }
            if (mb->nodes.empty()) { return; }
            
            if ((mb->index % 100) == 0) {
                int64 now=getmemval(pid);
                
                Logger::Progress lp(mb->file_progress);
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
                while ((n>0) && ((int64) nd.id > n)) {
                    missing++;
                    next_wn();
                }
                if (n<0) {
                    atend++;
                } else if ((int64) nd.id < n) {
                    //pass
                } else {
                    
                    while ((int64) nd.id == n) {
                    
                        curr->vals.push_back(std::move(WayNodeLocation{w, nd.lon, nd.lat}));
                        next_wn();
                    }
                }
                
            }
        }
    private:
        std::function<void(std::shared_ptr<WayNodeLocationBlock>)> expand_all;
        size_t missing;
        size_t atend;
        std::function<std::shared_ptr<WayNodes>()> next_block;
        std::shared_ptr<WayNodes> block;
        size_t pos;
        std::shared_ptr<WayNodeLocationBlock> curr;
        
        int64 w,n;
        
        void next_wn() {
            pos++;
            while (block && (pos >= block->size())) {
                if (! curr->vals.empty()) { expand_all(curr); }
                block = next_block();
                if (block) {
                    curr = make_waynodelocationblock(block->key(), block->size());
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
        

class ExpandWayBBoxes {
    public:
        ExpandWayBBoxes(size_t split_, size_t nt, bool use_48bit_quadtrees_) : split(split_), tiles(nt), use_48bit_quadtrees(use_48bit_quadtrees_) {
            pid = getpid();
            cc=0;
            mn=1ll<<61; mx=0;
        }
        
        virtual ~ExpandWayBBoxes() { Logger::Message() << "~ExpandBoxes"; }
        
        void expand(int64 w, int32 x, int32 y) {
            if (w<0) { throw std::domain_error("wtf"); }
            int64 k = (w/split);
            
            if (((size_t) k)>=tiles.size()) {
                Logger::Message() << "resize ExpandBoxes to " << k+1 << " {" << 16*(k+1) << "mb}, RSS=" << getmemval(getpid())/1024.0/1024;
                tiles.resize(k+1);
            }
            if (!tiles[k]) {
                
                
                cc++;
                try {
                    tiles[k] = std::make_unique<WayBBoxTile>(k, split);
                    
                } catch(std::exception& ex) {
                    Logger::Message() << "ExpandBoxes adding tile " << k << " failed, RSS=" << getmemval(getpid())/1024.0/1024;
                    throw ex;
                }
                
            }
            
            tiles[k]->expand(w,x,y);
        }
        
        void expand_all(std::shared_ptr<WayNodeLocationBlock> ww) {
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
                //Logger::Message() << "range " << mn << " [" << (mn/split) << "] to " << mx << " [" << (mx/split) << "]";
            }
        }
        
        
        
        void calculate(std::shared_ptr<QtStoreSplit> qts, double buffer, size_t maxdepth) {
            int64 now=getmemval(pid);
            Logger::Message() << "[" << std::setw(8)  << cc << " tiles, " << std::setw(12) << now*1.0/1024.0/1024.0  << "]";
            int64 was=now;
           
            
            for (size_t i=0; i < tiles.size(); i++) {
                if (tiles[i]) {
                    Logger::Progress(100.0*i/tiles.size()) <<  "calc tile " << i;
                    
                    qts->add_tile(i,tiles[i]->calculate(buffer, maxdepth,use_48bit_quadtrees));
                    tiles[i].reset();
                    
                }
            }
            
            
            tiles.clear();
            
            now=getmemval(pid);
            Logger::Message() << "[" << cc << " tiles, " << now*1.0/1024.0/1024.0 << "/" << (now-was)*1.0/1024.0/1024.0 << "]";
            
            
        }
    private:
       
        size_t split;
        size_t pid;
        
        std::vector<std::unique_ptr<WayBBoxTile>> tiles;
        size_t cc;
        
        int64 mn, mx;
        bool use_48bit_quadtrees;
};          
        
        

void find_way_quadtrees(
    const std::string& source_filename,
    const std::vector<int64>& source_locs, 
    size_t numchan,
    std::shared_ptr<QtStoreSplit> way_qts,
    std::shared_ptr<WayNodesFile> wns,
    double buffer, size_t max_depth,int64 minway, int64 maxway,
    bool use_48bit_quadtrees) {
    
    
    
    size_t nb=580;
    
    Logger::Message() << "find way qts " << minway << " to " << maxway << ", RSS=" << getmemval(getpid())/1024.0/1024;
    auto expand=std::make_shared<ExpandWayBBoxes>(1<<20, nb, use_48bit_quadtrees);
    auto expand_all = [expand](std::shared_ptr<WayNodeLocationBlock> ww) { expand->expand_all(ww); };
    
    
    auto wnla = std::make_shared<AddLocationsToWayNodes>(wns, expand_all, minway, maxway);
    read_blocks_minimalblock(source_filename, [wnla](minimal::BlockPtr mb) { wnla->call(mb); }, source_locs, numchan, 1 | 48);
    
    
    Logger::Get().time("expand way bboxes");
    
    
    expand->calculate(way_qts, buffer, max_depth);
    Logger::Get().time("calculate way qts");
}

}
