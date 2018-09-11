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


#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/calcqts/qttreegroups.hpp"
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>

#include "oqt/elements/block.hpp"
#include "oqt/utils/geometry.hpp"

#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/threadedcallback.hpp"


#include "oqt/elements/quadtree.hpp"
#include "oqt/pbfformat/readminimal.hpp"


#include "oqt/sorting/sortblocks.hpp"
#include "oqt/sorting/mergechanges.hpp"
#include "oqt/sorting/final.hpp"
#include "oqt/sorting/sortgroup.hpp"
#include "oqt/sorting/resortobjects.hpp"
#include "oqt/pbfformat/readfile.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/sorting/tempobjs.hpp"
namespace oqt {
class CollectQts { 
    public:
        CollectQts(std::function<void(std::shared_ptr<count_map>)> cb_) : cb(cb_), lim(1000000) {}
        
        void call(std::shared_ptr<qtvec> cc) {
            if (!cc) {
                if (curr) {
                    cb(curr);
                }
                logger_message() << "CollectQts finished";
                cb(nullptr);
                return;
            }                
                
            if (!curr) {
                curr = std::make_shared<count_map>();
            }
            for (const auto& p: *cc) {
                (*curr)[p.second] += 1;
            }
            if (curr->size()>lim) {
                cb(curr);
                curr.reset();
            }
        }
        static std::function<void(std::shared_ptr<qtvec>)> make(std::function<void(std::shared_ptr<count_map>)> cb) {
            auto cq = std::make_shared<CollectQts>(cb);
            return [cq](std::shared_ptr<qtvec> v) {
                cq->call(v);
            };
        }
        
    private:
        std::function<void(std::shared_ptr<count_map>)> cb;
        size_t lim;
        std::shared_ptr<count_map> curr;
};



std::shared_ptr<qttree> make_qts_tree_maxlevel(const std::string& qtsfn, size_t numchan, size_t maxlevel) {
    
    auto tree = make_tree_empty();
    
    auto add_qts = threaded_callback<count_map>::make(make_addcountmaptree(tree, maxlevel),numchan);
    
    std::vector<std::function<void(std::shared_ptr<qtvec>)>> make_addcounts;
    for (size_t i=0; i < numchan; i++) {
        make_addcounts.push_back(CollectQts::make(add_qts));
    }
    
    read_blocks_split_qtvec(qtsfn, make_addcounts, {}, 15);
    
    return tree;
};
           

std::shared_ptr<qttree> make_qts_tree(const std::string& qtsfn, size_t numchan) {
    return make_qts_tree_maxlevel(qtsfn, numchan, 17);
}

    


class AddQuadtrees {
    public:
        
        AddQuadtrees(std::function<std::shared_ptr<qtvec>(void)> next_qts_block_, bool msgs_) :
            next_qts_block(next_qts_block_), msgs(msgs_), curr_idx(0), count(0) {
                curr = next_qts_block();
                if (!curr) {
                    throw std::domain_error("no qts");
                }
                while (curr->empty()) {
                    logger_message() << "skip empty";
                    curr=next_qts_block();
                }
            }
            
        
        size_t call(primitiveblock_ptr bl) {
            
            if (msgs && ((count % 1000)==0)) {
                
                logger_progress pp(bl->file_progress);
                pp << "Block " << bl->index << " " << bl->size()
                   << " [" << std::fixed << std::setprecision(1) << ts.since() << "s]";
                if (bl->size()>0) {
                    auto f = bl->at(0);
                    pp << " " << f->Type() << " " << std::setw(10) << f->Id();
                }
                if (bl->size()>1) {
                    auto l = bl->at(bl->size()-1);
                    pp << " to " << l->Type() << " " << std::setw(10) << l->Id();
                }
                
            }
            
            for (size_t i=0; i < bl->size(); i++) {
                
                if (curr_idx == curr->size()) { 
                    curr = next_qts_block();
                    curr_idx=0;
                    if (!curr) {
                        throw std::domain_error("out of qts");
                    }
                }
                auto q = curr->at(curr_idx);//minimalblock_at(curr, curr_idx); //
                
                auto obj = bl->at(i);
                if (q.first != obj->InternalId()) {
                    auto err = "out of sync "+std::to_string(q.first)+" "+std::to_string(q.second)+" // "+std::to_string(obj->InternalId());
                    throw std::domain_error(err);
                    
                } else {
                    //bl->set_qt(i, q.second);
                    obj->SetQuadtree(q.second);
                }
                curr_idx++;
            }
            return count++;
        }
        
        
        
