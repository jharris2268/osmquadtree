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

#include "oqt/common.hpp"
#include "oqt/pbfformat/readfile.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/utils/logger.hpp"

#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/invertedcallback.hpp"

#include <algorithm>
#include <iostream>

#include <map>
#include <deque>

#include <fstream>

namespace oqt {


void read_some_split_locs_parallel_callback(
    const std::vector<std::string>& filenames,
    std::vector<std::function<void(std::shared_ptr<KeyedBlob>)>> callbacks,
    const src_locs_map& src_locs) {
    
    size_t index=0;
    
    
    std::map<size_t,std::ifstream> files;
    
    
    for (auto& src : src_locs) {
        
        try {
            
            auto res = std::make_shared<KeyedBlob>();
            
            res->idx = index;
            res->key=src.first;
            for (auto& lc : src.second) {
                //auto& infile = *(files.at(lc.first));
                if (!files.count(lc.first)) {
                    files[lc.first] = std::ifstream(filenames.at(lc.first),std::ios::in | std::ios::binary);
                }
                auto& infile = files.at(lc.first);
                if (!infile.good()) {
                    std::cout << "file " << filenames.at(lc.first) << " not open?" << std::endl;
                    throw std::domain_error("can't open "+filenames.at(lc.first));
                }
                infile.seekg(lc.second);
                auto fb=read_file_block(index,infile);
                res->blobs.push_back(std::make_pair(fb->data, fb->uncompressed_size));
            }
            res->file_progress = (100.0*index) / src_locs.size();
            callbacks[index%callbacks.size()](res);
            
        } catch (std::exception& ex) {
            Logger::Message() << "read_some_split_locs_parallel_callback failed at index " << index;
            for (auto& cl : callbacks) {
                cl(std::shared_ptr<KeyedBlob>());
            }
            throw ex;
        }
        index++;
    }
    for (auto& cl : callbacks) {
        cl(std::shared_ptr<KeyedBlob>());
    }
    return;
}

void read_some_split_locs_buffered_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer) {
    std::map<int64,std::shared_ptr<FileBlock>> data;
    std::vector<int64> locs_sorted = locs;
    std::sort(locs_sorted.begin(), locs_sorted.end());
    
    auto cb = [&data](std::shared_ptr<FileBlock> fb) {
        if (fb) {
            data[fb->file_position] = fb;
        }
    };
    
    read_some_split_locs_callback(filename, {cb}, 0, locs_sorted, 0);
    

    for (size_t i=0; i < locs.size(); i++) {
        int64 p = locs[i];
        auto it = data.find(p);
        if (it==data.end()) {
            throw std::domain_error("no data @ "+std::to_string(p));
        }
        callbacks[i%callbacks.size()](it->second);
        data.erase(it);
    }
    if (!data.empty()) {
        throw std::domain_error("??? "+std::to_string(data.size())+" fileblocks left");
    }
    for (auto c: callbacks) {
        c(nullptr);
    }
}


template <int F0, int F1, class X>
bool cmp_tup(const X& l, const X& r) {
    if (std::get<F0>(l)==std::get<F0>(r)) {
        return std::get<F1>(l)<std::get<F1>(r);
    }
    return std::get<F0>(l)<std::get<F0>(r);
}

typedef std::vector<std::shared_ptr<KeyedBlob>> keyedblob_vec;
typedef std::shared_ptr<keyedblob_vec> keyedblob_vec_ptr;
typedef std::function<void(keyedblob_vec_ptr)> keyedblob_vec_callback;

void read_some_split_buffered_keyed_int(
    const std::vector<std::string>& filenames,
    const src_locs_map& locs,
    keyedblob_vec& blobs,
    bool skipfirst) {
        
    using ttt = std::tuple<size_t,int64,std::shared_ptr<FileBlock>>;
    std::vector<ttt> data;
    size_t ns=0;
    std::vector<std::vector<int64>> locs_sorted(filenames.size());
    for (const auto& xx: locs) {
        for (const auto& x: xx.second) {
            
            locs_sorted.at(x.first).push_back(x.second);
            ns++;
        }
    }
    data.reserve(ns);
    
    for (size_t i=(skipfirst ? 1 : 0); i < filenames.size(); i++) {
        if (!locs_sorted.at(i).empty()) {
            auto& ll = locs_sorted.at(i);
            std::sort(ll.begin(), ll.end());
            auto cb = [&data,i](std::shared_ptr<FileBlock> fb) {
                if (fb) {
                    //data[std::make_pair(i, fb->filepos)] = fb;
                    data.push_back(std::make_tuple(i,fb->file_position,fb));
                }
            };
    
            //read_some_split_locs_callbackxx(*files.at(i), {cb}, 0, ll, 0);
            read_some_split_locs_callback(filenames.at(i), {cb}, 0, ll, 0);
        }
    }
    
    size_t i=0;
    for (const auto& xx: locs) {
        auto kk = std::make_shared<KeyedBlob>();
        kk->idx=i;
        kk->key=xx.first;
        
        for (const auto& x: xx.second) {
            if (skipfirst && (x.first==0)) {
                kk->blobs.push_back(std::make_pair("",-1));
            } else {
            
            
                auto it = std::lower_bound(data.begin(),data.end(),std::make_tuple(x.first,x.second,nullptr),cmp_tup<0,1,ttt>);
                if ((it==data.end()) || std::get<0>(*it)!=x.first || std::get<1>(*it)!=x.second) {
                    throw std::domain_error("no data fl "+std::to_string(x.first)+" @ "+std::to_string(x.second));
                }
                auto fb = std::get<2>(*it);
                kk->blobs.push_back(std::make_pair(fb->data,fb->uncompressed_size));
            }
        }
        blobs.push_back(kk);
        i++;
    }
}
    
