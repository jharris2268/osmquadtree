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



#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/calcqts/writewaynodes.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/threadedcallback.hpp"

namespace oqt {
    

typedef std::function<void(std::shared_ptr<WayNodes>)> waynodes_callback;


class WriteWayNodes {
    public:
        WriteWayNodes(
            write_file_callback writer_, minimalblock_callback relations_,
            size_t block_size_, int64 split_at_, int comp_level_) :
            
            relations(relations_),
            block_size(block_size_), split_at(split_at_),
            writer(writer_),comp_level(comp_level_)  {}
        
        
        
        
        std::shared_ptr<WayNodesWrite> tile_at(size_t k) {
            if (k>=waynodes.size()) { return nullptr; }
            return waynodes[k];
        }
        void reset_tile(size_t k) {
            if (k<waynodes.size()) { waynodes[k]->reset(false); }
        }
        void empty_tiles() {
            for (auto& t: waynodes) { t.reset(); }
            std::vector<std::shared_ptr<WayNodesWrite>> x;
            waynodes.swap(x);
        }
        size_t num_tiles() { return waynodes.size(); }
        
        void call(minimal::BlockPtr block) {
            if (!block) {
                
                for (size_t k=0; k < waynodes.size(); k++) {
                    if (waynodes[k]) {
                        finish_tile(k);
                    }
                }
                    
            
                relations(nullptr);
                
                writer(nullptr);
                empty_tiles();
                
                return;
            }
            for (const auto& w : block->ways) {
                int64 ndref=0;
                size_t pos=0;
                while ( pos < w.refs_data.size()) {
                    ndref += read_varint(w.refs_data,pos);
                    int64 ki = ndref/split_at;
                    if (ki<0) { throw std::domain_error("???"); }
                    size_t k = ki;
                    
                    
                    if (k>=waynodes.size()) {
                        waynodes.resize(k+1);
                    }
                    if (!waynodes[k]) {
                        waynodes[k] = make_way_nodes_write(block_size==0?16384:block_size,k);
                    }
                    
                    if (waynodes[k]->add(w.id,ndref,block_size==0)) {
                        finish_tile(k);
                        
                        
                    }

                }
            }
            
            if (block->relations.size()>0) {
                relations(block);
            }
        }
        
        void finish_tile(size_t k) {
            auto tile = tile_at(k);
            if (tile) {
                tile->sort_way();
                //writer(pack_noderefs_alt(k, tile, 0, tile->size(),comp_level));
                writer(pack_waynodes_block(tile));
                reset_tile(k);
            }
        }
        
    private:
        
        minimalblock_callback relations;
        
        size_t block_size;
        int64 split_at;
        
       std::vector<std::shared_ptr<WayNodesWrite>> waynodes;
       
       
        write_file_callback writer;
        int comp_level;
       
};
            

minimalblock_callback make_write_waynodes_callback(
    write_file_callback writer, minimalblock_callback relations,
            size_t block_size, int64 split_at, int comp_level) {

    auto wwn = std::make_shared<WriteWayNodes>(writer,relations,block_size,split_at,comp_level);
    return [wwn](minimal::BlockPtr mb) {
        wwn->call(mb);
    };
}


    
    
     
class WayNodesFilePrep {
    public:
        WayNodesFilePrep(const std::string& fn_) : fn(fn_) {}
        
