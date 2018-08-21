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

#include "oqt/writepbffile.hpp"
#include "oqt/writeblock.hpp"
#include "oqt/writefile.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
namespace oqt {
template<size_t N, size_t U>
bool cmp_idx_item(const std::tuple<int64,int64,int64>& l,const std::tuple<int64,int64,int64>& r) {
/*    if (std::get<N>(l)==std::get<N>(r)) {
        return std::get<U>(l)<std::get<U>(r);
    }
    return std::get<N>(l)<std::get<N>(r);*/
    
    return std::tie(std::get<N>(l),std::get<U>(l)) < std::tie(std::get<N>(r),std::get<U>(r));
    
}


void sort_block_index(block_index& index) {
    std::sort(index.begin(),index.end(),cmp_idx_item<0,1>);
}
typedef std::tuple<int64,size_t,size_t,size_t> loc_entry;

size_t sort_inmem(std::ifstream& inf, std::ofstream& outf, block_index& idx, size_t start, int64 tot) {
    
    block_index tmp;
    
    size_t pl = start;
    int64 curr=0;
    
    for (; pl < idx.size(); pl++) {
        if (curr>tot) {
            break;
        }
        tmp.push_back(std::make_tuple(pl, std::get<1>(idx[pl]),std::get<2>(idx[pl])));
        curr += std::get<2>(idx[pl]);
        
    }
    std::sort(tmp.begin(),tmp.end(),cmp_idx_item<1,0>);
    logger_message() << "read " << tmp.size() << " blocks, " << curr << " bytes {range "
            << std::get<1>(tmp.front()) << " => " << std::get<1>(tmp.back())+std::get<2>(tmp.back()) << "}";
    std::map<int64,std::string> data;
    for (auto& ii : tmp) {
        inf.seekg(std::get<1>(ii));
        auto bl = readBytes(inf,std::get<2>(ii));
        if (!bl.second) {
            logger_message() << "!!! @ " << std::get<1>(ii) << "=>" << std::get<2>(ii) << " != " << bl.first.size();
            throw std::domain_error("??");
        } 
        data[std::get<0>(ii)] = bl.first;
    }
    
    for (auto& bl: data) {
        std::get<1>(idx[bl.first]) = outf.tellp();
        outf.write(bl.second.data(),bl.second.size());
    }
    return pl;
}
    


void rewrite_indexed_file(std::string filename, std::string tempfilename, std::shared_ptr<header> head, block_index& idx, int64 tot) {
    size_t pos=0;
    for (auto& ii : idx) {
        std::get<1>(ii) = pos;
        pos += std::get<2>(ii);
    }
    
    sort_block_index(idx);

    std::ofstream outfile(filename, std::ios::out | std::ios::binary);
    if (!outfile.good()) { throw std::domain_error("can't open"); }
    
    if (head) {
        head->index = idx;
        auto hb = prepareFileBlock("OSMHeader",writePbfHeader(head));
        outfile.write(hb.data(),hb.size());
    }

    std::ifstream tempfile_read(tempfilename, std::ios::in | std::ios::binary);


    
    if (tot>0) {
        
        size_t pl = 0;
        while (pl < idx.size()) {
            pl = sort_inmem(tempfile_read, outfile, idx, pl, tot);
        }
    } else {


        for (auto& ii : idx) {
            
            tempfile_read.seekg(std::get<1>(ii));
            auto bl = readBytes(tempfile_read,std::get<2>(ii));
            if (!bl.second) {
                
                logger_message() << "!!! @ " << std::get<1>(ii) << "=>" << std::get<2>(ii) << " != " << bl.first.size();
                logger_message() << "eof? " << tempfile_read.eof() << " fail? " << tempfile_read.fail() << " bad? " << tempfile_read.bad();
                
            }
                        
            std::get<1>(ii) = outfile.tellp();
            outfile.write(bl.first.data(),bl.first.size());

        }
    }
       
    outfile.close();
    tempfile_read.close();
    
}


class PbfFileWriterImpl : public PbfFileWriter {
    public:
        PbfFileWriterImpl(const std::string& filename, std::shared_ptr<header> head)
            : file(filename, std::ios::out | std::ios::binary), pos(0) {
            
            if (!file.good()) { throw std::domain_error("can't open"); }
            if (head) {
                auto hb = prepareFileBlock("OSMHeader",writePbfHeader(head));
                file.write(hb.data(),hb.size());
                pos += hb.size();
            }
            
        }
        
