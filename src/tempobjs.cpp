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

#include "oqt/readpbffile.hpp"
#include "oqt/writepbffile.hpp"
#include "oqt/writefile.hpp"
#include "oqt/writeblock.hpp"
#include "oqt/simplepbf.hpp"
#include "oqt/utils.hpp"
#include "oqt/picojson.h"
#include "oqt/sortfile.hpp"
#include "oqt/sorting/tempobjs.hpp"
#include <iterator>
namespace oqt {

class BlobStoreFile : public BlobStore {
    
    public:
        BlobStoreFile(std::string tempfn_, bool sortfile) : tempfn(tempfn_) {
            fileobj = make_pbffilewriter(tempfn,nullptr,sortfile);
        }
        
        virtual void add(keystring_ptr ks) {
            if (!ks) {
                tempfl_idx = fileobj->finish();
                sort_block_index(tempfl_idx);
                logger_message() << tempfl_idx.size() << " blocks";
                return;
            }
            
            fileobj->writeBlock(ks->first,ks->second);
        }   
        virtual void read(std::vector<keyedblob_callback> outs) {
            src_locs_map mm;
            
            for (auto& l : tempfl_idx) {
                mm[std::get<0>(l)].push_back(std::make_pair(0, std::get<1>(l)));
                
            }
            
                       
            std::vector<std::unique_ptr<std::ifstream>> files;
            files.push_back(std::make_unique<std::ifstream>(tempfn, std::ios::binary | std::ios::in));
            if (!files.back()->good()) {
                throw std::domain_error("can't open "+tempfn);
            }
            read_some_split_locs_parallel_callback(files, outs, mm);
            std::remove(tempfn.c_str());
        }
    private:
        std::string tempfn;
        std::shared_ptr<PbfFileWriter> fileobj;
        block_index tempfl_idx;
};
std::shared_ptr<BlobStore> make_blobstore_file(std::string tempfn, bool sortfile) {
    return std::make_shared<BlobStoreFile>(tempfn,sortfile);
}

class BlobStoreFileSplit : public BlobStore {
    public:
        BlobStoreFileSplit(std::string tempfn_, int64 splitat_) 
            : tempfn(tempfn_), splitat(splitat_) { }
        
        void add(keystring_ptr p) {
            if (!p) {
                for (auto& x: writers) {
                    auto ii=std::get<2>(x.second)->finish();
                    sort_block_index(ii);
                    std::get<1>(x.second)=ii;
                }
                return;
            }                
            auto k = p->first/splitat;
            auto it = writers.find(k);
            
            if (it==writers.end()) {
                auto fn=tempfn+"-pt"+std::to_string(k);
                writers[k] = std::make_tuple(fn,block_index{},make_pbffilewriter(fn,nullptr,false));
                it = writers.find(k);
            }
            std::get<2>(it->second)->writeBlock(p->first,p->second);
        }           
        
                
        virtual void read(std::vector<keyedblob_callback> convs) {
            
            
            size_t ii=0;
            
            std::map<int64,double> kk;
            size_t nblocks=0;
            for (const auto& x: writers) {
                for (const auto& i: std::get<1>(x.second)) {
                    kk.insert(std::make_pair(std::get<0>(i),0));
                    nblocks+=1;
                }
            }
            double prog_factor = 100.0 / kk.size();
            size_t i=0;
            for (auto& p: kk) {
                p.second = prog_factor*i;
                i++;
            }
            logger_message() << "reading " << nblocks << " blocks with " << kk.size() << " keys";
            
            for (size_t i=0; i < convs.size(); i++) {
                auto c = convs[i];
                convs[i] = [prog_factor, c, &kk](std::shared_ptr<keyedblob> kb) {
                    if (kb) {
                        try {
                            kb->file_progress = kk.at(kb->key);
                        } catch (std::exception& ex) {
                            logger_message() << "?? kb->key=" << kb->key << ", kk.count = " << kk.count(kb->key) << " {" << ex.what() << "}";
                        }
                        
                    }
                    c(kb);
                };
            }
            
            
            
            
            for (auto&x : writers) {
                
                auto fn = std::get<0>(x.second);
                std::vector<std::unique_ptr<std::ifstream>> files;
                files.push_back(std::make_unique<std::ifstream>(fn, std::ios::binary | std::ios::in));
                if (!files.back()->good()) { throw std::domain_error("couldn't open "+fn); }
                const auto& bi = std::get<1>(x.second);
                
                src_locs_map ll;
                std::map<int64,size_t> tot;
                for (const auto& l: bi) {
                    ll[std::get<0>(l)].push_back(std::make_pair(0,std::get<1>(l)));
                    tot[std::get<0>(l)] += std::get<2>(l);
                }
                
                auto it = ll.begin();
                while (it != ll.end()) {
                    size_t t = 0;
                    src_locs_map ll2;
                    while ( (it != ll.end()) && (t < (100ll*1024*1024))) {
                        ll2[it->first] = it->second;
                        t += tot.at(it->first);
                        it++;
                    }
                    ii = read_some_split_buffered_keyed_callback(files, convs, ii, ll2, false);
                }
               
                std::remove(fn.c_str());
            }
            for (auto& cc: convs) { cc(nullptr); }
            
            
                
            
        }
    private:
        std::string tempfn;
        
        int64 splitat;
        size_t idx;
        
        std::map<int64,std::tuple<std::string,block_index,std::shared_ptr<PbfFileWriter>>> writers;
        
};          
std::shared_ptr<BlobStore> make_blobstore_filesplit(std::string tempfn, int64 splitat) {
    return std::make_shared<BlobStoreFileSplit>(tempfn,splitat);
}




std::function<void(std::shared_ptr<keyedblob>)> make_conv_keyedblob_primblock(primitiveblock_callback oo) {
    return [oo](std::shared_ptr<keyedblob> kk) {
        if (!kk) { return oo(nullptr); }
        
        primitiveblock_ptr res = std::make_shared<primitiveblock>(kk->key);
        res->quadtree = kk->key;
        for (const auto& x: kk->blobs) {
            auto dd = decompress(x.first,x.second);
            auto bl = readPrimitiveBlock(kk->key,dd,false,15,nullptr,nullptr);
            
            
            
            
            for (auto o: bl->objects) {
                res->add(o);
            }
        }
        res->file_progress = kk->file_progress;
        oo(res);
    };
}

class TempObjsBlobstore: public TempObjs {
    public:
        TempObjsBlobstore(std::shared_ptr<BlobStore> blobstore_, size_t numchan) : blobstore(blobstore_) {
            auto bsc = [this](keystring_ptr p) { blobstore->add(p); };
            packers = make_final_packers_cb(bsc, numchan, 0, true, false); 
        }
        
        virtual void finish() {
            for (auto p : packers) { p(nullptr); }
        }
        virtual primitiveblock_callback add_func(size_t i) {
            return packers.at(i);
        }
        virtual void read(std::vector<primitiveblock_callback> outs) {
                        
            
            std::vector<std::function<void(std::shared_ptr<keyedblob> kk)>> convs;
            for (auto oo:  outs) {
                convs.push_back(threaded_callback<keyedblob>::make(make_conv_keyedblob_primblock(oo)));
            }
            blobstore->read(convs);
        }
    private:
        
        std::shared_ptr<BlobStore> blobstore;
        std::vector<primitiveblock_callback> packers;
        
};   




std::shared_ptr<TempObjs> make_tempobjs(std::shared_ptr<BlobStore> blobstore, size_t numchan) {
    return std::make_shared<TempObjsBlobstore>(blobstore, numchan);
};
}
