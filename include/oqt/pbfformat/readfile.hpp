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

#ifndef PBFFORMAT_READFILE_HPP
#define PBFFORMAT_READFILE_HPP



#include "oqt/pbfformat/fileblock.hpp"
namespace oqt {

class ReadFile {
    public:
        virtual std::shared_ptr<FileBlock> next()=0;
        virtual ~ReadFile() {}
};
std::shared_ptr<ReadFile> make_readfile(const std::string& filename, std::vector<int64> locs, size_t index_offset, size_t buffer, int64 file_size);


void read_some_split_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, size_t buffer, int64 file_size);
void read_some_split_locs_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer);
void read_some_split_locs_buffered_callback(const std::string& filename, std::vector<std::function<void(std::shared_ptr<FileBlock>)>> callbacks, size_t index_offset, const std::vector<int64>& locs, size_t buffer);
}


#endif //PBFFORMAT_READFILE_HPP