        void writeBlock(int64 qt, const std::string& data) {
            index.push_back(std::make_tuple(qt, pos,data.size()));
            file.write(data.data(), data.size());
            pos += data.size();
        }
        
        
        void writeBlockPart(int64 qt, const std::string& data, size_t srcpos, size_t len) {
            index.push_back(std::make_tuple(qt, pos, len));
            file.write(data.data()+srcpos, len);
            pos += len;
        }
        
        
        block_index finish() {
            if (pos != file.tellp()) {
                logger_message() << "PbfFileWriterImpl finished: pos=" << pos << ", file.tellp()=" << file.tellp();
            }
            file.close();
            return index;
        }
        
        
        virtual ~PbfFileWriterImpl() {}
        
    private:
        std::ofstream file;
        block_index index;
        int64 pos=0;
        
};


        
        

class PbfFileWriterIndexed : public PbfFileWriter {
    public:
        PbfFileWriterIndexed(const std::string& filename_, std::shared_ptr<header> head_) :
            filename(filename_), tempfilename(filename_+"-temp"), head(head_) {
            temp = std::make_shared<PbfFileWriterImpl>(filename+"-temp", std::shared_ptr<header>());
        }
        virtual ~PbfFileWriterIndexed() {}
        
        void writeBlock(int64 qt, const std::string& data) {
            temp->writeBlock(qt,data);
        }
        
        block_index finish() {
            auto idx = temp->finish();
            
            if (head) {
                head->index = idx;
            }
            
            rewrite_indexed_file(filename, tempfilename, head, idx, 3ll*1048*1024*1024);
            std::remove(tempfilename.c_str());
            return idx;
        }
        
        
        
    private:
        std::string filename;
        std::string tempfilename;
        std::shared_ptr<header> head;
        
        std::shared_ptr<PbfFileWriter> temp;
};


size_t sort_inmem_writer(std::ifstream& inf, std::shared_ptr<PbfFileWriter> ww, const block_index& idx, size_t start, int64 tot) {
    
    block_index tmp;
    
    size_t pl = start;
    int64 curr=0;
    
    for (; pl < idx.size(); pl++) {
        if (curr>tot) {
            break;
        }
        tmp.push_back(std::make_tuple(pl, std::get<1>(idx[pl]),std::get<2>(idx[pl])));
        curr += std::get<2>(idx[pl]);
        
    }
    std::sort(tmp.begin(),tmp.end(),cmp_idx_item<1,0>);
    
    std::map<int64,std::string> data;
    for (auto& ii : tmp) {
        inf.seekg(std::get<1>(ii));
        auto bl = readBytes(inf,std::get<2>(ii));
        if (!bl.second) {
            logger_message() << "!!! @ " << std::get<1>(ii) << "=>" << std::get<2>(ii) << " != " << bl.first.size();
            throw std::domain_error("??");
        } 
        data[std::get<0>(ii)] = bl.first;
    }
    
    for (auto& bl: data) {
        ww->writeBlock(std::get<0>(idx[bl.first]), bl.second);
    }
    
    return pl;
}


size_t sort_inmem_writer_alt(std::ifstream& inf, std::shared_ptr<PbfFileWriter> ww, const block_index& idx, size_t start, int64 tot) {
    
    block_index tmp;
    std::vector<loc_entry> locs;
    
    
    
    size_t pl = start;
    int64 curr=0;
    
    for (; pl < idx.size(); pl++) {
        if (curr>tot) {
            break;
        }
        tmp.push_back(std::make_tuple(pl, std::get<1>(idx[pl]), std::get<2>(idx[pl])));
        curr += std::get<2>(idx[pl]);
        
    }
    
    std::string data(curr, 0);
    size_t pos=0;
    locs.reserve(tmp.size());
    
    std::sort(tmp.begin(),tmp.end(),cmp_idx_item<1,0>);
    
    //std::map<int64,std::string> data;
    for (auto& ii : tmp) {
        inf.seekg(std::get<1>(ii));
        
        locs.push_back(std::make_tuple(std::get<0>(ii), 0, pos, std::get<2>(ii)));
        bool suc;
        std::tie(pos,suc) = readBytesInto(inf, data, pos, std::get<2>(ii));
        
        
        if (!suc) {
            logger_message() << "!!! @ " << std::get<1>(ii) << "=>" << std::get<2>(ii);
            throw std::domain_error("??");
        } 
        
    }
    std::sort(locs.begin(), locs.end(), [](const loc_entry& l, const loc_entry& r) { return std::get<0>(l)<std::get<0>(r); });
    for (const auto& l: locs) {
        
        ww->writeBlock(std::get<0>(l), data.substr(std::get<2>(l), std::get<3>(l)));
    }
    
    return pl;
}

void copy_file_ordered(const std::string& srcfn, const block_index& idx, std::shared_ptr<PbfFileWriter> out) {
    std::ifstream inf(srcfn, std::ios::binary|std::ios::in);
    size_t pl=0;
    while (pl < idx.size()) {
        pl = sort_inmem_writer_alt(inf, out, idx, pl, 3ll * 1024*1024*1024);
    }
    inf.close();
    
}
    
    
    
    

class PbfFileWriterIndexedSplit : public PbfFileWriter {
    public:
        PbfFileWriterIndexedSplit(
            const std::string& filename_,
            std::shared_ptr<header> hh_,
            size_t split_at_) : 
                filename(filename_), hh(hh_), split_at(split_at_) {}
        
