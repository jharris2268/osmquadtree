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

#include "oqt/pbfformat/readminimal.hpp"
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/utils/pbf/protobuf_messages.hpp"

#include <iostream>
#include <iterator>
#include <iomanip>

namespace oqt {






void read_minimal_dense_info(minimal::BlockPtr& block, size_t firstnd, const std::string& data, size_t pos, size_t lim) {
    read_pbf_messages(data,pos,lim, 
        handle_pbf_packed_int{1, nullptr, [&block,&firstnd](size_t i, uint64 v) { block->nodes.at(firstnd+i).version = v; }},
        handle_pbf_packed_int_delta{2, nullptr, [&block,&firstnd](size_t i, int64 v) { block->nodes.at(firstnd+i).timestamp = v; }}
    );
    
}
            

void read_minimal_dense(minimal::BlockPtr& block, const std::string& data, size_t pos, size_t lim) {
    size_t firstnd = block->nodes.size();
    
    
    
    read_pbf_messages(data,pos,lim,
        handle_pbf_packed_int_delta{1, 
                [&block,&firstnd](size_t sz) { block->nodes.resize(firstnd+sz); },
                [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).id = v; }
        },
        handle_pbf_data{5, [&block,&firstnd](const std::string& d, size_t p, size_t l) {
            read_minimal_dense_info(block,firstnd,d,p,l);
        }},
        handle_pbf_packed_int_delta{8, nullptr, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).lat = v; }},
        handle_pbf_packed_int_delta{9, nullptr, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).lon = v; }},
        handle_pbf_packed_int_delta{20, nullptr,[&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).quadtree = v; }}
    );
    
}

template <class T>
void read_minimal_info(T& obj, const std::string& data, size_t pos, size_t lim) {
    
    
    read_pbf_messages(data,pos,lim,
        handle_pbf_value{1,[&obj](uint64 v) { obj.version=v; }},
        handle_pbf_value{2,[&obj](uint64 v) { obj.timestamp=v; }}
    );
    
    
}
    

void read_minimal_way(minimal::BlockPtr& block, const std::string& data, size_t pos, size_t lim) {
    
    minimal::Way w;
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&w](uint64 v) { w.id=v; }},
        handle_pbf_data{4, [&w](const std::string& d, size_t p, size_t l) { read_minimal_info(w, d, p, l); }},
        handle_pbf_data{8, [&w](const std::string& d, size_t p, size_t l) { w.refs_data=d.substr(p,l-p); }},
        handle_pbf_value{20, [&w](uint64 v) { w.quadtree = un_zig_zag(v); }}
    );
    
    block->ways.emplace_back(std::move(w));
    
}
void read_minimal_relation(minimal::BlockPtr& block, const std::string& data, size_t pos, size_t lim) {
    
    minimal::Relation r;
    
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&r](uint64 v) { r.id=v; }},
        handle_pbf_data{4, [&r](const std::string& d, size_t p, size_t l) { read_minimal_info(r, d, p, l); }},
        handle_pbf_data{9, [&r](const std::string& d, size_t p, size_t l) { r.refs_data=d.substr(p,l-p); }},
        handle_pbf_data{10, [&r](const std::string& d, size_t p, size_t l) { r.tys_data=d.substr(p,l-p); }},
        handle_pbf_value{20, [&r](uint64 v) { r.quadtree = un_zig_zag(v); }}
    );
    block->relations.emplace_back(std::move(r));
    
    
}
void read_minimal_geometry(minimal::BlockPtr& block, size_t gm_type, const std::string& data, size_t pos, size_t lim) {
    
    minimal::Geometry r;
    r.ty = gm_type;
    
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&r](uint64 v) { r.id=v; }},
        handle_pbf_data{4, [&r](const std::string& d, size_t p, size_t l) { read_minimal_info(r, d, p, l); }},
        handle_pbf_value{20, [&r](uint64 v) { r.quadtree = un_zig_zag(v); }}
    );
    block->geometries.emplace_back(std::move(r));
    
   
}


template <class T>
void set_changetype(std::vector<T>& objs, size_t first, size_t ct) {
    if (objs.size()>first) {
        for (auto it=objs.begin()+first; it < objs.end(); ++it) {
            it->changetype=ct;
        }
    }
}

