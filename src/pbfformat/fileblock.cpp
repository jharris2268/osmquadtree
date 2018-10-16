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

#include "oqt/pbfformat/fileblock.hpp"

#include <algorithm>
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/compress.hpp"
#include "oqt/utils/logger.hpp"
#include <fstream>
namespace oqt {
    
    
std::pair<std::string, bool> read_bytes(std::istream& infile, int64 bytes) {
    std::string res(bytes,0);
    infile.read(&res[0], bytes);
    if (infile.gcount()<bytes) {
        res.resize(infile.gcount());
        return std::make_pair(res,false);
    }
        
    return std::make_pair(res, true);
}


std::pair<size_t, bool> read_bytes_into(std::istream& infile, std::string& dest, size_t place, int64 bytes) {
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


std::shared_ptr<FileBlock> read_file_block(int64 index, std::istream& infile) {
    int64 fpos = infile.tellg();
    
    uint32_t size;
    std::pair<std::string, bool> r = read_bytes(infile,4);
    if (!r.second) {
        return nullptr;
    }

    char* ss = reinterpret_cast<char*>(&size);
    ss[0] = r.first[3];
    ss[1] = r.first[2];
    ss[2] = r.first[1];
    ss[3] = r.first[0];

   
    r = read_bytes(infile,size);
    if (!r.second) {
        return nullptr;
    }


    
    size_t block_size =0;

    size_t pos = 0;
    PbfTag tag;

    
    auto result = std::make_shared<FileBlock>(index,"","",0,true);
    result->file_position = fpos;
    tag = read_pbf_tag(r.first, pos);
    while (tag.tag!=0) {

        if (tag.tag==1) {
            result->blocktype = tag.data;
        } else if (tag.tag==3) {
            block_size = tag.value;
        } else {
            Logger::Message() << "??" << tag.tag << " " << tag.value << " " << tag.data;
        }
        tag = read_pbf_tag(r.first, pos);
    }

    
    r = std::move(read_bytes(infile, block_size));
    
    pos=0;
    tag = std::move(read_pbf_tag(r.first, pos));
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
            Logger::Message() << "??" << tag.tag << " " << tag.value << " " << tag.data;
        }

        tag = std::move(read_pbf_tag(r.first, pos));
    }
    
    
    return result;
}


    
    
    
    
std::string prepare_file_block(const std::string& head, const std::string& data, int compress_level) {
    
    
    std::string comp;
    if (compress_level != 0) {
        comp = std::move(compress(data,compress_level));
    }
    
    size_t data_blob_len = 0;
    if (compress_level != 0) {
        data_blob_len = pbf_value_length(2, uint64(data.size())) + pbf_data_length(3, comp.size());
    } else {
        data_blob_len = pbf_data_length(1, data.size());
    }    
    
    size_t head_blob_len = pbf_data_length(1, head.size()) + pbf_value_length(3, data_blob_len);
    
    std::string output(4+head_blob_len + data_blob_len,0);
    output[0]=(char)((head_blob_len>>24)&0xff);
    output[1]=(char)((head_blob_len>>16)&0xff);
    output[2]=(char)((head_blob_len>>8)&0xff);
    output[3]=(char)((head_blob_len)&0xff);
    
    size_t pos = 4;
    pos = write_pbf_data(output, pos, 1, head);
    pos = write_pbf_value(output, pos, 3, data_blob_len);
    
    if (pos != (4+head_blob_len)) { throw std::domain_error("wtf"); }
    
    if (compress_level != 0) {
        pos = write_pbf_value(output, pos, 2, uint64(data.size()));
        pos = write_pbf_data(output, pos, 3, comp);
    } else {
        pos = write_pbf_data(output, pos, 1, data);
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