        virtual ~PbfFileWriterIndexedSplit() {}
        
        void writeBlock(int64 qt, const std::string& data) {
            int64 kk = qt / split_at;
            auto it = temps.find(kk);
            if (it==temps.end()) {
                std::string tfn = filename + "-temp-pt" + std::to_string(kk);
                it = temps.insert(std::make_pair(kk, std::make_pair(tfn,std::make_shared<PbfFileWriterImpl>(tfn,nullptr)))).first;
            }
            it->second.second->writeBlock(qt,data);
        }
        
        block_index finish() {
            block_index idx;
            std::vector<std::pair<std::string,block_index>> temps_idx;
            for (auto& pp: temps) {
                auto tidx = pp.second.second->finish();
                sort_block_index(tidx);
                temps_idx.push_back(std::make_pair(pp.second.first, tidx));
                std::copy(tidx.begin(),tidx.end(),std::back_inserter(idx));
            }
            if (hh) {
                hh->index=idx;
            }                                    
            finalwriter=std::make_shared<PbfFileWriterImpl>(filename,hh);
            for (auto& pp : temps_idx) {
                copy_file_ordered(pp.first,pp.second, finalwriter);
                std::remove(pp.first.c_str());
            }
            
            return finalwriter->finish();
        }
    private:
        std::string filename;
        std::shared_ptr<header> hh;
        std::shared_ptr<PbfFileWriter> finalwriter;
        
        size_t split_at;
        std::map<int64,std::pair<std::string,std::shared_ptr<PbfFileWriterImpl>>> temps;
};        

std::shared_ptr<PbfFileWriter> make_pbffilewriter_indexedsplit(const std::string& fn, std::shared_ptr<header> head, size_t split_at) {
    return std::make_shared<PbfFileWriterIndexedSplit>(fn,head,split_at);
}
    
write_file_callback make_callback(std::shared_ptr<PbfFileWriter> writer) {
    return [writer](std::shared_ptr<std::pair<int64,std::string>> bl) {
        if (bl) {
            writer->writeBlock(bl->first,bl->second);
        } else {
            writer->finish();
        }
    };
}

std::shared_ptr<PbfFileWriter> make_pbffilewriter(
    const std::string& filename, std::shared_ptr<header> head, bool indexed) {
    
    if (indexed) {
        return std::make_shared<PbfFileWriterIndexed>(filename,head);
    }
    return std::make_shared<PbfFileWriterImpl>(filename,head);
}

write_file_callback make_pbffilewriter_callback(
    const std::string& filename, std::shared_ptr<header> head, bool indexed) {
    
    return make_callback(make_pbffilewriter(filename,head,indexed));
}


class PbfFileWriterIndexedInmem : public PbfFileWriter {
    public:
        PbfFileWriterIndexedInmem(const std::string& fn_, std::shared_ptr<header> head_)
            : fn(fn_), head(head_) {}
        
        virtual ~PbfFileWriterIndexedInmem() {}
        
