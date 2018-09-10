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

#include "oqt/sorting/sortgroup.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/timer.hpp"
#include "oqt/utils/logger.hpp"

namespace oqt {
    

        
class SortGroup : public SplitBlocks {
    public:
        SortGroup(primitiveblock_callback callback, 
             std::shared_ptr<qttree> tree_, size_t blocksplit_, size_t writeat) :
             SplitBlocks(callback,blocksplit_,writeat,true),
             tree(tree_), blocksplit(blocksplit_) {
            
            mxt=tree->size()/blocksplit+1;
                
        }
        virtual ~SortGroup() {}
        virtual size_t find_tile(element_ptr obj) {
            return tree->find_tile(obj->Quadtree()).idx / blocksplit;
        }
        virtual size_t max_tile() { return mxt; }
    private:
        std::shared_ptr<qttree> tree;
        size_t blocksplit;
        size_t mxt;
};
primitiveblock_callback make_sortgroup_callback(primitiveblock_callback packers, std::shared_ptr<qttree> groups, size_t blocksplit, size_t writeat) {
    auto sg = std::make_shared<SortGroup>(packers,groups,blocksplit,writeat);
    return [sg](primitiveblock_ptr bl) {
        sg->call(bl);
    };
}    
    
    
}
