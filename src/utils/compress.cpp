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

#include "oqt/utils/compress.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <zlib.h>

namespace oqt {



std::string compress(const std::string& data, int level) {
    std::string out(data.size()+100,0);

    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    // setup "a" as the input and "b" as the compressed output
    defstream.avail_in = data.size(); // size of input, string + terminator
    defstream.next_in = (Bytef *)&data[0]; // input char array

    defstream.avail_out = (uInt)out.size(); // size of output
    defstream.next_out = (Bytef *)&out[0]; // output char array

    // the actual compression work.
    //deflateInit(&defstream, Z_BEST_COMPRESSION);
    //deflateInit(&defstream,Z_DEFAULT_COMPRESSION);
    deflateInit(&defstream,level);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    out.resize(defstream.total_out);

    return out;

}
std::string decompress(const std::string& data, size_t size) {
    if (size==0) {
        return data;
    }
    std::string out(size,0);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)(data.size()); // size of input
    infstream.next_in = (Bytef *)(&data[0]); // input char array
    infstream.avail_out = (uInt)(size); // size of output
    infstream.next_out = (Bytef *)(&out[0]); // output char array

    // the actual DE-compression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    return out;
}
std::string compress_gzip(const std::string& fn, const std::string& data, int level) {
    std::string comp = compress(data,level);
    size_t ln= 10;
    if (!fn.empty()) {
        ln += 1;
        ln += fn.size();
    }
    size_t comp_pos=ln;
    ln += comp.size()-6;
    size_t tail_pos=ln;
    ln += 8;
    
    std::string output(ln,'\0');
    
    output[0] = '\037'; //magic
    output[1] = '\213'; //magic
    output[2] = '\010'; //compression method
    
    uint32_t flags = 0;
    if (!fn.empty()) {
        flags |= 8;
    }
    
    output[3] = (char) flags;
    
    //uint32_t mtime=time(0);
    uint32_t mtime = 1514764800; //2018-01-01T00:00:00Z (so repeated calls produce same result)
    
    write_uint32_le(output, 4, mtime);
    
    output[8] = '\002';
    output[9] = '\377';
    if (!fn.empty()) {
        std::copy(fn.begin(),fn.end(),output.begin()+10);
        output[11+fn.size()] = '\0';
    }
    
    std::copy(comp.begin()+2, comp.end()-4, output.begin()+comp_pos);
    
    uint32_t crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)&data[0], data.size());
        
    
    write_uint32_le(output, tail_pos, crc);
    write_uint32_le(output, tail_pos+4, data.size());
    return output;
}
std::pair<std::string,size_t> gzip_info(const std::string& data) {
    if (
            (data[0] != '\037')
         || (data[1] != '\213')
         || (data[2] != '\010')
         || (data[8] != '\002')
         || (data[9] != '\377')) {
        throw std::domain_error("not a gzipped string");
    }
    
    std::string fn;
    if (data[3] == '\010') {
        fn = std::string(&data[10]);
    }
    size_t mtime = read_uint32_le(data, 4);
    return std::make_pair(fn, mtime);
}
        
    
    
std::string decompress_gzip(const std::string& data) {
    size_t size = read_uint32_le(data, data.size()-4);
    std::string out(size,0);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "b" as the input and "c" as the compressed output
    infstream.avail_in = (uInt)(data.size()); // size of input
    infstream.next_in = (Bytef *)(&data[0]); // input char array
    infstream.avail_out = (uInt)(size); // size of output
    infstream.next_out = (Bytef *)(&out[0]); // output char array

    // the actual DE-compression work.
    inflateInit2(&infstream, 15+MAX_WBITS);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    return out;
}   
    
}