void read_minimal_group(minimal::BlockPtr& block, const std::string& data, size_t objflags, size_t pos, size_t lim) {
    
    
    
    size_t np=block->nodes.size(), wp=block->ways.size(), rp=block->relations.size(), gp=block->geometries.size();
    size_t ct=0;
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_data{1, [](const std::string& d, size_t p, size_t l) { throw std::domain_error("can't handle non-dense nodes"); }},
        handle_pbf_data{2, [&block,&objflags](const std::string& d, size_t p, size_t l) {
            block->has_nodes=true;
            if (objflags&1) { read_minimal_dense(block,d,p,l); }
        }},
        handle_pbf_data{3, [&block,&objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&2) { read_minimal_way(block,d,p,l); }
        }},
        handle_pbf_data{4, [&block,&objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&4) { read_minimal_relation(block,d,p,l); }
        }},
        handle_pbf_data_mt{20,24,[&block,&objflags](uint64 tg, const std::string& d, size_t p, size_t l) {
            if (objflags&8) { read_minimal_geometry(block, tg-17, d,p,l); }
        }},
        handle_pbf_value{10, [&ct](uint64 vl) { ct=vl; }}
        
    );
    
    if (ct!=0) {
        set_changetype(block->nodes, np, ct);
        set_changetype(block->ways, wp, ct);
        set_changetype(block->relations, rp, ct);
        set_changetype(block->geometries, gp, ct);
    }
               
}
    
minimal::BlockPtr read_minimal_block(int64 index, const std::string& data, size_t objflags) {

    auto result = std::make_shared<minimal::Block>();
    result->index = index;
    
    read_pbf_messages(data, 0, data.size(), 
        handle_pbf_data{2, [&result,objflags](const std::string& d, size_t s, size_t e) { read_minimal_group(result,d,objflags,s,e); }},
        handle_pbf_data{31, [&result](const std::string& d, size_t s, size_t e) { result->quadtree=read_quadtree(d.substr(s,e-s)); }},
        handle_pbf_value{32, [&result](uint64 vl) { result->quadtree = un_zig_zag(vl); }}
    );
    
    result->uncompressed_size = data.size();
    result->file_progress=0;
    return result;
}
    
    
void read_quadtree_vector_object(std::shared_ptr<quadtree_vector>& block, size_t off, const std::string& data, size_t pos, size_t lim) {
    
    int64 id=off<<61;
    int64 qt=-1;
    
    read_pbf_messages(data, pos, lim,
        handle_pbf_value{1, [&id](uint64 v) { id |= v; }},
        handle_pbf_value{20, [&qt](uint64 v) { qt = un_zig_zag(v); }}
    );
    
    block->push_back(std::make_pair(id,qt));
}

void read_quadtree_vector_dense(std::shared_ptr<quadtree_vector>& block, const std::string& data, size_t pos, size_t lim) {
    size_t firstidx = block->size();
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_packed_int_delta{1,
            [&block, firstidx](size_t sz) { block->resize(firstidx+sz,std::make_pair(-1,-1)); },
            [&block, firstidx](size_t i, int64 v) { block->at(firstidx+i).first=v; }
        },
        handle_pbf_packed_int_delta{20,nullptr,[&block, firstidx](size_t i, int64 v) { block->at(firstidx+i).second=v; }}
    );
}


void read_quadtree_vector_group(std::shared_ptr<quadtree_vector>& block, size_t objflags, const std::string& data, size_t pos, size_t lim) {
    
    block->reserve(8000);
    read_pbf_messages(data,pos,lim,
        handle_pbf_data{1, [&block,objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&1) { read_quadtree_vector_object(block, 0, d, p, l); }
        }},
        
        handle_pbf_data{2, [&block, objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&1) { read_quadtree_vector_dense(block, d, p, l); }
        }},
        handle_pbf_data{3, [&block,objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&2) { read_quadtree_vector_object(block, 1, d, p, l); }
        }},
        handle_pbf_data{4, [&block,objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&4) { read_quadtree_vector_object(block, 2, d, p, l); }
        }},
        handle_pbf_data_mt{20,24, [&block,objflags](size_t tg, const std::string& d, size_t p, size_t l) {
            if (objflags&8) { read_quadtree_vector_object(block, tg-17, d, p, l); }
        }}
    );
}
        
            
    


std::shared_ptr<quadtree_vector> read_quadtree_vector_block(const std::string& data, size_t objflags) {

    auto result = std::make_shared<quadtree_vector>();

    read_pbf_messages(data, 0, data.size(),
        handle_pbf_data{2, [&result, objflags](const std::string& d, size_t p, size_t l) { read_quadtree_vector_group(result, objflags,d,p,l); }}
    );
   
    return result;
}








}
