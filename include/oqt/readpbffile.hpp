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

#ifndef READPBFFILE_HPP
#define READPBFFILE_HPP

#include "oqt/utils.hpp"
#include "oqt/simplepbf.hpp"
#include "oqt/minimalblock.hpp"
#include "oqt/packedblock.hpp"
#include "oqt/readfile.hpp"
#include "oqt/combineblocks.hpp"

//#include "geometry/geometrytypes.hpp"

#include <vector>
#include <fstream>
namespace oqt {

namespace readpbffile_detail {

    template <class block_type>
    std::shared_ptr<block_type> process_fileblock(
        std::shared_ptr<FileBlock>,
        std::shared_ptr<idset>, bool, size_t) {};


    template <>
    inline std::shared_ptr<primitiveblock> process_fileblock<primitiveblock>(
        std::shared_ptr<FileBlock> bl,
        std::shared_ptr<idset> filter, bool isc, size_t objflags) {
        
        
        if ((bl->blocktype=="OSMData")) {
            std::string dd = bl->get_data();
            auto r = readPrimitiveBlock(bl->idx, dd, isc, objflags, filter, nullptr);
            r->file_progress = bl->file_progress;
            r->file_position = bl->file_position;
            return r;
            
        }
        auto r=std::make_shared<primitiveblock>(-1);
        r->file_progress = bl->file_progress;
        r->file_position = bl->file_position;
        return r;
    }
/*
    template <>
    inline std::shared_ptr<packedblock> process_fileblock<packedblock>(
        std::shared_ptr<FileBlock> bl,
        std::shared_ptr<idset> filter, bool isc, size_t objflags) {
        
        
        if ((bl->blocktype=="OSMData")) {
            std::string dd = bl->get_data();
            auto r =  readPackedBlock(bl->idx, dd, objflags, isc,filter);
            r->file_progress = bl->file_progress;
            r->file_position = bl->file_position;
            return r;
            
        }
        auto r = std::make_shared<packedblock>(-1,0);
        r->file_progress = bl->file_progress;
        r->file_position = bl->file_position;
        return r;
    }
*/
    template <>
    inline std::shared_ptr<minimalblock> process_fileblock<minimalblock>(
        std::shared_ptr<FileBlock> bl,
        std::shared_ptr<idset> filter, bool isc, size_t objflags) {
        
        
        
        if ((bl->blocktype=="OSMData")) {
            std::string dd = bl->get_data();
            auto r = readMinimalBlock(bl->idx, dd, objflags);
            r->file_progress = bl->file_progress;
            r->file_position = bl->file_position;
            return r;
        }
        auto r=std::make_shared<minimalblock>();
        r->index=-1;
        r->uncompressed_size=0;
        r->file_progress = bl->file_progress;
        r->file_position = bl->file_position;
        return r;
    }
    
    template <>
    inline std::shared_ptr<qtvec> process_fileblock<qtvec>(
        std::shared_ptr<FileBlock> bl,
        std::shared_ptr<idset> filter, bool isc, size_t objflags) {
        
        std::string dd = bl->get_data();
        if ((bl->blocktype=="OSMData")) {
            return readQtVecBlock(dd, objflags);
            
        }
        auto r=std::make_shared<qtvec>();
        
        return r;
    }

    template <class BlockType>
    std::shared_ptr<BlockType> merge_keyedblob(std::shared_ptr<keyedblob>, size_t, std::shared_ptr<idset>);


