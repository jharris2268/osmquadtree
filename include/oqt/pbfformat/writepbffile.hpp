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

#ifndef PBFFORMAT_WRITEPBFFILE_HPP
#define PBFFORMAT_WRITEPBFFILE_HPP

#include "oqt/common.hpp"
#include "oqt/elements/header.hpp"
namespace oqt {
typedef std::pair<int64,std::string> keystring;
typedef std::shared_ptr<keystring> keystring_ptr;
typedef std::function<void(keystring_ptr)> write_file_callback;

class PbfFileWriter {
    public:
        virtual void writeBlock(int64 qt, const std::string& data)=0;
        virtual void writeBlock(keystring_ptr p) {
            if (p) {
                writeBlock(p->first, p->second);
            }
        }
        virtual block_index finish()=0;
        virtual ~PbfFileWriter() {}
        
        
};
std::shared_ptr<PbfFileWriter> make_pbffilewriter(const std::string& filename, std::shared_ptr<header> head, bool indexed);
write_file_callback make_pbffilewriter_callback(const std::string& filename, std::shared_ptr<header> head, bool indexed);

void rewrite_indexed_file(std::string filename, std::string tempfilename, std::shared_ptr<header> head, block_index& idx);
void sort_block_index(block_index& index);

std::shared_ptr<PbfFileWriter> make_pbffilewriter_indexedinmem(const std::string& fn, std::shared_ptr<header> head);
std::shared_ptr<PbfFileWriter> make_pbffilewriter_indexedsplit(const std::string& fn, std::shared_ptr<header> head, size_t split);

std::shared_ptr<header> getHeaderBlock(const std::string& fn);
}
#endif //PBFFORMAT_WRITEPBFFILE_HPP