size_t read_some_split_buffered_keyed_callback(
        const std::vector<std::string>& filenames,
        std::vector<std::function<void(std::shared_ptr<KeyedBlob>)>> callbacks,
        size_t index_offset, const src_locs_map& locs, bool finish_callbacks) {
    
    //std::map<std::pair<int64,int64>,std::shared_ptr<FileBlock>> data;
    
    keyedblob_vec blobs;
    read_some_split_buffered_keyed_int(filenames,locs,std::ref(blobs),false);
    
    size_t ii=index_offset;
    for (auto kk: blobs) {
        callbacks[ii%callbacks.size()](kk);
        ii++;
    }
    if (finish_callbacks) {
        for (auto& cb: callbacks) { cb(nullptr); }
    }
    
    return ii;
}
    



void rssbkca_call(const std::vector<std::string>& filenames,
    keyedblob_vec_callback callcbs,
    const src_locs_map& locs, size_t numblocks, bool skipfirst) {
    
    src_locs_map locs_temp;
    size_t aa=0;
    for (const auto& ll: locs) {
        locs_temp.insert(ll);
        aa+=ll.second.size();
        if (locs_temp.size() == numblocks) {
            
            auto blbs = std::make_shared<keyedblob_vec>();
            read_some_split_buffered_keyed_int(filenames,locs_temp,*blbs,skipfirst);
            size_t ts=0;
            for (auto& s: *blbs) {
                for (auto& x: s->blobs) {
                    ts+=x.first.size();
                }
            }
            Logger::Message() << "read " << aa << " blocks, in " << locs_temp.size() << " qts " << "[" << std::fixed << std::setprecision(1) << ts/1024./1024 << " mb]";
            callcbs(blbs);
            locs_temp.clear();
            aa=0;
        }
    }
    if (!locs_temp.empty()) {
        auto blbs = std::make_shared<keyedblob_vec>();
        
        read_some_split_buffered_keyed_int(filenames,locs_temp,*blbs,skipfirst);
        size_t ts=0;
        for (auto& s: *blbs) {
            for (auto& x: s->blobs) {
                ts+=x.first.size();
            }
        }
        Logger::Message() << "read " << aa << " blocks, in " << locs_temp.size() << " qts " << "[" << std::fixed << std::setprecision(1) << ts/1024./1024 << " mb]";
        callcbs(blbs);
        
    }
    callcbs(nullptr);
    
    
}
        
void read_some_split_buffered_keyed_callback_all_pp(const std::vector<std::string>& filenames,
        std::vector<std::function<void(std::shared_ptr<KeyedBlob>)>> callbacks,
        const src_locs_map& locs, size_t numblocks) {
    
    
    size_t ii=0;
    auto callcbs_ff = [callbacks,&ii](keyedblob_vec_ptr blbs) {
        if (!blbs) { return; }
        for (auto kk: *blbs) {
            callbacks[ii%callbacks.size()](kk);
            ii++;
        }
    };
    auto callcbs = threaded_callback<std::vector<std::shared_ptr<KeyedBlob>>>::make(callcbs_ff);
    rssbkca_call(filenames,callcbs,locs,numblocks,false);
    for (auto& cb: callbacks) { cb(nullptr); }
}    


void read_some_split_buffered_keyed_callback_all(const std::vector<std::string>& filenames,
        std::vector<std::function<void(std::shared_ptr<KeyedBlob>)>> callbacks,
        const src_locs_map& locs, size_t numblocks) {

    
    
    auto fetch_others = inverted_callback<keyedblob_vec>::make(
        [&filenames, &locs, numblocks](keyedblob_vec_callback cb) {
            rssbkca_call(filenames,cb,locs,numblocks,true);
        }
    );
    
    size_t idx=0;
    size_t i=0;
    auto others = fetch_others();
    
    
    std::ifstream firstfile(filenames.at(0), std::ios::in | std::ios::binary);
    if (!firstfile.good()) {
        std::cout << "can't open "+filenames.at(0) << std::endl;
        throw std::domain_error("can't open "+filenames.at(0));
    }
    
    for (const auto& ll: locs) {
        while (others && (i==others->size())) {
            others = fetch_others();
            i=0;
        }
        if (!others) { throw std::domain_error("wtf"); }
        if (others->at(i)->key!=ll.first) {
            throw std::domain_error("wtf");
        }
        auto oo = others->at(i);
        others->at(i).reset();
        
        auto f = ll.second.front();
        if (f.first == 0) {
            
            //files.at(0)->seekg(f.second);
            firstfile.seekg(f.second);
            auto fb = read_file_block(idx, firstfile);//*files.at(0));
            oo->blobs.at(0).first = fb->data;
            oo->blobs.at(0).second = fb->uncompressed_size;
        }
        oo->file_progress = (100.0* idx) / locs.size();
        callbacks[idx%callbacks.size()](oo);
        
        i++;
        idx++;
    }
    for (auto& cb: callbacks) { cb(nullptr); }
}    
}
