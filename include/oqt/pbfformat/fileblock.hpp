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

#ifndef PBFFORMAT_FILEBLOCK_HPP
#define PBFFORMAT_FILEBLOCK_HPP

#include "oqt/common.hpp"

namespace oqt {

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

std::pair<std::string, bool> read_bytes(std::istream& infile, int64 bytes);
std::pair<size_t, bool> read_bytes_into(std::istream& infile, std::string& dest, size_t place, int64 bytes);


std::shared_ptr<FileBlock> read_file_block(int64 index, std::istream& infile);


std::string prepare_file_block(const std::string& blocktype, const std::string& data, int compress_level=-1);

}

#endif //PBFFORMAT_FILEBLOCK_HPP