        virtual void writeBlock(int64 qt, const std::string& data) {
            //temps.push_back(std::make_pair(qt,data));
            temps.push_back(std::make_shared<keystring>(qt,data));
        }
        
        virtual void writeBlock(keystring_ptr p) {
            if (p) {
                temps.push_back(p);
            }
        }
        
        block_index finish() {
            std::sort(temps.begin(),temps.end(),[](const keystring_ptr& l, const keystring_ptr& r) { return l->first<r->first; });
            if (head) {
                int64 p=0;
                head->index.clear();
                head->index.reserve(temps.size());
                for (const auto& ks : temps) {
                    head->index.push_back(std::make_tuple(ks->first,p,ks->second.size()));
                    p += ks->second.size();
                }
            }
            
            auto out = make_pbffilewriter(fn,head,false);
            for (auto& ks : temps) {
                //out->writeBlock(ks.first,ks.second);
                out->writeBlock(ks);
                ks.reset();
            }
            std::vector<keystring_ptr> x;
            temps.swap(x);
            
            return out->finish();
            
        }
    private:
        std::string fn;
        std::shared_ptr<header> head;
        std::vector<keystring_ptr> temps;
        
};



class PbfFileWriterIndexedInmemAlt : public PbfFileWriter {
    
    
    typedef std::pair<std::string, size_t> blob_entry;
    public:
        PbfFileWriterIndexedInmemAlt(const std::string& fn_, std::shared_ptr<header> head_)
            : fn(fn_), head(head_) {
            
            blob_size=1024*1024*128;
            blobs.push_back(std::make_pair(std::string(blob_size,0),0));
        
        }
        
        blob_entry& check_blob(size_t sz) {
            if ((sz + blobs.back().second) > blob_size) {
                blobs.push_back(std::make_pair(std::string(blob_size,0),0));
                
            }
            return blobs.back();
        }
        
        virtual ~PbfFileWriterIndexedInmemAlt() {}
        
        virtual void writeBlock(int64 qt, const std::string& data) {
            
            auto& blob = check_blob(data.size());
            
            locs.push_back(std::make_tuple(qt, blobs.size()-1, blob.second, data.size()));
            std::copy(data.begin(), data.end(), blob.first.begin()+blob.second);
            blob.second+=data.size();
            
        }
        
        block_index finish() {
            std::sort(locs.begin(),locs.end(),[](const loc_entry& l, const loc_entry& r) { return std::get<0>(l)<std::get<0>(r); });
            if (head) {
                int64 p=0;
                head->index.clear();
                head->index.reserve(locs.size());
                for (const auto& ks : locs) {
                    head->index.push_back(std::make_tuple(std::get<0>(ks),p,std::get<3>(ks)));
                    p += std::get<3>(ks);
                }
            }
            
            auto out = std::make_shared<PbfFileWriterImpl>(fn,head);
            for (const auto& ks : locs) {
                out->writeBlockPart(std::get<0>(ks), blobs[std::get<1>(ks)].first, std::get<2>(ks), std::get<3>(ks));
                
            }
            std::vector<loc_entry> x;
            locs.swap(x);
            
            std::vector<std::pair<std::string,size_t>> y;
            blobs.swap(y);
            
            return out->finish();
            
        }
    private:
        std::string fn;
        std::shared_ptr<header> head;
        size_t blob_size;
        std::vector<std::pair<std::string,size_t>> blobs;
        std::vector<std::tuple<int64,size_t,size_t,size_t>> locs;
        
};


std::shared_ptr<PbfFileWriter> make_pbffilewriter_indexedinmem(const std::string& fn, std::shared_ptr<header> head) {
    return std::make_shared<PbfFileWriterIndexedInmemAlt>(fn,head);
}

std::shared_ptr<header> getHeaderBlock_file(std::ifstream& infile) {
    auto fb = readFileBlock(0,infile);
    if ((!fb) || (fb->blocktype!="OSMHeader")) {
        throw std::domain_error("first block not a header");
    }
    int64 p = infile.tellg();
    
    return readPbfHeader(fb->get_data(),p);
}

std::shared_ptr<header> getHeaderBlock(const std::string& fn) {
    std::ifstream infile(fn, std::ios::binary | std::ios::in);
    if (!infile.good()) { throw std::domain_error("not a file?? "+fn); }
    auto r= getHeaderBlock_file(infile);
    infile.close();
    return r;
}

}
