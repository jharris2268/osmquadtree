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
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/timing.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/utils/compress.hpp"
#include "oqt/utils/operatingsystem.hpp"

#include <algorithm>

namespace oqt {

/*

class ResortObjects {
    public:
        ResortObjects(primblock_vec_callback callback_, std::shared_ptr<QtTree> groups_, int64 blocksize_, bool sortobjs_) :
                callback(callback_), groups(groups_), blocksize(blocksize_), sortobjs(sortobjs_), pid(getpid()) {}
        
        void call(PrimitiveBlockPtr inobjs) {
            if (!inobjs) { return callback(nullptr); }
            
            auto result = std::make_shared<std::vector<PrimitiveBlockPtr>>(blocksize);
            size_t off = (inobjs->Index())*blocksize;
            int64 tt=0;
            for (auto o: inobjs->Objects()) {
                const auto& tile = groups->find_tile(o->Quadtree());
                if (tile.idx < off) { throw std::domain_error("wrong tile"); }
                size_t ii = tile.idx-off;
                if (ii>=blocksize) {throw std::domain_error("wrong tile"); }
                if (!result->at(ii)) {
                    result->at(ii) = std::make_shared<PrimitiveBlock>(ii, tile.weight);
                    result->at(ii)->SetQuadtree(tile.qt);
                    tt++;
                }
                result->at(ii)->add(o);
            }
            if ((inobjs->Index()%10)==0) {
                Logger::Progress(100.0*off/groups->size()) << "ResortObjects [" << ts << "] "
                    << "finish_current: " << inobjs->Index() << ": have " << tt
                    << " tiles // " << inobjs->size() << " objs [" << getmem(pid) << "]";
            }
            
            if (sortobjs) {
                for (auto t: *result) {
                    if (t) {
                        std::sort(t->Objects().begin(), t->Objects().end(), element_cmp);
                    }
                    
                }
            }
            callback(result);
        }
    private:
        //primitiveblock_callback callback;
        primblock_vec_callback callback;
        std::shared_ptr<QtTree> groups;
        size_t blocksize;
        TimeSingle ts;
        bool sortobjs;
        int pid;
}; 

primitiveblock_callback make_resortobjects_callback(
    //primitiveblock_callback callback,
    primblock_vec_callback callback,
    std::shared_ptr<QtTree> groups,
    int64 blocksplit, bool sortobjs) {
    
    auto rso = std::make_shared<ResortObjects>(callback,groups,blocksplit,sortobjs);
    return [rso](PrimitiveBlockPtr tl) { rso->call(tl); };
}
*/

PrimitiveBlockPtr prep_prim_block(size_t idx, double pf, int64 qt, int64 ts) {
    auto bl = std::make_shared<PrimitiveBlock>(idx,0);
    bl->SetQuadtree(qt);
    bl->SetEndDate(ts);
    bl->SetFileProgress(pf*idx);
    
    return bl;
}


std::shared_ptr<std::map<int64, PrimitiveBlockPtr>> resort_objects(std::shared_ptr<QtTree> groups, bool sortobjs, int64 timestamp, std::shared_ptr<KeyedBlob> inblocks) {

//std::shared_ptr<std::vector<keystring_ptr>> resort_objects(std::shared_ptr<QtTree> groups, bool sortobjs, int64 timestamp, int complevel, std::shared_ptr<KeyedBlob> inblocks) {
    
    auto sorted_blocks=std::make_shared<std::map<int64, PrimitiveBlockPtr>>();
    double pf = 100.0/groups->size();
    for (const auto& x: inblocks->blobs) {
        auto dd = decompress(x.first,x.second);
        auto bl = read_primitive_block(inblocks->key,dd,false,ReadBlockFlags::Empty,nullptr,nullptr);
        for (auto o: bl->Objects()) {
            const auto& tile = groups->find_tile(o->Quadtree());
            
            if (sorted_blocks->count(tile.qt)==0) {
                sorted_blocks->emplace(tile.qt, prep_prim_block(tile.idx, pf, tile.qt, timestamp));
            }
            sorted_blocks->at(tile.qt)->add(o);
        }
    }
    
    
    
    for (const auto& kv: *sorted_blocks) {
        if (sortobjs) {
            std::sort(kv.second->Objects().begin(), kv.second->Objects().end(), element_cmp);
        }
    }
    
    return sorted_blocks;
}




    

std::vector<keyedblob_callback> make_resortobjects_callback_alt(
    std::shared_ptr<QtTree> groups, bool sortobjs,
    int64 timestamp, int complevel,
    std::function<void(keystring_ptr)> cb, size_t numchan) {
        
        
    
    auto cb_collect = multi_threaded_callback<std::vector<keystring_ptr>>::make(
        [cb](std::shared_ptr<std::vector<keystring_ptr>> inb) {
            if (!inb) {
                cb(nullptr);
                return;
            }
            
            for (auto bl: *inb) {
                cb(bl);
            }
        }, numchan);
    
    
    
    std::vector<keyedblob_callback> out;
    
    
    int pid=getpid();
    for (auto cbc: cb_collect) {
        TimeSingle ts;
        out.push_back(
            threaded_callback<KeyedBlob>::make(
                [cbc, groups, sortobjs, timestamp, complevel,ts,pid](std::shared_ptr<KeyedBlob> kb) {
                    if (!kb) {
                        cbc(nullptr);
                        return;
                    }
                    
                    auto sorted_blocks = resort_objects(groups, sortobjs, timestamp, kb);
                    
                    
                    if ((!sorted_blocks->empty()) && ((kb->key % 10)==0)) {
                        double fp = sorted_blocks->rbegin()->second->FileProgress();
                        Logger::Progress(fp) << "resort_objects_alt [" << ts << "] "
                            << "finish_current: " << kb->key << " [" <<kb->blobs.size() << " blobs] converted, "
                            << " to " << sorted_blocks->size() << " tiles "
                            << "[" << getmem(pid) << "]";
                    }
                    
                    
                    auto result = std::make_shared<std::vector<keystring_ptr>>();
                    for (const auto& kv: *sorted_blocks) {
                       
                           
                        auto p = pack_primitive_block(kv.second, true, false, true, true);
                        result->push_back(std::make_shared<keystring>(kv.first, prepare_file_block("OSMData", p,complevel)));
                    }
                    cbc(result);                    
                }
            )
        );
    }
    
    return out;
}
        

std::vector<keyedblob_callback> make_resort_objects_collect_block(
    std::shared_ptr<QtTree> groups, bool sortobjs,
    int64 timestamp,
    primitiveblock_callback cb, size_t numchan) {

    int pid=getpid();
    if (numchan==0) {
        TimeSingle ts;
        
        return {
            
            
            [cb, groups, sortobjs, timestamp,ts,pid](std::shared_ptr<KeyedBlob> kb) {
                if (!kb) { cb(nullptr); return; }
                auto sorted_blocks = resort_objects(groups, sortobjs, timestamp, kb);
                    
                    
                if ((!sorted_blocks->empty()) && ((kb->key % 10)==0)) {
                    double fp = sorted_blocks->rbegin()->second->FileProgress();
                    Logger::Progress(fp) << "resort_objects_alt [" << ts << "] "
                        << "finish_current: " << kb->key << " [" <<kb->blobs.size() << " blobs] converted, "
                        << " to " << sorted_blocks->size() << " tiles "
                        << "[" << getmem(pid) << "]";
                }
                for (const auto& bl: *sorted_blocks) {
                    cb(bl.second);
                }
            }};
    }
        
        


    auto cb_collect = multi_threaded_callback<std::map<int64, PrimitiveBlockPtr>>::make(
        [cb](std::shared_ptr<std::map<int64, PrimitiveBlockPtr>> inb) {
            if (!inb) {
                cb(nullptr);
                return;
            }
            
            for (const auto& bl: *inb) {
                cb(bl.second);
            }
        }, numchan);
    
    
    
    std::vector<keyedblob_callback> out;
    
    
    
    for (auto cbc: cb_collect) {
        TimeSingle ts;
        out.push_back(
            threaded_callback<KeyedBlob>::make(
                [cbc, groups, sortobjs, timestamp, ts,pid](std::shared_ptr<KeyedBlob> kb) {
                    if (!kb) {
                        cbc(nullptr);
                        return;
                    }
                    
                    auto sorted_blocks = resort_objects(groups, sortobjs, timestamp, kb);
                    
                    
                    if ((!sorted_blocks->empty()) && ((kb->key % 10)==0)) {
                        double fp = sorted_blocks->rbegin()->second->FileProgress();
                        Logger::Progress(fp) << "resort_objects_alt [" << ts << "] "
                            << "finish_current: " << kb->key << " [" <<kb->blobs.size() << " blobs] converted, "
                            << " to " << sorted_blocks->size() << " tiles "
                            << "[" << getmem(pid) << "]";
                    }
                    
                    
                    cbc(sorted_blocks);                    
                }
            )
        );
    }
    
    return out;
}

    
                

    
    


}
