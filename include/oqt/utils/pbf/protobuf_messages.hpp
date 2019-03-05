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

#ifndef UTILS_PBF_PROTOBUF_MESSAGES_HPP
#define UTILS_PBF_PROTOBUF_MESSAGES_HPP


#include "oqt/utils/pbf/protobuf.hpp"

namespace oqt {




typedef std::function<void(uint64, const std::string&, size_t&)> handle_func;
typedef std::function<void(uint64)> handle_value_func;
typedef std::function<void(const std::string&, size_t, size_t)> handle_data_func;


struct handle_end {
    void operator()(uint64 tg, const std::string& data, size_t& pos) {
        if ((tg&7) == 2) {
            uint64 ln = read_unsigned_varint(data,pos);
            pos += ln;
        }
    }
};


//template <class NextHandle>
struct handle_pbf_value {
    uint64 tag;
    handle_value_func& call;
    handle_func& next;
    
    void operator()(uint64 tg, const std::string& data, size_t& pos) {
        
        if ( ((tg >> 3) == tag) && (tg&7) == 0) {
        
            uint64 v = read_unsigned_varint(data,pos);
            call(v);
        } else {
            next(tg, data, pos);
        }
    }
};

//template <class NextHandle>
struct handle_pbf_data {
    uint64 tag;
    handle_data_func& call;
    handle_func& next;
    
    void operator()(uint64 tg, const std::string& data, size_t& pos) {
        
        if ( ((tg >> 3) == tag) && (tg&7) == 2) {
        
            uint64 ln = read_unsigned_varint(data,pos);
            call(data,pos,pos+ln);
            pos+=ln;
        } else {
            next(tg, data,pos);
            
        }
    }
};

//template <class Handle>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, handle_func& handle) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        handle(tg, data, pos);
    }        
}
        
    



    
}

#endif //PBFFORMAT_PROTOBUF_HPP