    private:
        
        std::function<std::shared_ptr<qtvec>(void)> next_qts_block;
        bool msgs;
        std::shared_ptr<qtvec> curr;
        size_t curr_idx;
        size_t count;
        
        time_single ts;
};          

class ReadQtsVec {
    public:
        ReadQtsVec(const std::string& qtsfn) {
            
            auto fs = file_size(qtsfn);
            readfile = make_readfile(qtsfn,{},0, 128*1024*1024, fs);
        }
        
        std::shared_ptr<qtvec> next() {
            auto bl = readfile->next();
            if (!bl) { return nullptr; }
            if (bl->blocktype != "OSMData") { return std::make_shared<qtvec>(); }
            return readQtVecBlock(bl->get_data(), 7);
        }
        
        static std::function<std::shared_ptr<qtvec>()> make(const std::string& qtsfn) {
            auto readqtsvec = std::make_shared<ReadQtsVec>(qtsfn);
    
            return [readqtsvec]() -> std::shared_ptr<qtvec> {
                return readqtsvec->next();
            };
        }
    
    private:
        
        std::shared_ptr<ReadFile> readfile;
};


primitiveblock_callback add_quadtreesup_callback(std::vector<primitiveblock_callback> callbacks, const std::string& qtsfn) {
    
    
    auto nqb = ReadQtsVec::make(qtsfn);
    
    //auto nqb = ReadBlocksNoThread<qtvec>::make(qtsfn,{}, nullptr,false,7);
    //auto nqb = inverted_callback<qtvec>::make([&qtsfn](std::function<void(std::shared_ptr<qtvec>)> qtc) { read_blocks<qtvec>(qtsfn,qtc,{},4,nullptr,false,7,false); });
    
    auto aq = std::make_shared<AddQuadtrees>(nqb,false);
    return [aq,callbacks](primitiveblock_ptr bl) {
        if (!bl) {
            logger_message() << "finished add_quadtrees";
            for (auto cb: callbacks) {
                cb(nullptr);
            }
            return;
        }
        size_t c = aq->call(bl);
        callbacks[c%callbacks.size()](bl);
    };
}
   



int run_sortblocks_inmem(const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<qttree> groups) {
        
    auto lg=get_logger();
    
    auto hh = std::make_shared<header>();
    hh->box=bbox{-1800000000,-900000000,1800000000,900000000};
    auto write_file_obj = make_pbffilewriter_indexedinmem(outfn, hh);
    
    
    auto packers2 = make_final_packers(write_file_obj, numchan, timestamp, true,true);
    
    
    //auto resort/*_int*/ = make_resortobjects_callback(packers2, groups, groups->size());
    std::vector<primitiveblock_ptr> outs;
    outs.resize(groups->size());
    
    auto resort = [&outs,groups,timestamp](primitiveblock_ptr bl) {
        if (!bl) { return; }
        for (auto o: bl->objects) {
            auto tl = groups->find_tile(o->Quadtree());
            if (!outs.at(tl.idx)) {
                auto nbl = std::make_shared<primitiveblock>(tl.idx, tl.weight);
                nbl->enddate=timestamp;
                nbl->quadtree=tl.qt;
                outs.at(tl.idx) = nbl;
            }
            outs.at(tl.idx)->add(o);
        }
    };
    
    if (qtsfn == "NONE") {
        read_blocks_primitiveblock(origfn, resort, {}, numchan, nullptr, false, 7);
    } else {
        
        auto add_quadtrees = add_quadtreesup_callback({resort}, qtsfn);
        read_blocks_primitiveblock(origfn, add_quadtrees, {}, numchan, nullptr, false,7);
    }
    logger_message() << "finished";
    
    lg->time("sorted data");
    
    size_t i=0;
    for (auto bl: outs) {
        if (bl) {
            packers2[i%numchan](bl);
            i++;

        } 
    }
    for (auto p: packers2) { p(nullptr); }
    
    write_file_obj->finish();
    lg->time("written to file");
    return 0;
}
    
    
    



std::shared_ptr<primitiveblock> convert_primblock(std::shared_ptr<FileBlock> bl) {
    if (!bl) { return primitiveblock_ptr(); }
    if ((bl->blocktype=="OSMData")) {
        std::string dd = bl->get_data();//decompress(std::get<2>(*bl), std::get<3>(*bl))
        auto pb = readPrimitiveBlock(bl->idx, dd, false,15,nullptr,nullptr);
        pb->file_position = bl->file_position;
        pb->file_progress= bl->file_progress;
        return pb;
    }
    return std::make_shared<primitiveblock>(-1);
}




class SortBlocksImpl : public SortBlocks {
    public:
        SortBlocksImpl(int64 orig_file_size_, std::shared_ptr<qttree> groups_, const std::string& tempfn_,
            size_t blocksplit_, size_t numchan_)
        : orig_file_size(orig_file_size_), groups(groups_), tempfn(tempfn_),  blocksplit(blocksplit_), numchan(numchan_) {
            
            if (!groups) {
                throw std::domain_error("no groups");
            }
            
            num_splits = orig_file_size /1024 + 1;
            group_split = groups->size() / num_splits;
            if (blocksplit==0) {
                blocksplit = groups->size() * 30 / orig_file_size;
            }
            logger_message() << "orig_file_size=" << orig_file_size << "mb; blocksplit=" << blocksplit << "num_splits=" << num_splits << ", groups->size() = " << groups->size() << ", group_split=" << group_split;
            
            auto blobs = (num_splits>2) ? make_blobstore_filesplit(tempfn, group_split/blocksplit) : make_blobstore_file(tempfn, true);
            tempobjs = make_tempobjs(blobs, numchan);
        
        
            
        }
        virtual ~SortBlocksImpl() {}
        
