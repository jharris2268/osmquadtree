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

#ifndef READFILE_H
#define READFILE_H

#include "oqt/utils.hpp"
#include <map>
#include <deque>
namespace oqt {
//typedef std::tuple<int64, std::string, std::string, uint64, bool> fileBlock;
struct FileBlock {
    int64 idx;
    std::string blocktype;
    std::string data;
    size_t uncompressed_size;
    bool compressed;
    int64 file_position;
    double file_progress;
    FileBlock(int64 idx_, std::string bt_, std::string d_, size_t ucs_, bool c_) : idx(idx_), blocktype(bt_), data(d_), uncompressed_size(ucs_), compressed(c_), file_position(0), file_progress(0) {};
    std::string get_data();
};
std::pair<std::string, bool> readBytes(std::istream& infile, int64 bytes);
std::pair<size_t, bool> readBytesInto(std::istream& infile, std::string& dest, size_t place, int64 bytes);


std::shared_ptr<FileBlock> readFileBlock(int64 index, std::istream& infile);

class ReadFile {
    public:
        virtual std::shared_ptr<FileBlock> next()=0;
        virtual ~ReadFile() {}
};
std::shared_ptr<ReadFile> make_readfile(const std::string& filename, std::vector<int64> locs, size_t index_offset, size_t buffer, int64 file_size);
    

struct keyedblob {
    size_t idx;
    int64 key;
    std::deque<std::pair<std::string,size_t> > blobs;
    double file_progress;
};

typedef std::map<int64,std::vector<std::pair<size_t,int64>>> src_locs_map;

void read_some_split_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, size_t buffer, int64 file_size);
void read_some_split_locs_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer);

void read_some_split_locs_parallel_callback(const std::vector<std::string>& files, std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks, const src_locs_map& src_locs);

void read_some_split_locs_buffered_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer);

size_t read_some_split_buffered_keyed_callback(const std::vector<std::string>& files, std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks, size_t index_offset, const src_locs_map& locs, bool finish_callbacks);

void read_some_split_buffered_keyed_callback_all(const std::vector<std::string>& files, std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks, const src_locs_map& locs, size_t numblocks);
}
#endif
