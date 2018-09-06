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

#include "oqt/sortfile.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/quadtreegroups.hpp"
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <thread>


#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/readpbffile.hpp"
#include "oqt/writepbffile.hpp"


namespace oqt {
/*
packedblock_ptr read_tempobjs(std::shared_ptr<FileBlock> fb) {
    if (!fb) { return nullptr; }
    if (fb->blocktype=="TempObjs") {
        std::string dd = fb->get_data();
        size_t pos=0;
        PbfTag tg = readPbfTag(dd, pos);
        
        auto out = make_outpb_ptr(0,8000,-1);
        
        for ( ; tg.tag>0; tg = readPbfTag(dd, pos)) {
            if (tg.tag==1) {
                //std::get<0>(*out) = tg.value;
                out->index=tg.value;
            } else if (tg.tag==3) {
                //std::get<2>(*out).push_back(read_packedobj(tg.data));
                out->add(read_packedobj(tg.data));
            }
        }
        
        return out;
    }
    return std::make_shared<packedblock>(0,-1);
}
packedblock_ptr make_outpb_ptr(size_t idx, size_t sz, int64 qt) {
    auto r = std::make_shared<packedblock>(idx,sz);
    r->quadtree=qt;
    return r;
}


keystring_ptr pack_temps(packedblock_ptr pb) {

    size_t ln=20;
    for (const auto& s : pb->objects) {
        ln+=(10+s->approx_size());
    }

    std::string out(ln,0);
    size_t op=0;

    op=writePbfValue(out,op,1,pb->index);
    if (pb->quadtree>=0) {
        op=writePbfValue(out,op,2,zigZag(pb->quadtree));
    }
    for (const auto& s: pb->objects) {
        op=writePbfData(out,op,3,s->pack());
    }
    out.resize(op);

    return std::make_shared<keystring>(pb->index,prepareFileBlock("TempObjs",out, 1));
}

void read_tempobjs_file(packedblock_callback cb, std::string filename, std::vector<int64> locs, size_t numchan) {
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (!file.good()) {
        throw std::domain_error("can't open"+filename);
    }

    auto cbs = multi_threaded_callback<packedblock>::make(cb, numchan);

    std::vector<std::function<void(std::shared_ptr<FileBlock> fb)>> convblocks;
    for (size_t i=0; i < numchan; i++) {
        auto collect=cbs[i];
        convblocks.push_back(threaded_callback<FileBlock>::make(
            [collect](std::shared_ptr<FileBlock> fb) {
                
                if (fb) {
                    collect(read_tempobjs(fb));
                } else {
                    collect(nullptr);
                }
                
            }
        ));
    }
    
    read_some_split_locs_callback(file, convblocks, 0, locs, 0);
    
    
}
*/
            
class CollectObjs {
    public:
        CollectObjs(std::vector<primitiveblock_callback> cb_, size_t blocksize_, bool checkorder_) : cb(cb_), idx(0), blocksize(blocksize_), checkorder(checkorder_), prev(0) {
            
            curr = std::make_shared<primitiveblock>(idx,blocksize);
            
        }
        
        void add(element_ptr obj) {
            //if (std::get<2>(*curr).size()==blocksize) {
            if (curr->size()==blocksize) {
                cb[idx%cb.size()](curr);
                idx++;
                curr = std::make_shared<primitiveblock>(idx,blocksize);
                
                
            }
            
            //std::get<2>(*curr).push_back(std::move(obj));
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
            
            //if (std::get<2>(*curr).size()>0) {
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
class SplitBlocks {
    typedef std::vector<primitiveblock_ptr> tempsvec;
    
    
    public:
        SplitBlocks(primitiveblock_callback callback_, size_t blocksplit_, size_t writeat_, bool msgs_)
            : callback(callback_), blocksplit(blocksplit_), writeat(writeat_), msgs(msgs_), ii(0), ntile(0),tb(0),currww(0) {
                tmm=0;
                tww=0;
                trr=0;
                last=ts.since();
                temps = std::make_shared<tempsvec>();
                
                call_writetemps = threaded_callback<tempsvec>::make([this](std::shared_ptr<tempsvec> t) { writetemps(t); });
                maxp=0;
        }
        
        virtual ~SplitBlocks() {
            
        }
        
        virtual size_t find_tile(element_ptr obj)=0;
        virtual size_t max_tile()=0;
        
        
        double ltt() {
            double m = ts.since();
            double n = m-last;
            last=m;
            return n;
        }
        
        void call(primitiveblock_ptr bl) {
            if (!bl) {
                call_writetemps(temps);
                logger_progress(100) << " finished SplitBlocks " << TmStr{ts.since(),6,1} << " [" << ii << " blocks, " << tb << " objects ], tmm=" << TmStr{tmm,6,1} << ", trr=" << TmStr{trr,8,1} << ", tww=" << TmStr{tww,8,1} << " {" << std::this_thread::get_id() << "}";
                //callback(nullptr);
                call_writetemps(nullptr);
                return;
            }
            tww += ltt();
            if (bl->file_progress> maxp) {
                maxp = bl->file_progress;
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
                
                currww += 1;
                if ( (o->Type() == elementtype::Way) || (o->Type() == elementtype::Linestring) || (o->Type() == elementtype::SimplePolygon)) {
                    currww += 5;
                } else if ( (o->Type() == elementtype::Relation) || (o->Type() == elementtype::ComplicatedPolygon)) {
                    currww += 10;
                }
               
            }
            
            ntile++;
            //if ((ntile % blocksplit)==0) {
            if (currww > writeat) {
                call_writetemps(temps);
                temps = std::make_shared<tempsvec>();
                currww = 0;
            }
            
            tmm += ltt();
        }
        
        
    private:
        primitiveblock_callback callback;
        
        
        size_t blocksplit;
        size_t writeat;
        bool msgs;
        std::shared_ptr<tempsvec> temps;
        
        size_t ii;
        size_t ntile;
        time_single ts;
        size_t tb;
        size_t currww;
        double maxp;
        void writetemps(std::shared_ptr<tempsvec> temps) {
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
        
        std::function<void(std::shared_ptr<tempsvec>)> call_writetemps;
        double tmm,tww,trr,last;
        
        
        
}; 
class SortGroup : public SplitBlocks {
    public:
        SortGroup(primitiveblock_callback callback, 
             std::shared_ptr<qttree> tree_, size_t blocksplit_, size_t writeat) :
             SplitBlocks(callback,blocksplit_,writeat,true),
             tree(tree_), blocksplit(blocksplit_) {
            
            mxt=tree->size()/blocksplit+1;
                
        }
        
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