        std::shared_ptr<WayNodesFile> make_waynodesfile() {
            Logger::Message() << "make_waynodesfile fn=" << fn << ", ll.size()=" << ll.size();
            //return std::make_shared<WayNodesFileImpl>(fn, ll);
            
            return oqt::make_waynodesfile(fn, ll);
            
        }
        
        
        //std::vector<minimalblock_callback> make_writewaynodes(std::shared_ptr<calculate_relations> rels, bool resort, size_t numchan) {
        minimalblock_callback make_writewaynodes(std::shared_ptr<CalculateRelations> rels, bool sortinmem, size_t numchan) {
            if (sortinmem) {
                waynodes_writer_obj = make_pbffilewriter_indexedinmem(fn, nullptr);
            } else {
                waynodes_writer_obj = make_pbffilewriter_filelocs(fn,nullptr);
            }
                
            //auto add_relations = threaded_callback<way_nodes>::make([rels](std::shared_ptr<way_nodes> r) {
            minimalblock_callback add_relations = [rels](minimal::BlockPtr r) {
                if (r) {
                    rels->add_relations_data(r);
                }
            };
                        
            auto wwo = waynodes_writer_obj;
            typedef std::function<void(keystring_ptr)> keystring_callback;
            keystring_callback write_waynodes = [wwo](keystring_ptr k) {
                if (k) {
                    wwo->writeBlock(k);
                }
            };
            
            
            if (numchan!=0) {
                add_relations = threaded_callback<minimal::Block>::make(add_relations,numchan);
                write_waynodes = threaded_callback<keystring>::make(write_waynodes,numchan);
            }
            
            size_t block_size = 1<<12;//(sortinmem ? 12 : 16);
            int64 split_at = 1<<20;
            int comp_level=1;
            
            std::vector<minimalblock_callback> pack_waynodes;
            if (numchan==0) {
                pack_waynodes.push_back(make_write_waynodes_callback(write_waynodes, add_relations, block_size,split_at,comp_level));
            } else {
            
                for (size_t i=0; i < numchan; i++) {
                    pack_waynodes.push_back(threaded_callback<minimal::Block>::make(
                        
                        make_write_waynodes_callback(write_waynodes, add_relations, block_size,split_at,comp_level)
                    )
                    );
                }
            }
            
            node_blocks.reserve(800000);
            return [pack_waynodes,this](minimal::BlockPtr bl) {
                if (!bl) {
                    for (auto pw: pack_waynodes) { pw(bl); }
                } else {
                    
                    Logger::Progress(bl->file_progress) << "writing way nodes";
                    
                    if (bl->has_nodes) { node_blocks.push_back(bl->file_position); }
                    pack_waynodes[bl->index % pack_waynodes.size()](bl);
                }
            };
            
            
        }
        
        std::pair<std::string,std::vector<int64>> finish_writewaynodes() {
            if (!waynodes_writer_obj) {
                throw std::domain_error("waynodes not written???");
            }
            block_index idx = waynodes_writer_obj->finish();
            ll.reserve(idx.size());
            
            sort_block_index(idx);
            for (const auto& l: idx) {
                ll.push_back(std::get<1>(l));
            }
            waynodes_writer_obj.reset();
            return std::make_pair(nodefn,std::move(node_blocks));
        }
        
        
        
    private:
        std::string fn;
        bool writelocs;
        std::string nodefn;
        std::vector<int64> ll;
        std::vector<int64> node_blocks;
        std::shared_ptr<PbfFileWriter> waynodes_writer_obj;
};



std::tuple<std::shared_ptr<WayNodesFile>,std::shared_ptr<CalculateRelations>,std::string,std::vector<int64>>
    write_waynodes(const std::string& orig_fn, const std::string& waynodes_fn, size_t numchan, bool sortinmem) {
    
    auto rels = make_calculate_relations();
    auto waynodes = std::make_shared<WayNodesFilePrep>(waynodes_fn);
        
    auto pack_waynodes= waynodes->make_writewaynodes(rels, sortinmem, numchan);
        
    
    if (numchan!=0) {
        read_blocks_minimalblock(orig_fn, pack_waynodes, {}, 4, 6); //6 = ways (2) | rels (4)
    } else {
        read_blocks_nothread_minimalblock(orig_fn, pack_waynodes, {}, 6);
    }
        
    
    Logger::Message() << rels->str();
    Logger::Get().time("wrote waynodes");
    
    auto res = waynodes->finish_writewaynodes();
    Logger::Message() << "have " << res.second.size() << " blocks with nodes";
    
    Logger::Get().time("finished waynodes");    
    
    return std::make_tuple(waynodes->make_waynodesfile(),rels,orig_fn,res.second);
}
    
    
    
    
}
