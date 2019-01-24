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

#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/writeblock.hpp"

#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/calcqts/writeqts.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/pbf/packedint.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/logger.hpp"

#include <algorithm>
#include <chrono>

namespace oqt {

    
    
struct QtsBlock {
    size_t idx;
    int64 ty;
    std::vector<int64> refs;
    std::vector<int64> qts;
};

std::string pack_qtsblock_data(std::shared_ptr<QtsBlock> bl) {
    std::list<PbfTag> mm;
    if (bl->ty==0) {
        std::string ids = write_packed_delta(bl->refs);
        std::string qts = write_packed_delta(bl->qts);
        std::string lns(bl->refs.size(),0);
        
        std::string result(ids.size()+qts.size()+2*lns.size()+40,0);
        size_t pos=0;

        pos = write_pbf_data(result,pos,1,ids);
        pos = write_pbf_data(result,pos,8,lns);
        pos = write_pbf_data(result,pos,9,lns);
        pos = write_pbf_data(result,pos,20,qts);
        result.resize(pos);

        mm.push_back(PbfTag{2,0,result});
    } else {
        for (size_t i=0; i < bl->refs.size(); i++) {
            std::string res(20,0);
            size_t pos = write_pbf_value(res,0,1,uint64(bl->refs.at(i)));
            pos = write_pbf_value(res,pos,20,zig_zag(bl->qts.at(i)));
            res.resize(pos);
            size_t x = 2 + bl->ty;
            mm.push_back(PbfTag{x, 0, res});
        }       
    }
    
    return pack_pbf_tags({PbfTag{2,0,pack_pbf_tags(mm)}});    
    
}


keystring_ptr pack_qtsblock(std::shared_ptr<QtsBlock> qq) {
    
    if (!qq) { return nullptr; }
    
    std::string bl = pack_qtsblock_data(qq);
    
    auto fb = prepare_file_block("OSMData", bl, -1);
    return std::make_shared<keystring>(qq->idx,fb);
}   





class CollectQtsImpl : public CollectQts {
    public:
        typedef std::shared_ptr<QtsBlock> qtsblock_ptr;
        typedef std::function<void(qtsblock_ptr)> callback;
        
        CollectQtsImpl(std::vector<callback> callbacks_, size_t blocksize_) :
            callbacks(callbacks_), blocksize(blocksize_), idx(0) {
            
            resetcurr(0);
            
        }
        
        virtual ~CollectQtsImpl() {}
        
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
            curr=std::make_shared<QtsBlock>();
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

std::shared_ptr<CollectQts> make_collectqts(const std::string& qtsfn, size_t numchan, size_t blocksize) {
    auto write_file_obj = make_pbffilewriter(qtsfn, nullptr);
    
    auto writeblocks = multi_threaded_callback<keystring>::make(
        [write_file_obj](std::shared_ptr<keystring> ks) {
            if (ks) {
                write_file_obj->writeBlock(ks->first,ks->second);
            } else {
                write_file_obj->finish();
            }
        }, numchan);
    
    std::vector<std::function<void(std::shared_ptr<QtsBlock>)>> packers;
    for (size_t i=0; i < numchan; i++) {
        auto wb = writeblocks[i];
        packers.push_back(threaded_callback<QtsBlock>::make(
            [wb](std::shared_ptr<QtsBlock> qv) {
                if (!qv) {
                    wb(nullptr);
                } else {
                    wb(pack_qtsblock(qv));
                }
            }
        ));
    }
    
    
    return std::make_shared<CollectQtsImpl>(packers, blocksize);
}
}
