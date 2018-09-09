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

#include "oqt/sorting/final.hpp"
#include "oqt/sorting/tempobjs.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/calcqts/qttreegroups.hpp"
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <thread>


#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/timer.hpp"
#include "oqt/utils/logger.hpp"


namespace oqt {
    

class packfinal {
    public:
        packfinal(write_file_callback cb_, int64 enddate_, bool writeqts_, size_t ii_, int complevel_) :
            cb(cb_), enddate(enddate_), writeqts(writeqts_), ii(ii_), complevel(complevel_) {}//, nb(0),no(0),sort(0),pack(0),comp(0),writ(0) {}
        
        void call(primitiveblock_ptr oo) {
            if (!oo) {
                cb(nullptr);
                return;
            }
            //time_single t;
            if (enddate>0) {
                oo->enddate=enddate;
            }
            std::sort(oo->objects.begin(), oo->objects.end(), element_cmp);
            auto p = writePbfBlock(oo, writeqts, false, true, true);
            auto q = std::make_shared<keystring>(writeqts ? oo->quadtree : oo->index, prepareFileBlock("OSMData", p,complevel));
            
            cb(q);
            
        }
    private:
        write_file_callback cb;
        int64 enddate;
        bool writeqts;
        size_t ii;
        int complevel;
        
};

primitiveblock_callback make_pack_final(write_file_callback cb, int64 enddate, bool writeqts, size_t ii, int complevel) {
    auto pfu = std::make_shared<packfinal>(cb,enddate,writeqts,ii,complevel);
    return [pfu](primitiveblock_ptr oo) { pfu->call(oo); };
}   


std::vector<primitiveblock_callback> make_final_packers(std::shared_ptr<PbfFileWriter> write_file_obj, size_t numchan, int64 timestamp, bool writeqts, bool asthread) {
    
    auto writers = threaded_callback<keystring>::make(
        [write_file_obj](keystring_ptr p) {
            if (p) {
                write_file_obj->writeBlock(p->first,p->second);
            }
    },numchan);
    
    
    std::vector<primitiveblock_callback> packers;
    for (size_t i=0; i < numchan; i++) {
        auto cb=make_pack_final(writers, timestamp, writeqts, i, -1);
        
        if (asthread) {
            packers.push_back(threaded_callback<primitiveblock>::make(cb));
        } else {
            packers.push_back(cb);
        }
    }
    return packers;
};

std::vector<primitiveblock_callback> make_final_packers_cb(std::function<void(keystring_ptr)> ks_cb, size_t numchan, int64 timestamp, bool writeqts, bool asthread) {
    
    auto writers = threaded_callback<keystring>::make(ks_cb,numchan);
    
    
    std::vector<primitiveblock_callback> packers;
    for (size_t i=0; i < numchan; i++) {
        auto cb=make_pack_final(writers, timestamp, writeqts, i,1);
        
        if (asthread) {
            packers.push_back(threaded_callback<primitiveblock>::make(cb));
        } else {
            packers.push_back(cb);
        }
    }
    return packers;
};


std::vector<primitiveblock_callback> make_final_packers_sync(std::shared_ptr<PbfFileWriter> write_file_obj, size_t numchan, int64 timestamp, bool writeqts, bool asthread) {
    
    auto writers = multi_threaded_callback<keystring>::make(
        [write_file_obj](keystring_ptr p) {
            if (p) {
                write_file_obj->writeBlock(p->first,p->second);
            }
    },numchan);
    
    
    std::vector<primitiveblock_callback> packers;
    for (size_t i=0; i < numchan; i++) {
        auto cb=make_pack_final(writers.at(i), timestamp, writeqts, i,-1);
        
        if (asthread) {
            packers.push_back(threaded_callback<primitiveblock>::make(cb));
        } else {
            packers.push_back(cb);
        }
    }
    return packers;
};





    
        
        

        
        
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
class SplitById : public SplitBlocks {
    public:
        SplitById(primitiveblock_callback callbacks, size_t blocksplit, size_t writeat, int64 node_split_at_, int64 way_split_at_, int64 offset_) :
            SplitBlocks(callbacks,blocksplit,writeat,true), node_split_at(node_split_at_), way_split_at(way_split_at_), offset(offset_) {}
        
        virtual ~SplitById() {}
        virtual size_t find_tile(element_ptr obj) {
            int64 i = obj->Id();
            elementtype t = obj->Type();
            if (t==elementtype::Node) {
                int64 x = i / node_split_at;
                if (x>=offset) { x=offset-1; }
                return x;
            } else if (t==elementtype::Way) {
                int64 x = i / way_split_at;
                if (x>=offset) { x=offset-1; }
                return offset+x;
            } else if (t==elementtype::Relation) {
                int64 x =  i / (way_split_at/10);
                if (x>=offset) { x=offset-1; }
                return 2*offset+x;
            }
            return -1;
        }
        virtual size_t max_tile() { return 3*offset; }
        
    private:
        int64 node_split_at;
        int64 way_split_at;
        
        int64 offset;
};

primitiveblock_callback make_splitbyid_callback(primitiveblock_callback packers, size_t blocksplit, size_t writeat, int64 split_at) {
    auto sg = std::make_shared<SplitById>(packers,blocksplit,writeat,split_at, split_at / 8, ((1ll)<<34) / split_at);
    return [sg](primitiveblock_ptr bl) { sg->call(bl); };
}

class ResortObjects {
    public:
        ResortObjects(primitiveblock_callback callback_, std::shared_ptr<qttree> groups_, int64 blocksize_, bool sortobjs_) :
                callback(callback_), groups(groups_), blocksize(blocksize_), sortobjs(sortobjs_) {}
        
        void call(primitiveblock_ptr inobjs) {
            if (!inobjs) { return callback(nullptr); }
            
            std::vector<primitiveblock_ptr> result(blocksize);
            size_t off = (inobjs->index)*blocksize;
            int64 tt=0;
            for (auto o: inobjs->objects) {
                const auto& tile = groups->find_tile(o->Quadtree());
                if (tile.idx < off) { throw std::domain_error("wrong tile"); }
                size_t ii = tile.idx-off;
                if (ii>=blocksize) {throw std::domain_error("wrong tile"); }
                if (!result.at(ii)) {
                    result.at(ii) = std::make_shared<primitiveblock>(ii, tile.weight);
                    result.at(ii)->quadtree=tile.qt;
                    tt++;
                }
                result.at(ii)->add(o);
            }
            
            logger_progress(100.0*off/groups->size()) << "ResortObjects [" << TmStr{ts.since(),6,1} << "] "
                << "finish_current: " << inobjs->index << ": have " << tt
                << " tiles // " << inobjs->objects.size() << " objs";
            
           
            for (auto t: result) {
                if (t) {
                    if (sortobjs) {
                        std::sort(t->objects.begin(), t->objects.end(), element_cmp);
                    }
                    callback(t);
                }
            }
        }
    private:
        primitiveblock_callback callback;
        std::shared_ptr<qttree> groups;
        size_t blocksize;
        time_single ts;
        bool sortobjs;
}; 

primitiveblock_callback make_resortobjects_callback(
    primitiveblock_callback callback,
    std::shared_ptr<qttree> groups,
    int64 blocksplit, bool sortobjs) {
    
    auto rso = std::make_shared<ResortObjects>(callback,groups,blocksplit,sortobjs);
    return [rso](primitiveblock_ptr tl) { rso->call(tl); };
}
 
}
