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
#include "oqt/sorting/qttreegroups.hpp"
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>

#include "oqt/elements/block.hpp"
#include "oqt/utils/geometry.hpp"

#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/splitcallback.hpp"

#include "oqt/utils/logger.hpp"


#include "oqt/elements/quadtree.hpp"
#include "oqt/elements/relation.hpp"

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
        
        void call(std::shared_ptr<quadtree_vector> cc) {
            if (!cc) {
                if (curr) {
                    cb(curr);
                }
                Logger::Message() << "CollectQts finished";
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
        static std::function<void(std::shared_ptr<quadtree_vector>)> make(std::function<void(std::shared_ptr<count_map>)> cb) {
            auto cq = std::make_shared<CollectQts>(cb);
            return [cq](std::shared_ptr<quadtree_vector> v) {
                cq->call(v);
            };
        }
        
    private:
        std::function<void(std::shared_ptr<count_map>)> cb;
        size_t lim;
        std::shared_ptr<count_map> curr;
};



std::shared_ptr<QtTree> make_qts_tree_maxlevel(const std::string& qtsfn, size_t numchan, size_t maxlevel) {
    
    auto tree = make_tree_empty();
    
    if (numchan==0) {
        auto add_qts=make_addcountmaptree(tree,maxlevel);
        auto addcount = CollectQts::make(add_qts);
        read_blocks_nothread_quadtree_vector(qtsfn, addcount, {}, ReadBlockFlags::Empty);
    } else {
        
    
        auto add_qts = multi_threaded_callback<count_map>::make(make_addcountmaptree(tree, maxlevel),numchan);
        
        std::vector<std::function<void(std::shared_ptr<quadtree_vector>)>> make_addcounts;
        for (size_t i=0; i < numchan; i++) {
            make_addcounts.push_back(CollectQts::make(add_qts[i]));
        }
        
        read_blocks_split_quadtree_vector(qtsfn, make_addcounts, {}, ReadBlockFlags::Empty);
    }
    
    return tree;
};
           

std::shared_ptr<QtTree> make_qts_tree(const std::string& qtsfn, size_t numchan) {
    return make_qts_tree_maxlevel(qtsfn, numchan, 17);
}

    


class AddQuadtrees {
    public:
        
        AddQuadtrees(std::function<std::shared_ptr<quadtree_vector>(void)> next_qts_block_, bool msgs_) :
            next_qts_block(next_qts_block_), msgs(msgs_), curr_idx(0), count(0) {
                curr = next_qts_block();
                if (!curr) {
                    throw std::domain_error("no qts");
                }
                while (curr->empty()) {
                    Logger::Message() << "skip empty";
                    curr=next_qts_block();
                }
            }
            
        
        size_t call(PrimitiveBlockPtr bl) {
            
            if (msgs && ((count % 1000)==0)) {
                
                Logger::Progress pp(bl->FilePosition());
                pp << "Block " << bl->Index() << " " << bl->size()
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
            //std::cout << "bl with " << bl->size() << " objs" << std::endl;
            
            for (size_t i=0; i < bl->size(); i++) {
                
                if (curr_idx == curr->size()) { 
                    curr = next_qts_block();
                    curr_idx=0;
                    if (!curr) {
                        throw std::domain_error("out of qts");
                    }
                }
                auto q = curr->at(curr_idx);//minimalblock_at(curr, curr_idx); //
                //std::cout << q.first << " // " << q.second << std::endl;
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
        
        std::function<std::shared_ptr<quadtree_vector>(void)> next_qts_block;
        bool msgs;
        std::shared_ptr<quadtree_vector> curr;
        size_t curr_idx;
        size_t count;
        
        TimeSingle ts;
};          

class ReadQtsVec {
    public:
        ReadQtsVec(const std::string& qtsfn) {
            
            auto fs = file_size(qtsfn);
            readfile = make_readfile(qtsfn,{},0, 128*1024*1024, fs);
        }
        
        std::shared_ptr<quadtree_vector> next() {
            auto bl = readfile->next();
            if (!bl) { return nullptr; }
            if (bl->blocktype != "OSMData") { return std::make_shared<quadtree_vector>(); }
            return read_quadtree_vector_block(bl->get_data(), ReadBlockFlags::Empty);
        }
        
        static std::function<std::shared_ptr<quadtree_vector>()> make(const std::string& qtsfn) {
            auto readqtsvec = std::make_shared<ReadQtsVec>(qtsfn);
    
            return [readqtsvec]() -> std::shared_ptr<quadtree_vector> {
                return readqtsvec->next();
            };
        }
    
    private:
        
        std::shared_ptr<ReadFile> readfile;
};


primitiveblock_callback add_quadtreesup_callback(std::vector<primitiveblock_callback> callbacks, const std::string& qtsfn) {
    
    
    auto nqb = ReadQtsVec::make(qtsfn);
    
    auto aq = std::make_shared<AddQuadtrees>(nqb,false);
    return [aq,callbacks](PrimitiveBlockPtr bl) {
        if (!bl) {
            Logger::Message() << "finished add_quadtrees";
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
    int64 timestamp, size_t numchan, std::shared_ptr<QtTree> groups, bool fix_strs) {
        
    
    
    auto hh = std::make_shared<Header>();
    hh->SetBBox(bbox{-1800000000,-900000000,1800000000,900000000});
    auto write_file_obj = make_pbffilewriter_indexedinmem(outfn, hh);
    
    
    auto packers2 = make_final_packers(write_file_obj, numchan, timestamp, true,true);
    
    std::cout << "groups->size()=" << groups->size() << std::endl;
    /*for (size_t i=0; i < groups->size(); i++) {
        auto tl = groups->at(i);
        std::cout << "groups->at(" << i << ")=" << tl.idx << " " << tl.qt << " " << tl.weight << " " << tl.total  << std::endl;
    }*/
    
    //auto resort/*_int*/ = make_resortobjects_callback(packers2, groups, groups->size());
    std::vector<PrimitiveBlockPtr> outs;
    outs.resize(groups->size());
    
    auto resort = [&outs,groups,timestamp,fix_strs](PrimitiveBlockPtr bl) {
        if (!bl) { return; }
        if (bl->size()==0) { return; }
        //std::cout << "block " << bl->Index() << " " << bl->size() << " objs" << std::endl;
        for (auto o: bl->Objects()) {
            std::cout << o->Type() << " " << o->Id() << " " << oqt::quadtree::string(o->Quadtree()) << " to tile " << std::flush;
            auto tl = groups->find_tile(o->Quadtree());
            //std::cout << tl.idx << " " << oqt::quadtree::string(tl.qt) << std::endl;
            if (!outs.at(tl.idx-1)) {
                auto nbl = std::make_shared<PrimitiveBlock>(tl.idx, tl.weight);
                nbl->SetEndDate(timestamp);
                nbl->SetQuadtree(tl.qt);
                outs.at(tl.idx-1) = nbl;
            }
            
            if (fix_strs) {
                fix_tags(*o);
                if (o->Type()==ElementType::Relation) {
                    auto rel = std::dynamic_pointer_cast<Relation>(o);
                    fix_members(*rel);
                }
            }
            
            outs.at(tl.idx-1)->add(o);
        }
    };
    
    if (qtsfn == "NONE") {
        read_blocks_primitiveblock(origfn, resort, {}, numchan, nullptr, false, ReadBlockFlags::Empty);
    } else {
        
        auto add_quadtrees = add_quadtreesup_callback({resort}, qtsfn);
        read_blocks_primitiveblock(origfn, add_quadtrees, {}, numchan, nullptr, false,ReadBlockFlags::Empty);
    }
    Logger::Message() << "finished";
    
    Logger::Get().time("sorted data");
    
    size_t i=0;
    for (auto bl: outs) {
        if (bl) {
            packers2[i%numchan](bl);
            i++;

        } 
    }
    for (auto p: packers2) { p(nullptr); }
    
    write_file_obj->finish();
    Logger::Get().time("written to file");
    return 0;
}
    
    
    



PrimitiveBlockPtr convert_primblock(std::shared_ptr<FileBlock> bl) {
    if (!bl) { return PrimitiveBlockPtr(); }
    if ((bl->blocktype=="OSMData")) {
        std::string dd = bl->get_data();//decompress(std::get<2>(*bl), std::get<3>(*bl))
        auto pb = read_primitive_block(bl->idx, dd, false,ReadBlockFlags::Empty,nullptr,nullptr);
        pb->SetFilePosition(bl->file_position);
        pb->SetFileProgress(bl->file_progress);
        return pb;
    }
    return std::make_shared<PrimitiveBlock>(-1);
}




class SortBlocksImpl : public SortBlocks {
    public:
        SortBlocksImpl(int64 orig_file_size_, std::shared_ptr<QtTree> groups_, const std::string& tempfn_,
            size_t blocksplit_, size_t numchan_, int64 timestamp_)
        : orig_file_size(orig_file_size_), groups(groups_), tempfn(tempfn_),  blocksplit(blocksplit_), numchan(numchan_), timestamp(timestamp_) {
            
            if (!groups) {
                throw std::domain_error("no groups");
            }
            
            num_splits = orig_file_size /1024 + 1;
            group_split = groups->size() / num_splits;
            if (blocksplit==0) {
                blocksplit = groups->size() * 15 / orig_file_size;
            }
            Logger::Message() << "orig_file_size=" << orig_file_size << "mb; blocksplit=" << blocksplit << "num_splits=" << num_splits << ", groups->size() = " << groups->size() << ", group_split=" << group_split;
            
            blobs = (num_splits>2) ? make_blobstore_filesplit(tempfn, group_split/blocksplit) : make_blobstore_file(tempfn, false);
            tempobjs = make_tempobjs(blobs, numchan);
        
        
            
        }
        virtual ~SortBlocksImpl() {}
        
        virtual std::vector<primitiveblock_callback> make_addblocks_cb(bool threaded) {
            
            std::vector<primitiveblock_callback> sgg;
            for (size_t i=0; i < numchan; i++) {
                auto cb = make_sortgroup_callback(tempobjs->add_func(i),groups,blocksplit, 1000000);
                if (threaded) {
                    cb = threaded_callback<PrimitiveBlock>::make(cb);
                }
                sgg.push_back(cb);
            }
            return sgg;
        }           
        virtual void finish() {
            tempobjs->finish();
        }
    
    
        virtual void read_blocks_packed(write_file_callback cb) {
            auto resort = make_resortobjects_callback_alt(groups, true, timestamp, -1, cb, numchan);
            blobs->read(resort);
        }
                
        virtual void read_blocks(primitiveblock_callback cb, bool sortobjs) {
            auto resort = make_resort_objects_collect_block(groups, sortobjs, timestamp, cb, numchan);
            blobs->read(resort);
        }
            
                      

    private:
        int64 orig_file_size;
        std::shared_ptr<QtTree> groups;
        std::string tempfn;
        size_t blocksplit;
        size_t numchan;
        
        int64 num_splits;
        size_t group_split;
        std::shared_ptr<BlobStore> blobs;
        std::shared_ptr<TempObjs> tempobjs;
        int64 timestamp;
};
std::shared_ptr<SortBlocks> make_sortblocks(int64 orig_file_size, std::shared_ptr<QtTree> groups,
    const std::string& tempfn, size_t blocksplit, size_t numchan, int64 timestamp) {
    return std::make_shared<SortBlocksImpl>(orig_file_size,groups,tempfn,blocksplit,numchan, timestamp  );
}


int run_sortblocks(const std::string& origfn, const std::string& qtsfn, const std::string& outfn,
    int64 timestamp, size_t numchan, std::shared_ptr<QtTree> groups,
    const std::string& tempfn, size_t blocksplit, bool fixstrs, bool seperate_filelocs) {
    
   
    if (tempfn=="NONE") {
        return run_sortblocks_inmem(origfn, qtsfn, outfn, timestamp, numchan, groups, fixstrs);
    }
    
    int64 orig_file_size = file_size(origfn) / 1024/1024;
    auto sb = make_sortblocks(orig_file_size, groups, tempfn, blocksplit,numchan, timestamp);
    
    
    std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> convert;
    
    std::atomic<size_t> numfixed;
    numfixed=0;
    
    if (fixstrs) {
        convert = [&numfixed](std::shared_ptr<FileBlock> fb) {
            auto bl = convert_primblock(fb);
            size_t nf=0;
            if (bl) {
                for (auto ele: bl->Objects()) {
                    if (fix_tags(*ele)) {
                        nf++;
                    }
                    if (ele->Type()==ElementType::Relation) {
                        auto rel = std::dynamic_pointer_cast<Relation>(ele);
                        if (fix_members(*rel)) {
                            nf++;
                        }
                    }
                }
            }
            if (nf>0) {
                numfixed+=nf;
            }
            return bl;
        };
    } else {
        convert = convert_primblock;
    }
            
    
    auto sgg = sb->make_addblocks_cb(qtsfn!="NONE");
    if (qtsfn == "NONE") {
        read_blocks_split_convfunc_primitiveblock(origfn, sgg, {}, convert);
    } else { 
        auto add_quadtrees = add_quadtreesup_callback(sgg, qtsfn);
        read_blocks_convfunc_primitiveblock(origfn, add_quadtrees, {}, numchan, convert);
    }
    
    sb->finish();
    if (fixstrs) {
        Logger::Message() << "fixed " << numfixed << " obj strings";
    }
    
    Logger::Get().time("read data");
    
    auto hh = std::make_shared<Header>();
    hh->SetBBox(bbox{-1800000000,-900000000,1800000000,900000000});
    auto write_file_obj = seperate_filelocs ? make_pbffilewriter_filelocs(outfn, hh) : make_pbffilewriter_indexed(outfn, hh);
    
    auto cb = [write_file_obj](keystring_ptr k) {
        if (k) {
            write_file_obj->writeBlock(k->first,k->second);
        }
    };    
   
    sb->read_blocks_packed(cb);
    
    Logger::Get().time("resort objs");
    block_index finalidx = write_file_obj->finish();
    Logger::Message() << "final: have " << finalidx.size() << " blocks";
    Logger::Get().time("rewrote file");
    return 0;
}

}


