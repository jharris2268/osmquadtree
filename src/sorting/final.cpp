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
    

class packfinal {
    public:
        packfinal(write_file_callback cb_, int64 enddate_, bool writeqts_, size_t ii_, int complevel_) :
            cb(cb_), enddate(enddate_), writeqts(writeqts_), ii(ii_), complevel(complevel_) {}//, nb(0),no(0),sort(0),pack(0),comp(0),writ(0) {}
        
        void call(PrimitiveBlockPtr oo) {
            if (!oo) {
                cb(nullptr);
                return;
            }
            
            if (enddate>0) {
                oo->SetEndDate(enddate);
            }
            std::sort(oo->Objects().begin(), oo->Objects().end(), element_cmp);
            auto p = writePbfBlock(oo, writeqts, false, true, true);
            auto q = std::make_shared<keystring>(writeqts ? oo->Quadtree() : oo->Index(), prepareFileBlock("OSMData", p,complevel));
            
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
    return [pfu](PrimitiveBlockPtr oo) { pfu->call(oo); };
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
            packers.push_back(threaded_callback<PrimitiveBlock>::make(cb));
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
            packers.push_back(threaded_callback<PrimitiveBlock>::make(cb));
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
            packers.push_back(threaded_callback<PrimitiveBlock>::make(cb));
        } else {
            packers.push_back(cb);
        }
    }
    return packers;
};



    
    
    
    
    
    
    
    
    
}
