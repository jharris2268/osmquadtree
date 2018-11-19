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

#ifndef PBFFORMAT_READFILEBLOCKS_HPP
#define PBFFORMAT_READFILEBLOCKS_HPP


#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/pbfformat/readminimal.hpp"
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/pbfformat/readfile.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/elements/combineblocks.hpp"
#include "oqt/pbfformat/idset.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/operatingsystem.hpp"
#include "oqt/utils/compress.hpp"
#include <vector>
#include <fstream>
namespace oqt {




PrimitiveBlockPtr read_as_primitiveblock(
    std::shared_ptr<FileBlock> bl, 
    IdSetPtr idset, 
    bool ischange,
    size_t objflags);

minimal::BlockPtr read_as_minimalblock(
    std::shared_ptr<FileBlock> bl, 
    size_t objflags);


std::shared_ptr<quadtree_vector> read_as_quadtree_vector(
    std::shared_ptr<FileBlock> bl, 
    size_t objflags);


PrimitiveBlockPtr merge_as_primitiveblock(
    std::shared_ptr<FileBlock> bl, 
    IdSetPtr idset, 
    size_t objflags);

minimal::BlockPtr merge_as_minimalblock(
    std::shared_ptr<FileBlock> bl, 
    size_t objflags);





void read_block_split_internal(const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<FileBlock>)>> convblocks,
    std::vector<int64> locs);



template <class BlockType>
void read_blocks_convfunc(
    const std::string& filename,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    std::vector<int64> locs, size_t numchan, 
    std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> conv_func) {

    auto cbs = multi_threaded_callback<BlockType>::make(callback, numchan);
    read_blocks_split_convfunc(filename, cbs, locs,/* buffer,*/ conv_func);
    
}




template <class BlockType>
void read_blocks_split_convfunc(
    const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks, 
    std::vector<int64> locs, std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> conv_func) {
    
    
    //try {
        return read_block_split_internal(filename, 
            wrap_callbacks(callbacks, conv_func),
            locs);
    /*} catch (std::exception& ex) {
        for (auto c: callbacks) { c(nullptr); }
        throw ex;
    }*/
        
    
}




template <class BlockType>
void read_blocks_nothread_convfunc(
    const std::string& filename,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    std::vector<int64> locs, 
    std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> conv_func) {
    
    
    auto conv = [callback,conv_func](std::shared_ptr<FileBlock> fb) {
        if (fb) {
            callback(conv_func(fb));
        } else {
            
            callback(nullptr);
        }
    };
    
    try {
    
        if (locs.empty()) {
            int64 fs = file_size(filename);
            
            read_some_split_callback(filename, {conv}, 0, 0, fs);
        } else {
            read_some_split_locs_callback(filename, {conv}, 0, locs, 0);
        }
    } catch (std::exception& ex) {
        conv(nullptr);
        throw ex;
    }
   
}




template <class SourceType, class DestType>
std::vector<std::function<void(std::shared_ptr<SourceType>)>> wrap_callbacks(
    std::vector<std::function<void(std::shared_ptr<DestType>)>> callbacks,
    std::function<std::shared_ptr<DestType>(std::shared_ptr<SourceType>)> convfunc) {

    std::vector<std::function<void(std::shared_ptr<SourceType>)>> result;
    for (auto cb: callbacks) {
        result.push_back(threaded_callback<SourceType>::make(
            [cb, convfunc](std::shared_ptr<SourceType> bl) {
                if (bl) {
                    cb(convfunc(bl));
                } else {
                    cb(nullptr);
                }
            }
        ));
    }
    
    return result;
}
    

void read_blocks_primitiveblock(
    const std::string& filename,
    primitiveblock_callback callback,
    std::vector<int64> locs, 
    size_t numchan,
    IdSetPtr filter, bool ischange, size_t objflags);




void read_blocks_split_primitiveblock(
    const std::string& filename,
    std::vector<primitiveblock_callback> callbacks,
    std::vector<int64> locs, 
    IdSetPtr filter, bool ischange, size_t objflags);        
    
void read_blocks_convfunc_primitiveblock(
    const std::string& filename,
    primitiveblock_callback callback,
    std::vector<int64> locs, 
    size_t numchan,
    std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> convfunc);

void read_blocks_split_convfunc_primitiveblock(
    const std::string& filename,
    std::vector<primitiveblock_callback> callbacks,
    std::vector<int64> locs, 
    std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> convfunc);

void read_blocks_nothread_primitiveblock(
    const std::string& filename,
    primitiveblock_callback callback,
    std::vector<int64> locs, 
    IdSetPtr filter, bool ischange, size_t objflags);



void read_blocks_minimalblock(
    const std::string& filename,
    minimalblock_callback callback,
    std::vector<int64> locs, 
    size_t numchan, 
    size_t objflags);

void read_blocks_split_minimalblock(
    const std::string& filename,
    std::vector<minimalblock_callback> callbacks,
    std::vector<int64> locs, 
    size_t objflags);

void read_blocks_nothread_minimalblock(
    const std::string& filename,
    minimalblock_callback callback,
    std::vector<int64> locs, 
    size_t objflags);

void read_blocks_split_quadtree_vector(
    const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<quadtree_vector>)>> callbacks,
    std::vector<int64> locs, 
    size_t objflags);   





namespace readpbffile_detail {
    template <class BlockType>
    std::shared_ptr<BlockType> merge_keyedblob(std::shared_ptr<KeyedBlob>, size_t, IdSetPtr);


    template <>
    inline PrimitiveBlockPtr merge_keyedblob<PrimitiveBlock>(
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

    template <>
    inline minimal::BlockPtr merge_keyedblob<minimal::Block>(
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
}




template <class BlockType>
void read_blocks_split_merge(
    std::vector<std::string> filenames,
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks,
    src_locs_map locs,
    IdSetPtr filter, size_t objflags, size_t buffer=0) {



    auto convblocks = wrap_callbacks<KeyedBlob, BlockType>(callbacks, 
        [objflags, filter](std::shared_ptr<KeyedBlob> kb) {
            return readpbffile_detail::merge_keyedblob<BlockType>(kb, objflags, filter);
        });


    
    if (buffer==0) {
        read_some_split_locs_parallel_callback(filenames, convblocks, locs);
    } else {
        read_some_split_buffered_keyed_callback_all(filenames,convblocks,locs, buffer);
    }

}


template <class BlockType>
void read_blocks_merge(
    std::vector<std::string> filenames,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    src_locs_map locs, size_t numchan,
    IdSetPtr filter, size_t objflags, size_t buffer=0) {


    auto cbs = multi_threaded_callback<BlockType>::make(callback, numchan);
    read_blocks_split_merge(filenames,cbs,locs,filter,objflags,buffer);
    
}




template <class BlockType>
void read_blocks_merge_nothread(
    std::vector<std::string> filenames,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    src_locs_map locs,
    IdSetPtr filter, size_t objflags) {
    
    
    
    auto cb = [filter,objflags,callback](std::shared_ptr<KeyedBlob> kb) {
        if (kb) {
            callback(readpbffile_detail::merge_keyedblob<BlockType>(kb, objflags, filter));
        } else {
            callback(std::shared_ptr<BlockType>());
        }
    };
    
    
    read_some_split_locs_parallel_callback(filenames, {cb}, locs);
    
}

}

#endif
