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

#include "oqt/sorting/common.hpp"
#include "oqt/utils/threadedcallback.hpp"

#include "oqt/utils/timer.hpp"
#include "oqt/utils/logger.hpp"

namespace oqt {



struct SplitBlocksDetail {
    
    size_t writeat;
    primitiveblock_callback callback;
    bool msgs;
    
    size_t ii;
    size_t ntile;
    time_single ts;
    size_t tb;
    size_t currww;
    double maxp;
    
    double ltt();
    
    void writetemps(std::shared_ptr<std::vector<primitiveblock_ptr>>);
    
    double tmm,tww,trr,last;
    
    void finished_message() {
        logger_progress(100) << " finished SplitBlocks " << TmStr{ts.since(),6,1} << " [" << ii << " blocks, " << tb << " objects ], tmm=" << TmStr{tmm,6,1} << ", trr=" << TmStr{trr,8,1} << ", tww=" << TmStr{tww,8,1} << " {" << std::this_thread::get_id() << "}";
    }
    
};



        
        
        
        
double SplitBlocksDetail::ltt() {
    double m = ts.since();
    double n = m-last;
    last=m;
    return n;
}
void SplitBlocksDetail::writetemps(std::shared_ptr<std::vector<primitiveblock_ptr>> temps) {
    if (!temps) {
        return;
    }
        
        
    double x=ts.since();
    size_t a=0,b=0;
    for (auto& p : *temps) {
        if (p) {
            a++;
            b+=p->size();
            callback(p);
            p.reset();
            ++ii;
        }
    }
    
    tb+=b;
    if (msgs) {
        logger_progress(maxp) << "SplitBlocks: write " << a << " blocks // " << b << " objs"
            << " [" << currww << "/" << writeat << " " << TmStr{ts.since()-x,4,1} << "]";
    }
    trr += (ts.since()-x);
}



SplitBlocks::SplitBlocks(primitiveblock_callback callback_, size_t blocksplit_, size_t writeat_, bool msgs_)
    : blocksplit(blocksplit_), detail(std::make_unique<SplitBlocksDetail>()) {
        
        
        detail->callback = callback_;
        detail->writeat = writeat_;
        detail->msgs = msgs_;
        
        
        detail->ii = 0; detail->ntile = 0;
        detail->tb = 0; detail->currww = 0;
        detail->tmm=0;
        detail->tww=0;
        detail->trr=0;
        detail->last=detail->ts.since();
        temps = std::make_shared<tempsvec>();
        
        call_writetemps = threaded_callback<tempsvec>::make([this](std::shared_ptr<tempsvec> t) { detail->writetemps(t); });
        detail->maxp=0;
}

SplitBlocks::~SplitBlocks() {}

void SplitBlocks::call(primitiveblock_ptr bl) {
    if (!bl) {
        call_writetemps(temps);
        detail->finished_message();
        //callback(nullptr);
        call_writetemps(nullptr);
        return;
    }
    detail->tww += detail->ltt();
    if (bl->file_progress> detail->maxp) {
        detail->maxp = bl->file_progress;
    }
    
    if (temps->empty()) {
        temps->resize(max_tile()+1);
    }
    for (auto o: bl->objects) {
        size_t k = find_tile(o);
        if (!temps->at(k)) {
            auto x = std::make_shared<primitiveblock>(k,0);
            x->quadtree=k;
            temps->at(k) = x; 
        }
        temps->at(k)->add(o);
        
        detail->currww += 1;
        if ( (o->Type() == elementtype::Way) || (o->Type() == elementtype::Linestring) || (o->Type() == elementtype::SimplePolygon)) {
            detail->currww += 5;
        } else if ( (o->Type() == elementtype::Relation) || (o->Type() == elementtype::ComplicatedPolygon)) {
            detail->currww += 10;
        }
       
    }
    
    detail->ntile++;
    //if ((ntile % blocksplit)==0) {
    if (detail->currww > detail->writeat) {
        call_writetemps(temps);
        temps = std::make_shared<tempsvec>();
        detail->currww = 0;
    }
    
    detail->tmm += detail->ltt();
}

        
}
