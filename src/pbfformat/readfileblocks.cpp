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

#include "oqt/pbfformat/readfileblocks.hpp"


namespace oqt {

    
PrimitiveBlockPtr read_as_primitiveblock(
    std::shared_ptr<FileBlock> bl,
    IdSetPtr filter, bool isc, size_t objflags) {
    
    
    if ((bl->blocktype=="OSMData")) {
        std::string dd = bl->get_data();
        auto r = read_primitive_block(bl->idx, dd, isc, objflags, filter, nullptr);
        r->SetFilePosition(bl->file_position);
        r->SetFileProgress(bl->file_progress);
        
        return r;
        
    }
    auto r=std::make_shared<PrimitiveBlock>(-1);
    r->SetFilePosition(bl->file_position);
    r->SetFileProgress(bl->file_progress);
    return r;
}

minimal::BlockPtr read_as_minimalblock(
    std::shared_ptr<FileBlock> bl, size_t objflags) {
    
    
    
    if ((bl->blocktype=="OSMData")) {
        std::string dd = bl->get_data();
        auto r = read_minimal_block(bl->idx, dd, objflags);
        r->file_progress = bl->file_progress;
        r->file_position = bl->file_position;
        return r;
    }
    auto r=std::make_shared<minimal::Block>();
    r->index=-1;
    r->uncompressed_size=0;
    r->file_progress = bl->file_progress;
    r->file_position = bl->file_position;
    return r;
}

std::shared_ptr<quadtree_vector> read_as_quadtree_vector(
    std::shared_ptr<FileBlock> bl, size_t objflags) {
    
    std::string dd = bl->get_data();
    if ((bl->blocktype=="OSMData")) {
        return read_quadtree_vector_block(dd, objflags);
        
    }
    auto r=std::make_shared<quadtree_vector>();
    
    return r;
}

PrimitiveBlockPtr merge_as_primitiveblock(
    std::shared_ptr<KeyedBlob> bl,
    size_t objflags, IdSetPtr ids) {
    
    std::vector<PrimitiveBlockPtr> changes;
    PrimitiveBlockPtr main;
    for (auto& b: bl->blobs) {
        std::string dd = decompress(b.first,b.second);
        if (main) {
            changes.push_back(read_primitive_block(bl->idx, dd, true,objflags,ids));
        } else {
            main = read_primitive_block(bl->idx, dd, true,objflags,ids);
        }
    }
    
    if (changes.empty()) {
    
        main->SetFileProgress(bl->file_progress);
        return main;
    }
    auto comb = combine_primitiveblock_many(main,changes);
    
    comb->SetFileProgress(bl->file_progress);
    
    return comb;
}

minimal::BlockPtr merge_as_minimalblock(
    std::shared_ptr<KeyedBlob> bl,
    size_t objflags, IdSetPtr ids) {
    
    std::vector<minimal::BlockPtr> changes;
    minimal::BlockPtr main;
    int64 ts=0;
    for (auto& b: bl->blobs) {
        std::string dd = decompress(b.first,b.second);
        if (main) {
            changes.push_back(read_minimal_block(bl->idx, dd, objflags));
        } else {
            main = read_minimal_block(bl->idx, dd, objflags);
        }
        ts+=dd.size();
    }
    
    if (changes.empty()) {
        main->file_progress=bl->file_progress;
        return main;
    }
    auto comb =  combine_minimalblock_many(main,changes);
    comb->uncompressed_size = ts;
    comb->file_progress = bl->file_progress;
    comb->index = bl->idx;
    return comb;
}
    
    
    
    
void read_block_split_internal(const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<FileBlock>)>> convblocks,
    std::vector<int64> locs) {
        
    
    try {
    
        if (locs.empty()) {
            int64 fs = file_size(filename);
            read_some_split_callback(filename, convblocks, 0, 0, fs);
        } else {
            /*if (inmem) {
                read_some_split_locs_buffered_callback(filename,convblocks,0,locs,0);
            } else {*/
                read_some_split_locs_callback(filename, convblocks, 0, locs, 0);
            //}
        }
    } catch (std::exception& ex) {
        std::cout << "read_block_split_internal failed: " << ex.what() << std::endl;
        for (auto c: convblocks) {
            c(nullptr);
        }
        throw ex;
    }
    
}




void read_blocks_primitiveblock(
    const std::string& filename,
    primitiveblock_callback callback,
    std::vector<int64> locs, 
    size_t numchan,
    IdSetPtr filter, bool ischange, size_t objflags) {
        
    
    
    return read_blocks_convfunc<PrimitiveBlock>(filename, callback, locs, numchan,
        [filter,ischange,objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_primitiveblock(fb, filter, ischange, objflags); });
}

void read_blocks_split_primitiveblock(
    const std::string& filename,
    std::vector<primitiveblock_callback> callbacks,
    std::vector<int64> locs, 
    IdSetPtr filter, bool ischange, size_t objflags)  {
        
        
    return read_blocks_split_convfunc<PrimitiveBlock>(filename, callbacks, locs,
        [filter,ischange,objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_primitiveblock(fb, filter, ischange, objflags); });
}       
    
void read_blocks_convfunc_primitiveblock(
    const std::string& filename,
    primitiveblock_callback callback,
    std::vector<int64> locs, 
    size_t numchan, 
    std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> convfunc)  {
        
        
    return read_blocks_convfunc<PrimitiveBlock>(filename, callback, locs, numchan, convfunc);
}

void read_blocks_split_convfunc_primitiveblock(
    const std::string& filename,
    std::vector<primitiveblock_callback> callbacks,
    std::vector<int64> locs, 
    std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> convfunc) {
    
    return read_blocks_split_convfunc<PrimitiveBlock>(filename, callbacks, locs, convfunc);
}

void read_blocks_nothread_primitiveblock(
    const std::string& filename,
    primitiveblock_callback callback,
    std::vector<int64> locs, 
    IdSetPtr filter, bool ischange, size_t objflags) {
        
    
    return read_blocks_nothread_convfunc<PrimitiveBlock>(filename,callback,locs,
        [filter,ischange,objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_primitiveblock(fb, filter, ischange, objflags); });
} 





void read_blocks_minimalblock(
    const std::string& filename,
    minimalblock_callback callback,
    std::vector<int64> locs, 
    size_t numchan, 
    size_t objflags) {
    
    return read_blocks_convfunc<minimal::Block>(filename, callback, locs, numchan,
        [objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_minimalblock(fb, objflags); });
    
}
    
    

void read_blocks_split_minimalblock(
    const std::string& filename,
    std::vector<minimalblock_callback> callbacks,
    std::vector<int64> locs, 
    size_t objflags) {
    
    
    return read_blocks_split_convfunc<minimal::Block>(filename, callbacks, locs,
        [objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_minimalblock(fb, objflags); });
}


void read_blocks_nothread_minimalblock(
    const std::string& filename,
    minimalblock_callback callback,
    std::vector<int64> locs, 
    size_t objflags) {
        
    return read_blocks_nothread_convfunc<minimal::Block>(filename, callback, locs,
        [objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_minimalblock(fb, objflags); });

}

void read_blocks_split_quadtree_vector(
    const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<quadtree_vector>)>> callbacks,
    std::vector<int64> locs, 
    size_t objflags)  {
        
        
    return read_blocks_split_convfunc<quadtree_vector>(filename, callbacks, locs,
        [objflags](std::shared_ptr<FileBlock> fb) {
            return read_as_quadtree_vector(fb, objflags); });
}  


}
