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

#include "oqt/sorting/splitbyid.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/timing.hpp"
#include "oqt/utils/logger.hpp"

namespace oqt {
    
            
class CollectObjs {
    public:
        CollectObjs(std::vector<primitiveblock_callback> cb_, size_t blocksize_, bool checkorder_) : cb(cb_), idx(0), blocksize(blocksize_), checkorder(checkorder_), prev(0) {
            
            curr = std::make_shared<PrimitiveBlock>(idx,blocksize);
            
        }
        
        void add(ElementPtr obj) {
            if (curr->size()==blocksize) {
                cb[idx%cb.size()](curr);
                idx++;
                curr = std::make_shared<PrimitiveBlock>(idx,blocksize);
                
                
            }
            
            if (checkorder) {
                if (!(obj->InternalId() > prev)) {
                    throw std::domain_error(
                        "CollectObjs out of sequence @"+
                        std::to_string(idx*blocksize+curr->size())+
                        ": "+std::to_string(prev>>61)+
                        " "+std::to_string(prev&0x1fffffffffffffffll)+
                        " followed by "+std::to_string(int(obj->Type()))+
                        " "+std::to_string(obj->Id())+
                        " [ "+std::to_string(obj->Info().version)+"]");
                }
                prev= obj->InternalId();
            }
            curr->add(obj);
        }
        
        void finish() {
            
            
            if (curr->size()>0) {
                cb[idx%cb.size()](curr);
            }
            curr.reset();
            for (auto c: cb) { c(nullptr); }
            
        }
    private:
        std::vector<primitiveblock_callback> cb;
        size_t idx;
        size_t blocksize;
        bool checkorder;
        uint64 prev;
        
        PrimitiveBlockPtr curr;
};

std::function<void(ElementPtr)> make_collectobjs(std::vector<primitiveblock_callback> callbacks, size_t blocksize)  {
    auto collect = std::make_shared<CollectObjs>(callbacks,blocksize,true);
    return [collect](ElementPtr obj) {
        if (obj) {
            collect->add(obj);
        } else {
            collect->finish();
        }
    };
}

int64 split_id_key(ElementPtr obj, int64 node_split_at, int64 way_split_at, int64 offset) {
    int64 i = obj->Id();
    ElementType t = obj->Type();
    if (t==ElementType::Node) {
        int64 x = i / node_split_at;
        if (x>=offset) { x=offset-1; }
        return x;
    } else if (t==ElementType::Way) {
        int64 x = i / way_split_at;
        if (x>=offset) { x=offset-1; }
        return offset+x;
    } else if (t==ElementType::Relation) {
        int64 x =  i / (way_split_at/10);
        if (x>=offset) { x=offset-1; }
        return 2*offset+x;
    }
    return -1;
}
        
class SplitById : public SplitBlocks {
    public:
        SplitById(primitiveblock_callback callbacks, size_t blocksplit, size_t writeat, int64 node_split_at_, int64 way_split_at_, int64 offset_) :
            SplitBlocks(callbacks,blocksplit,writeat,true), node_split_at(node_split_at_), way_split_at(way_split_at_), offset(offset_) {}
        
        virtual ~SplitById() {}
        virtual size_t find_tile(ElementPtr obj) {
            return split_id_key(obj, node_split_at, way_split_at, offset);
        }
        virtual size_t max_tile() { return 3*offset; }
        
    private:
        int64 node_split_at;
        int64 way_split_at;
        
        int64 offset;
};

primitiveblock_callback make_splitbyid_callback(primitiveblock_callback packers, size_t blocksplit, size_t writeat, int64 split_at) {
    auto sg = std::make_shared<SplitById>(packers,blocksplit,writeat,split_at, split_at / 8, ((1ll)<<34) / split_at);
    return [sg](PrimitiveBlockPtr bl) { sg->call(bl); };
}

}
