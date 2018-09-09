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

namespace oqt {
    
            
class CollectObjs {
    public:
        CollectObjs(std::vector<primitiveblock_callback> cb_, size_t blocksize_, bool checkorder_) : cb(cb_), idx(0), blocksize(blocksize_), checkorder(checkorder_), prev(0) {
            
            curr = std::make_shared<primitiveblock>(idx,blocksize);
            
        }
        
        void add(element_ptr obj) {
            if (curr->size()==blocksize) {
                cb[idx%cb.size()](curr);
                idx++;
                curr = std::make_shared<primitiveblock>(idx,blocksize);
                
                
            }
            
            if (checkorder) {
                if (!(obj->InternalId() > prev)) {
                    throw std::domain_error(
                        "CollectObjs out of sequence @"+
                        std::to_string(idx*blocksize+curr->size())+
                        ": "+std::to_string(prev>>61)+
                        " "+std::to_string(prev&0x1fffffffffffffffll)+
                        " followed by "+std::to_string(obj->Type())+
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
        
        primitiveblock_ptr curr;
};

std::function<void(element_ptr)> make_collectobjs(std::vector<primitiveblock_callback> callbacks, size_t blocksize)  {
    auto collect = std::make_shared<CollectObjs>(callbacks,blocksize,true);
    return [collect](element_ptr obj) {
        if (obj) {
            collect->add(obj);
        } else {
            collect->finish();
        }
    };
}


}