    template <>
    inline std::shared_ptr<primitiveblock> merge_keyedblob<primitiveblock>(
        std::shared_ptr<keyedblob> bl,
        size_t objflags, std::shared_ptr<idset> ids) {
        
        std::vector<std::shared_ptr<primitiveblock>> changes;
        std::shared_ptr<primitiveblock> main;
        for (auto& b: bl->blobs) {
            std::string dd = decompress(b.first,b.second);
            if (main) {
                changes.push_back(readPrimitiveBlock(bl->idx, dd, true,objflags,ids));
            } else {
                main = readPrimitiveBlock(bl->idx, dd, true,objflags,ids);
            }
        }
        
        if (changes.empty()) {
            main->file_progress = bl->file_progress;
            return main;
        }
        auto comb = combine_primitiveblock_many(main,changes);
        comb->file_progress = bl->file_progress;
        comb->index = bl->idx;
        return comb;
    }
/*
    template <>
    inline std::shared_ptr<packedblock> merge_keyedblob<packedblock>(
        std::shared_ptr<keyedblob> bl,
        size_t objflags, std::shared_ptr<idset> ids) {
        
        std::vector<std::shared_ptr<packedblock>> changes;
        std::shared_ptr<packedblock> main;
        for (auto& b: bl->blobs) {
            std::string dd = decompress(b.first,b.second);
            if (main) {
                changes.push_back(readPackedBlock(bl->key, dd, objflags, true, ids));
            } else {
                main = readPackedBlock(bl->key, dd, objflags, true,ids);
            }
        }
        if (changes.empty()) {
            main->file_progress = bl->file_progress;
            return main;
        }
        auto comb =  combine_packedblock_many(main,changes);
        comb->file_progress = bl->file_progress;
        return comb;
    }*/
    template <>
    inline std::shared_ptr<minimalblock> merge_keyedblob<minimalblock>(
        std::shared_ptr<keyedblob> bl,
        size_t objflags, std::shared_ptr<idset> ids) {
        
        std::vector<std::shared_ptr<minimalblock>> changes;
        std::shared_ptr<minimalblock> main;
        int64 ts=0;
        for (auto& b: bl->blobs) {
            std::string dd = decompress(b.first,b.second);
            if (main) {
                changes.push_back(readMinimalBlock(bl->idx, dd, objflags));
            } else {
                main = readMinimalBlock(bl->idx, dd, objflags);
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
std::vector<std::function<void(std::shared_ptr<FileBlock>)>> make_readpbf_callbacks(
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks,
    std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> convfunc) {
    
    
    size_t numchan = callbacks.size();
    std::vector<std::function<void(std::shared_ptr<FileBlock>)>> convblocks;
    for (size_t i=0; i < numchan; i++) {
        auto collect=callbacks[i];
        convblocks.push_back(threaded_callback<FileBlock>::make(
            [collect,convfunc](std::shared_ptr<FileBlock> fb) {
                
                if (fb) {
                    collect(convfunc(fb));
                } else {
                    collect(std::shared_ptr<BlockType>());
                }
                
            }
        ));
    }
    return convblocks;
}



inline void read_block_split_internal(const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<FileBlock>)>> convblocks,
    std::vector<int64> locs, bool inmem) {
    /*
    
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (!file.good()) {
        for (auto c: convblocks) { c(nullptr); }
        throw std::domain_error("can't open"+filename);
    }
    */
    
    
    if (locs.empty()) {
        int64 fs = file_size(filename);
        read_some_split_callback(filename, convblocks, 0, 0, fs);
    } else {
        if (inmem) {
            read_some_split_locs_buffered_callback(filename,convblocks,0,locs,0);
        } else {
            read_some_split_locs_callback(filename, convblocks, 0, locs, 0);
        }
    }
    
}
        
    



template <class BlockType>
void read_blocks(
    const std::string& filename,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    std::vector<int64> locs, 
    size_t numchan,
    std::shared_ptr<idset> filter, bool ischange, size_t objflags, bool inmem) {

    auto cbs = multi_threaded_callback<BlockType>::make(callback, numchan);
    return read_blocks_split(filename, cbs, locs, filter, ischange,objflags,inmem);
}

/*
template <class BlockType>
void read_blocks_split(
    const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks,
    std::vector<int64> locs,
    std::shared_ptr<idset> filter, bool ischange, size_t objflags, bool inmem) {
        
    
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (!file.good()) {
        for (auto c: callbacks) { c(nullptr); }
        throw std::domain_error("can't open"+filename);
    }

    size_t numchan = callbacks.size();

    std::vector<std::function<void(std::shared_ptr<FileBlock> fb)>> convblocks;
    for (size_t i=0; i < numchan; i++) {
        auto collect=callbacks[i];
        convblocks.push_back(threaded_callback<FileBlock>::make(
            [filter,ischange,objflags, collect](std::shared_ptr<FileBlock> fb) {
                
                if (fb) {
                    collect(readpbffile_detail::process_fileblock<BlockType>(fb,filter,ischange,objflags));
                } else {
                    collect(std::shared_ptr<BlockType>());
                }
                
            }
        ));
    }
    
    if (locs.empty()) {
        int64 fs = file_size(filename);
        read_some_split_callback(file, convblocks, 0, 0, fs);
    } else {
        if (inmem) {
            read_some_split_locs_buffered_callback(file,convblocks,0,locs,0);
        } else {
            read_some_split_locs_callback(file, convblocks, 0, locs, 0);
        }
    }
    
}*/




template <class BlockType>
void read_blocks_convfunc(
    const std::string& filename,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    std::vector<int64> locs, size_t numchan,
    std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> conv_func) {

    auto cbs = multi_threaded_callback<BlockType>::make(callback, numchan);
    return read_blocks_split_convfunc(filename, cbs, locs,/* buffer,*/ conv_func);
}




template <class BlockType>
void read_blocks_split_convfunc(
    const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks,
    std::vector<int64> locs, std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> conv_func) {
    
    /*
    std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (!file.good()) {
        //logger_message() << "failed to open file " << filename;
        for (auto c: callbacks) { c(nullptr); }
        throw std::domain_error("can't open "+filename);
    }

    size_t numchan = callbacks.size();

    std::vector<std::function<void(std::shared_ptr<FileBlock> fb)>> convblocks;
    for (size_t i=0; i < numchan; i++) {
        auto collect=callbacks[i];
        convblocks.push_back(threaded_callback<FileBlock>::make(
            [conv_func, collect](std::shared_ptr<FileBlock> fb) {
                collect(conv_func(fb));
            }
        ));
    }
    
    if (locs.empty()) {
        int64 fs = file_size(filename);
        read_some_split_callback(file, convblocks, 0, 0, fs);
    } else {
        read_some_split_locs_callback(file, convblocks, 0, locs, 0);
    }*/
    try {
        return read_block_split_internal(filename, 
            make_readpbf_callbacks(callbacks, conv_func),
            locs, false);
    } catch (std::exception& ex) {
        for (auto c: callbacks) { c(nullptr); }
        throw ex;
    }
        
    
}

template <class BlockType>
void read_blocks_split(
    const std::string& filename,
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks,
    std::vector<int64> locs,
    std::shared_ptr<idset> filter, bool ischange, size_t objflags, bool inmem) {
        
    
    std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)> convfunc = 
        [filter,ischange,objflags](std::shared_ptr<FileBlock> fb) {
            return readpbffile_detail::process_fileblock<BlockType>(fb,filter,ischange,objflags);
    };
    
    return read_block_split_internal(filename, 
        make_readpbf_callbacks(callbacks, convfunc),
        locs, inmem);
        
}


template <class BlockType>
void read_blocks_nothread(
    const std::string& filename,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    std::vector<int64> locs, 
    std::shared_ptr<idset> filter, bool ischange, size_t objflags) {
    
    /*std::ifstream file(filename, std::ios::binary | std::ios::in);
    if (!file.good()) {
        throw std::domain_error("can't open"+filename);
    }*/
    auto conv = [callback,filter,ischange,objflags](std::shared_ptr<FileBlock> fb) {
        if (fb) {
            callback(readpbffile_detail::process_fileblock<BlockType>(fb,filter,ischange,objflags));
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


template <class BlockType>
void read_blocks_split_merge(
    std::vector<std::string> filenames,
    std::vector<std::function<void(std::shared_ptr<BlockType>)>> callbacks,
    src_locs_map locs,
    std::shared_ptr<idset> filter, size_t objflags, size_t buffer=0) {

    std::vector<std::function<void(std::shared_ptr<keyedblob> fb)>> convblocks;
    for (auto cb: callbacks) {
        
        convblocks.push_back(threaded_callback<keyedblob>::make(
            [filter,objflags, cb](std::shared_ptr<keyedblob> kb) {
                if (kb) {
                    cb(readpbffile_detail::merge_keyedblob<BlockType>(kb, objflags, filter));
                } else {
                    cb(nullptr);
                }
                
            }
        ));
    }
    /*
    std::vector<std::unique_ptr<std::ifstream>> files;
    for (auto fn: filenames) {
        files.push_back(std::make_unique<std::ifstream>(fn, std::ios::binary | std::ios::in));
        if (!files.back()->good()) {
            throw std::domain_error("can't open "+fn);
        }
    }
    */
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
    std::shared_ptr<idset> filter, size_t objflags, size_t buffer=0) {


    auto cbs = multi_threaded_callback<BlockType>::make(callback, numchan);
    read_blocks_split_merge(filenames,cbs,locs,filter,objflags,buffer);
    
}




template <class BlockType>
void read_blocks_merge_nothread(
    std::vector<std::string> filenames,
    std::function<void(std::shared_ptr<BlockType>)> callback,
    src_locs_map locs,
    std::shared_ptr<idset> filter, size_t objflags) {
    
    
    
    auto cb = [filter,objflags,callback](std::shared_ptr<keyedblob> kb) {
        if (kb) {
            callback(readpbffile_detail::merge_keyedblob<BlockType>(kb, objflags, filter));
        } else {
            callback(std::shared_ptr<BlockType>());
        }
    };
    /*
    std::vector<std::unique_ptr<std::ifstream>> files;
    for (auto fn: filenames) {
        files.push_back(std::make_unique<std::ifstream>(fn, std::ios::binary | std::ios::in));
        if (!files.back()->good()) {
            throw std::domain_error("can't open "+fn);
        }
    }*/
    
    read_some_split_locs_parallel_callback(filenames, {cb}, locs);
    
}
/*        
template <class BlockType>
class ReadBlocksNoThread {
    public:
        ReadBlocksNoThread(
            std::string filename, std::vector<int64> locs,
            std::shared_ptr<idset> filter_, bool ischange_, size_t objflags_) :
            file(filename, std::ios::in|std::ios::binary), filter(filter_), ischange(ischange_), objflags(objflags_) {
            
                if (!file.good()) {
                    throw std::domain_error("can't open "+filename);
                }
                auto fs = file_size(filename);
                readfile = make_readfile(file,locs,0, 128*1024*1024, fs);                
        }
        
        virtual std::shared_ptr<BlockType> next() {
            auto fb = readfile->next();
            if (!fb) {
                return nullptr;
            }
            return readpbffile_detail::process_fileblock<BlockType>(fb,filter,ischange,objflags);
        }
        
        static std::function<std::shared_ptr<BlockType>(void)> make(
            std::string filename, std::vector<int64> locs,
            std::shared_ptr<idset> filter, bool ischange, size_t objflags) {
            
            auto rbnt = std::make_shared<ReadBlocksNoThread<BlockType>>(
                filename,locs,filter,ischange,objflags);
            return [rbnt]() { return rbnt->next(); };
        }            
            
    private:
        std::ifstream file;
        std::shared_ptr<idset> filter;
        bool ischange;
        size_t objflags;
        std::shared_ptr<ReadFile> readfile;
};

template <class BlockType>
class ReadBlocksNoThreadConvFunc {
    public:
        typedef std::function<std::shared_ptr<BlockType>(std::shared_ptr<FileBlock>)>  conv_function;
        ReadBlocksNoThreadConvFunc(
            std::string filename, std::vector<int64> locs,
            conv_function convfunc_) :
            file(filename, std::ios::in|std::ios::binary), convfunc(convfunc_) {
            
                if (!file.good()) {
                    throw std::domain_error("can't open "+filename);
                }
                auto fs = file_size(filename);
                readfile = make_readfile(file,locs,0, 128*1024*1024,fs);                
        }
        
        virtual std::shared_ptr<BlockType> next() {
            auto fb = readfile->next();
            if (!fb) {
                return nullptr;
            }
            return convfunc(fb);
        }
        
        static std::function<std::shared_ptr<BlockType>(void)> make(
            std::string filename, std::vector<int64> locs,
            conv_function convfunc ) {
            
            auto rbnt = std::make_shared<ReadBlocksNoThread<BlockType>>(
                filename,locs,convfunc);
            return [rbnt]() { return rbnt->next(); };
        }            
            
    private:
        std::ifstream file;
        conv_function  convfunc;
        std::shared_ptr<ReadFile> readfile;
};
*/
}
    
#endif
