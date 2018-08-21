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

#include "oqt/writefile.hpp"
#include <algorithm>


namespace oqt {
std::string prepareFileBlock(const std::string& head, const std::string& data, int compress_level) {
    
    
    std::string comp;
    if (compress_level != 0) {
        comp = std::move(compress(data,compress_level));
    }
    
    size_t data_blob_len = 0;
    if (compress_level != 0) {
        data_blob_len = pbfValueLength(2, uint64(data.size())) + pbfDataLength(3, comp.size());
    } else {
        data_blob_len = pbfDataLength(1, data.size());
    }    
    
    size_t head_blob_len = pbfDataLength(1, head.size()) + pbfValueLength(3, data_blob_len);
    
    std::string output(4+head_blob_len + data_blob_len,0);
    output[0]=(char)((head_blob_len>>24)&0xff);
    output[1]=(char)((head_blob_len>>16)&0xff);
    output[2]=(char)((head_blob_len>>8)&0xff);
    output[3]=(char)((head_blob_len)&0xff);
    
    size_t pos = 4;
    pos = writePbfData(output, pos, 1, head);
    pos = writePbfValue(output, pos, 3, data_blob_len);
    
    if (pos != (4+head_blob_len)) { throw std::domain_error("wtf"); }
    
    if (compress_level != 0) {
        pos = writePbfValue(output, pos, 2, uint64(data.size()));
        pos = writePbfData(output, pos, 3, comp);
    } else {
        pos = writePbfData(output, pos, 1, data);
    }
    
    if (pos != output.size()) { throw std::domain_error("wtf"); }
    
    return output;
}

/*
std::shared_ptr<fileBlock> prepFileBlock(const std::string& head, const std::string& data, int level) {
    return std::make_shared<fileBlock>(0, head, compress(data, level), data.size(), true);
}
*/

}
