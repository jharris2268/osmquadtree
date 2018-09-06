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

#ifndef PBFFORMAT_READFILEPARALLEL_HPP
#define PBFFORMAT_READFILEPARALLEL_HPP

#include "oqt/pbfformat/fileblock.hpp"
#include <map>
#include <deque>
namespace oqt {
    
struct keyedblob {
    size_t idx;
    int64 key;
    std::deque<std::pair<std::string,size_t> > blobs;
    double file_progress;
};

typedef std::map<int64,std::vector<std::pair<size_t,int64>>> src_locs_map;

void read_some_split_locs_parallel_callback(const std::vector<std::string>& files, std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks, const src_locs_map& src_locs);
size_t read_some_split_buffered_keyed_callback(const std::vector<std::string>& files, std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks, size_t index_offset, const src_locs_map& locs, bool finish_callbacks);
void read_some_split_buffered_keyed_callback_all(const std::vector<std::string>& files, std::vector<std::function<void(std::shared_ptr<keyedblob>)>> callbacks, const src_locs_map& locs, size_t numblocks);

}

#endif //PBFFORMAT_READFILEPARALLEL_HPP
