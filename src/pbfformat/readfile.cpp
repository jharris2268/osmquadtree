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
#include "oqt/utils/logger.hpp"

#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/invertedcallback.hpp"
#include "oqt/utils/operatingsystem.hpp"

#include <algorithm>
#include <iostream>

#include <map>
#include <deque>

#include <fstream>

namespace oqt {



class ReadFileImpl : public ReadFile {
    public:
        ReadFileImpl(const std::string& filename, size_t index_offset_, int64 file_size_, int64 startpos) : infile(filename, std::ios::in | std::ios::binary), index_offset(index_offset_), file_size(file_size_), index(0) {
            if (!infile.good()) {
                throw std::domain_error("can't open "+filename);
            }
            
            if (file_size==0) {
                file_size = oqt::file_size(filename);
            }
            
            if (startpos>0) {
                infile.seekg(startpos);
            }
            
        }
        virtual int64 file_position() { return infile.tellg(); }
        
        virtual std::shared_ptr<FileBlock> next() {
            if (infile.good() && (infile.peek()!=std::istream::traits_type::eof())) {
                size_t ii = index_offset+index;
                ++index;
                auto fb = read_file_block(ii, infile);
                fb->file_progress = 100.0*fb->file_position / file_size;
                return fb;
            }
            return std::shared_ptr<FileBlock>();
        }
    private:
        std::ifstream infile;
        size_t index_offset;
        int64 file_size;
        size_t index;
            
};

std::shared_ptr<ReadFile> make_readfile_startpos(const std::string& filename, size_t index_offset, int64 file_size, int64 startpos) {
    return std::make_shared<ReadFileImpl>(filename, index_offset, file_size, startpos);
}

class ReadFileLocs : public ReadFile {
    public:
        ReadFileLocs(const std::string& filename, std::vector<int64> locs_, size_t index_offset_) : infile(filename, std::ios::in | std::ios::binary), locs(locs_), index_offset(index_offset_), index(0) {
            if (!infile.good()) {
                throw std::domain_error("can't open "+filename);
            }
        }
        virtual int64 file_position() { return infile.tellg(); }
        
        virtual std::shared_ptr<FileBlock> next() {
            if (index < locs.size()) {
                infile.seekg(locs.at(index));
                if (!(infile.good() && (infile.peek()!=std::istream::traits_type::eof()))) {
                    throw std::domain_error("can't read at loc "+std::to_string(index)+" "+std::to_string(locs.at(index)));
                }
                size_t ii = index_offset+index;
                ++index;
                auto fb = read_file_block(ii,infile);
                fb->file_progress = (100.0*index) / locs.size();
                return fb;
                
            }
            return std::shared_ptr<FileBlock>();
        }
    private:
        std::ifstream infile;
        std::vector<int64> locs;
        size_t index_offset;
        size_t index;
            
};


class ReadFileImplXX : public ReadFile {
    public:
        ReadFileImplXX(std::ifstream& infile_, size_t index_offset_, int64 file_size_) : infile(infile_), index_offset(index_offset_), file_size(file_size_), index(0) {
           
        }
        virtual int64 file_position() { return -1; }
        
        virtual std::shared_ptr<FileBlock> next() {
            if (infile.good() && (infile.peek()!=std::istream::traits_type::eof())) {
                size_t ii = index_offset+index;
                ++index;
                auto fb = read_file_block(ii, infile);
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


class ReadFileBuffered : public ReadFile {
    public:
        ReadFileBuffered(std::shared_ptr<ReadFile> readfile_, size_t sz_)
            : readfile(readfile_), sz(sz_) {}

        virtual int64 file_position() { return readfile->file_position(); }

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

std::shared_ptr<ReadFile> make_readfile(const std::string& filename, std::vector<int64> locs, size_t index_offset, size_t buffer, int64 file_size) {
    
    std::shared_ptr<ReadFile> res;
    
    if (locs.empty()) {
        res = std::make_shared<ReadFileImpl>(filename,index_offset,file_size,0);
    } else {
        res = std::make_shared<ReadFileLocs>(filename,locs,index_offset);
    }
    if (buffer==0) {
        return res;
    }
    return std::make_shared<ReadFileBuffered>(res, buffer);
}



void read_some_split_locs_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer) {
    
    auto rp = make_readfile(filename,locs,index_offset,buffer,0);
    
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



        
    
    


void read_some_split_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, size_t buffer, int64 file_size) {

    auto rp = make_readfile(filename,{},index_offset,buffer,file_size);
        
    size_t index=0;
    auto bl = rp->next();
    while (bl) {
        if (file_size>0) {
            bl->file_progress = (100.0*bl->file_position) / file_size;
        }
        callbacks[index%callbacks.size()](bl);
        index++;
        bl = rp->next();
    }
    for (auto& c: callbacks) {
        c(std::shared_ptr<FileBlock>());
    }
}
}