        virtual std::vector<primitiveblock_callback> make_addblocks_cb(bool threaded) {
            
            std::vector<primitiveblock_callback> sgg;
            for (size_t i=0; i < numchan; i++) {
                auto cb = make_sortgroup_callback(tempobjs->add_func(i),groups,blocksplit, 1000000);
                if (threaded) {
                    cb = threaded_callback<primitiveblock>::make(cb);
                }
                sgg.push_back(cb);
            }
            return sgg;
        }           
        virtual void finish() {
            tempobjs->finish();
        }
        
                
        virtual void read_blocks(std::vector<primitiveblock_callback> packers, bool sortobjs) {
            std::vector<primitiveblock_callback> rss;
            for (auto p: packers) {
                rss.push_back(make_resortobjects_callback(p,groups, blocksplit, sortobjs));
            }
            tempobjs->read(rss);
        }
        
          

    private:
        int64 orig_file_size;
        std::shared_ptr<qttree> groups;
        std::string tempfn;
        size_t blocksplit;
        size_t numchan;
        
        int64 num_splits;
        size_t group_split;
        std::shared_ptr<TempObjs> tempobjs;
};
std::shared_ptr<SortBlocks> make_sortblocks(int64 orig_file_size, std::shared_ptr<qttree> groups,
    const std::string& tempfn, size_t blocksplit, size_t numchan) {
    return std::make_shared<SortBlocksImpl>(orig_file_size,groups,tempfn,blocksplit,numchan  );
}


int run_sortblocks(const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<qttree> groups,
    const std::string& tempfn, size_t blocksplit) {
    
    auto lg=get_logger();
    if (tempfn=="NONE") {
        return run_sortblocks_inmem(origfn, qtsfn, outfn, timestamp, numchan, groups);
    }
    
    int64 orig_file_size = file_size(origfn) / 1024/1024;
    auto sb = make_sortblocks(orig_file_size, groups, tempfn, blocksplit,numchan);
    
    auto sgg = sb->make_addblocks_cb(qtsfn!="NONE");
    if (qtsfn == "NONE") {
        read_blocks_split_convfunc_primitiveblock(origfn, sgg, {}, convert_primblock);
    } else { 
        auto add_quadtrees = add_quadtreesup_callback(sgg, qtsfn);
        read_blocks_convfunc_primitiveblock(origfn, add_quadtrees, {}, numchan, convert_primblock);
    }
    
    sb->finish();
    
    
    lg->time("read data");
    
    auto hh = std::make_shared<header>();
    hh->box=bbox{-1800000000,-900000000,1800000000,900000000};
    auto write_file_obj = make_pbffilewriter(outfn, hh, true);
    
    
            
    auto packers = make_final_packers(write_file_obj, numchan, timestamp, true,false);
    sb->read_blocks(packers, false);
    
    lg->time("resort objs");
    block_index finalidx = write_file_obj->finish();
    logger_message() << "final: have " << finalidx.size() << " blocks";
    lg->time("rewrote file");
    return 0;
}

}


