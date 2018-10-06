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

#include "oqt/sorting/resortobjects.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/timing.hpp"
#include "oqt/utils/logger.hpp"

#include <algorithm>

namespace oqt {



class ResortObjects {
    public:
        ResortObjects(primitiveblock_callback callback_, std::shared_ptr<qttree> groups_, int64 blocksize_, bool sortobjs_) :
                callback(callback_), groups(groups_), blocksize(blocksize_), sortobjs(sortobjs_) {}
        
        void call(PrimitiveBlockPtr inobjs) {
            if (!inobjs) { return callback(nullptr); }
            
            std::vector<PrimitiveBlockPtr> result(blocksize);
            size_t off = (inobjs->Index())*blocksize;
            int64 tt=0;
            for (auto o: inobjs->Objects()) {
                const auto& tile = groups->find_tile(o->Quadtree());
                if (tile.idx < off) { throw std::domain_error("wrong tile"); }
                size_t ii = tile.idx-off;
                if (ii>=blocksize) {throw std::domain_error("wrong tile"); }
                if (!result.at(ii)) {
                    result.at(ii) = std::make_shared<PrimitiveBlock>(ii, tile.weight);
                    result.at(ii)->SetQuadtree(tile.qt);
                    tt++;
                }
                result.at(ii)->add(o);
            }
            
            Logger::Progress(100.0*off/groups->size()) << "ResortObjects [" << ts << "] "
                << "finish_current: " << inobjs->Index() << ": have " << tt
                << " tiles // " << inobjs->size() << " objs";
            
           
            for (auto t: result) {
                if (t) {
                    if (sortobjs) {
                        std::sort(t->Objects().begin(), t->Objects().end(), element_cmp);
                    }
                    callback(t);
                }
            }
        }
    private:
        primitiveblock_callback callback;
        std::shared_ptr<qttree> groups;
        size_t blocksize;
        TimeSingle ts;
        bool sortobjs;
}; 

primitiveblock_callback make_resortobjects_callback(
    primitiveblock_callback callback,
    std::shared_ptr<qttree> groups,
    int64 blocksplit, bool sortobjs) {
    
    auto rso = std::make_shared<ResortObjects>(callback,groups,blocksplit,sortobjs);
    return [rso](PrimitiveBlockPtr tl) { rso->call(tl); };
}


}
