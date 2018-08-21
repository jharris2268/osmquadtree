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

#include "oqt/readfile.hpp"
#include <algorithm>
#include <map>

#include <fstream>

namespace oqt {
std::pair<std::string, bool> readBytes(std::istream& infile, int64 bytes) {
    std::string res(bytes,0);
    infile.read(&res[0], bytes);
    if (infile.gcount()<bytes) {
        res.resize(infile.gcount());
        return std::make_pair(res,false);
    }
        
    return std::make_pair(res, true);
}


std::pair<size_t, bool> readBytesInto(std::istream& infile, std::string& dest, size_t place, int64 bytes) {
    //std::string res(bytes,0);
    infile.read(&dest[place], bytes);
    if (infile.gcount()<bytes) {
        //res.resize(infile.gcount());
        return std::make_pair(place+infile.gcount(),false);
    }
        
    return std::make_pair(place+bytes, true);
}




std::string FileBlock::get_data() {
    return decompress(data,uncompressed_size);
};


std::shared_ptr<FileBlock> readFileBlock(int64 index, std::istream& infile) {
    int64 fpos = infile.tellg();
    
    uint32_t size;
    std::pair<std::string, bool> r = readBytes(infile,4);
    if (!r.second) {
        return nullptr;
    }

    char* ss = reinterpret_cast<char*>(&size);
    ss[0] = r.first[3];
    ss[1] = r.first[2];
    ss[2] = r.first[1];
    ss[3] = r.first[0];

   
    r = readBytes(infile,size);
    if (!r.second) {
        return nullptr;
    }


    
    size_t block_size =0;

    size_t pos = 0;
    PbfTag tag;

    
    auto result = std::make_shared<FileBlock>(index,"","",0,true);
    result->file_position = fpos;
    tag = readPbfTag(r.first, pos);
    while (tag.tag!=0) {

        if (tag.tag==1) {
            result->blocktype = tag.data;
        } else if (tag.tag==3) {
            block_size = tag.value;
        } else {
            logger_message() << "??" << tag.tag << " " << tag.value << " " << tag.data;
        }
        tag = readPbfTag(r.first, pos);
    }

    
    r = std::move(readBytes(infile, block_size));
    
    pos=0;
    tag = std::move(readPbfTag(r.first, pos));
    while (tag.tag!=0) {

        if (tag.tag==1) {
            result->data=std::move(tag.data);
            
            result->compressed=false;
            return result;
        } else if (tag.tag==2) {
            result->uncompressed_size = tag.value;
        } else if (tag.tag==3) {
            result->data=std::move(tag.data);
            
        } else {
            logger_message() << "??" << tag.tag << " " << tag.value << " " << tag.data;
        }

        tag = std::move(readPbfTag(r.first, pos));
    }
    
    
    return result;
}




class ReadFileImpl : public ReadFile {
    public:
        ReadFileImpl(std::ifstream& infile_, size_t index_offset_, int64 file_size_) : infile(infile_), index_offset(index_offset_), file_size(file_size_), index(0) {
            
        }
        
        virtual std::shared_ptr<FileBlock> next() {
            if (infile.good() && (infile.peek()!=std::istream::traits_type::eof())) {
                size_t ii = index_offset+index;
                ++index;
                auto fb = readFileBlock(ii, infile);
                fb->file_progress = 100.0*fb->file_position / file_size;
                return fb;
            }
            return std::shared_ptr<FileBlock>();
        }
    private:
        std::ifstream& infile;
        size_t index_offset;
        int64 file_size;
        size_t index;
            
};

class ReadFileLocs : public ReadFile {
    public:
        ReadFileLocs(std::ifstream& infile_, std::vector<int64> locs_, size_t index_offset_) : infile(infile_), locs(locs_), index_offset(index_offset_), index(0) {
            
        }
        
        virtual std::shared_ptr<FileBlock> next() {
            if (index < locs.size()) {
                infile.seekg(locs.at(index));
                if (!(infile.good() && (infile.peek()!=std::istream::traits_type::eof()))) {
                    throw std::domain_error("can't read at loc "+std::to_string(index)+" "+std::to_string(locs.at(index)));
                }
                size_t ii = index_offset+index;
                ++index;
                auto fb = readFileBlock(ii,infile);
                fb->file_progress = (100.0*index) / locs.size();
                return fb;
                
            }
            return std::shared_ptr<FileBlock>();
        }
    private:
        std::ifstream& infile;
        std::vector<int64> locs;
        size_t index_offset;
        size_t index;
            
};

class ReadFileBuffered : public ReadFile {
    public:
        ReadFileBuffered(std::shared_ptr<ReadFile> readfile_, size_t sz_)
            : readfile(readfile_), sz(sz_) {}

        virtual std::shared_ptr<FileBlock> next() {
            if (pending.empty()) {
                if (fetch()==0) {
                    return nullptr;
                }
            }
            auto res = pending.front();
            pending.pop_front();
            return res;
        }
        
    private:
        std::shared_ptr<ReadFile> readfile;
        size_t sz;
        std::deque<std::shared_ptr<FileBlock>> pending;
        
        size_t fetch() {
            size_t tot=0;
            while (tot < sz) {
                auto nn = readfile->next();
                if (!nn) { 
                    return tot;
                }
                tot += nn->data.size();
                pending.push_back(nn);
            }
            
            return tot;
        }
        
};

std::shared_ptr<ReadFile> make_readfile(std::ifstream& file, std::vector<int64> locs, size_t index_offset, size_t buffer, int64 file_size) {
    
    std::shared_ptr<ReadFile> res;
    
    if (locs.empty()) {
        res = std::make_shared<ReadFileImpl>(std::ref(file),index_offset,file_size);
    } else {
        res = std::make_shared<ReadFileLocs>(std::ref(file),locs,index_offset);
    }
    if (buffer==0) {
        return res;
    }
    return std::make_shared<ReadFileBuffered>(res, buffer);
}



void read_some_split_locs_callback(std::ifstream& infile, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer) {
    
    auto rp = make_readfile(infile,locs,index_offset,buffer,0);
    
    size_t idx=0;
    auto bl = rp->next();
    while (bl) {
        callbacks[idx%callbacks.size()](bl);
        idx++;
        bl = rp->next();
    }
        
    
    for (auto& c: callbacks) {
        c(std::shared_ptr<FileBlock>());
    }
}
void read_some_split_callback(std::ifstream& infile, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, size_t buffer, int64 file_size) {

   
        
    for(size_t index=0; (infile.good() && (infile.peek()!=std::istream::traits_type::eof())); index++) {
        auto bl = readFileBlock(index+index_offset,infile);
        if (file_size>0) {
            bl->file_progress = (100.0*bl->file_position) / file_size;
        }
        callbacks[index%callbacks.size()](bl);
    }
    for (auto& c: callbacks) {
        c(std::shared_ptr<FileBlock>());
    }
}

void read_some_split_locs_parallel_callback(
    std::vector<std::unique_ptr<std::ifstream>>& files,
    std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks,
    const src_locs_map& src_locs) {
    
    size_t index=0;
    
    for (auto& src : src_locs) {
        
        try {
            
            auto res = std::make_shared<keyedblob>();
            
            res->idx = index;
            res->key=src.first;
            for (auto& lc : src.second) {
                auto& infile = *(files.at(lc.first));
                infile.seekg(lc.second);
                auto fb=readFileBlock(index,infile);
                res->blobs.push_back(std::make_pair(fb->data, fb->uncompressed_size));
            }
            res->file_progress = (100.0*index) / src_locs.size();
            callbacks[index%callbacks.size()](res);
            
        } catch (std::exception& ex) {
            logger_message() << "read_some_split_locs_parallel_callback failed at index " << index;
            for (auto& cl : callbacks) {
                cl(std::shared_ptr<keyedblob>());
            }
            throw ex;
        }
        index++;
    }
    for (auto& cl : callbacks) {
        cl(std::shared_ptr<keyedblob>());
    }
    return;
}

void read_some_split_locs_buffered_callback(std::ifstream& infile, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer) {
    std::map<int64,std::shared_ptr<FileBlock>> data;
    std::vector<int64> locs_sorted = locs;
    std::sort(locs_sorted.begin(), locs_sorted.end());
    
    auto cb = [&data](std::shared_ptr<FileBlock> fb) {
        if (fb) {
            data[fb->file_position] = fb;
        }
    };
    
    read_some_split_locs_callback(infile, {cb}, 0, locs_sorted, 0);
    

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

typedef std::vector<std::shared_ptr<keyedblob>> keyedblob_vec;
typedef std::shared_ptr<keyedblob_vec> keyedblob_vec_ptr;
typedef std::function<void(keyedblob_vec_ptr)> keyedblob_vec_callback;

void read_some_split_buffered_keyed_int(
    std::vector<std::unique_ptr<std::ifstream>>& files,
    const src_locs_map& locs,
    keyedblob_vec& blobs,
    bool skipfirst) {
        
    using ttt = std::tuple<size_t,int64,std::shared_ptr<FileBlock>>;
    std::vector<ttt> data;
    size_t ns=0;
    std::vector<std::vector<int64>> locs_sorted(files.size());
    for (const auto& xx: locs) {
        for (const auto& x: xx.second) {
            
            locs_sorted.at(x.first).push_back(x.second);
            ns++;
        }
    }
    data.reserve(ns);
    
    for (size_t i=(skipfirst ? 1 : 0); i < files.size(); i++) {
        if (!locs_sorted.at(i).empty()) {
            auto& ll = locs_sorted.at(i);
            std::sort(ll.begin(), ll.end());
            auto cb = [&data,i](std::shared_ptr<FileBlock> fb) {
                if (fb) {
                    //data[std::make_pair(i, fb->filepos)] = fb;
                    data.push_back(std::make_tuple(i,fb->file_position,fb));
                }
            };
    
            read_some_split_locs_callback(*files.at(i), {cb}, 0, ll, 0);
        }
    }
    
    size_t i=0;
    for (const auto& xx: locs) {
        auto kk = std::make_shared<keyedblob>();
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
        std::vector<std::unique_ptr<std::ifstream>>& files,
        std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks,
        size_t index_offset, const src_locs_map& locs, bool finish_callbacks) {
    
    //std::map<std::pair<int64,int64>,std::shared_ptr<FileBlock>> data;
    
    keyedblob_vec blobs;
    read_some_split_buffered_keyed_int(files,locs,std::ref(blobs),false);
    
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
    



void rssbkca_call(std::vector<std::unique_ptr<std::ifstream>>& files,
    keyedblob_vec_callback callcbs,
    const src_locs_map& locs, size_t numblocks, bool skipfirst) {
    
    src_locs_map locs_temp;
    size_t aa=0;
    for (const auto& ll: locs) {
        locs_temp.insert(ll);
        aa+=ll.second.size();
        if (locs_temp.size() == numblocks) {
            
            auto blbs = std::make_shared<keyedblob_vec>();
            read_some_split_buffered_keyed_int(files,locs_temp,*blbs,skipfirst);
            size_t ts=0;
            for (auto& s: *blbs) {
                for (auto& x: s->blobs) {
                    ts+=x.first.size();
                }
            }
            callcbs(blbs);
            locs_temp.clear();
            aa=0;
        }
    }
    if (!locs_temp.empty()) {
        auto blbs = std::make_shared<keyedblob_vec>();
        
        read_some_split_buffered_keyed_int(files,locs_temp,*blbs,skipfirst);
        size_t ts=0;
        for (auto& s: *blbs) {
            for (auto& x: s->blobs) {
                ts+=x.first.size();
            }
        }
        logger_message() << "read " << aa << " blocks, in " << locs_temp.size() << " qts " << "[" << std::fixed << std::setprecision(1) << ts/1024./1024 << " mb]";
        callcbs(blbs);
        
    }
    callcbs(nullptr);
    
    
}
        
void read_some_split_buffered_keyed_callback_all_pp(std::vector<std::unique_ptr<std::ifstream>>& files,
        std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks,
        const src_locs_map& locs, size_t numblocks) {
    
    
    size_t ii=0;
    auto callcbs_ff = [callbacks,&ii](keyedblob_vec_ptr blbs) {
        if (!blbs) { return; }
        for (auto kk: *blbs) {
            callbacks[ii%callbacks.size()](kk);
            ii++;
        }
    };
    auto callcbs = threaded_callback<std::vector<std::shared_ptr<keyedblob>>>::make(callcbs_ff);
    rssbkca_call(files,callcbs,locs,numblocks,false);
    for (auto& cb: callbacks) { cb(nullptr); }
}    


void read_some_split_buffered_keyed_callback_all(std::vector<std::unique_ptr<std::ifstream>>& files,
        std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks,
        const src_locs_map& locs, size_t numblocks) {

    
    
    auto fetch_others = inverted_callback<keyedblob_vec>::make(
        [&files, &locs, numblocks](keyedblob_vec_callback cb) {
            rssbkca_call(files,cb,locs,numblocks,true);
        }
    );
    
    size_t idx=0;
    size_t i=0;
    auto others = fetch_others();
    
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
            files.at(0)->seekg(f.second);
            auto fb = readFileBlock(idx, *files.at(0));
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
