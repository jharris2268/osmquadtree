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
typedef std::function<void(uint64, const std::string&, size_t, size_t)> handle_data_mt_func;
/*
class handle_t {
    public:
        virtual bool operator()(uint64 tg, const std::string& data, size_t& pos)=0;
};

class handle_end : public handle_t {
    
    
    public:
        handle_end() {}
    
        virtual bool operator()(uint64 tg, const std::string& data, size_t& pos){
            if ((tg&7) == 2) {
                uint64 ln = read_unsigned_varint(data,pos);
                pos += ln;
                return true;
            }
            return false;
        }
};
*/
void handle_end_f(uint64 tg, const std::string& data, size_t& pos) {
    if ((tg&7) == 2) {
        uint64 ln = read_unsigned_varint(data,pos);
        pos += ln;
    } else {
        read_unsigned_varint(data,pos);
    }
}



//class handle_pbf_value : public handle_t {
struct handle_pbf_value {
    uint64 tag;
    handle_value_func call;
    /*public:
        handle_pbf_value(uint64 tag_, const handle_value_func& call_) 
            : tag(tag_), call(call_) {}*/
    
        /*virtual*/ bool operator()(uint64 tg, const std::string& data, size_t& pos) {
            
            if ( ((tg >> 3) == tag) && ((tg&7) == 0)) {
            
                uint64 v = read_unsigned_varint(data,pos);
                if (call) {
                    call(v);
                }
                return true;
            }
            return false;
        }
};

//template <class NextHandle>
//class handle_pbf_data : public handle_t {
struct handle_pbf_data {
    uint64 tag;
    handle_data_func call;
    
    
    /*public:
        handle_pbf_data(uint64 tag_, const handle_data_func& call_) 
            : tag(tag_), call(call_) {}
    
        virtual*/ bool operator()(uint64 tg, const std::string& data, size_t& pos) {
        
            if ( ((tg >> 3) == tag) && ((tg&7) == 2)) {
            
                uint64 ln = read_unsigned_varint(data,pos);
                if (call) {
                    call(data,pos,pos+ln);
                }
                pos+=ln;
                return true;
            }
            return false;
        }
};

struct handle_pbf_data_mt {
    uint64 min_tag;
    uint64 max_tag;
    handle_data_mt_func call;
    
    
        bool operator()(uint64 tg, const std::string& data, size_t& pos) {
        
            if ( ((tg >> 3) >= min_tag) && ((tg >> 3) < max_tag) && ((tg&7) == 2))  {
            
                uint64 ln = read_unsigned_varint(data,pos);
                if (call) {
                    call(tg>>3, data,pos,pos+ln);
                }
                pos+=ln;
                return true;
            }
            return false;
        }
};



typedef std::function<void(size_t)> handle_packed_int_size_call;
typedef std::function<void(size_t, int64)> handle_packed_int_delta_call;
typedef std::function<void(size_t, uint64)> handle_packed_int_call;

    /*
size_t readPacked_altcb(const std::string& data, size_t pos, size_t len, std::function<void(size_t i, uint64 v)> cb) {
    size_t pp = pos;
    
    size_t i=0;
    while (pp < (pos+len)) {
        uint64 v = read_unsigned_varint(data,pp);
        cb(i, v);
        i+=1;
    }
    
    if (pp != (pos+len)) {
        Logger::Message() << "?? readPackedDelta_altcb " << pos << ", " << len << ", @ " << pp;
        throw std::domain_error("wtf");
    }
    return pp;
}

size_t readPackedDelta_altcb(const std::string& data, size_t pos, size_t len, std::function<void(size_t i, int64 v)> cb) {
    size_t pp = pos;
    int64 v=0;
    size_t i=0;
    while (pp < (pos+len)) {
        v += read_varint(data,pp);
        cb(i, v);
        i+=1;
    }
    
    if (pp != (pos+len)) {
        Logger::Message() << "?? readPackedDelta_altcb " << pos << ", " << len << ", @ " << pp;
        throw std::domain_error("wtf");
    }
    return pp;
}*/
    

struct handle_pbf_packed_int {
    uint64 tag;
    handle_packed_int_size_call size_call;
    handle_packed_int_call call;
    
    bool operator()(uint64 tg, const std::string& data, size_t& pos) {
        if ( ((tg >> 3) == tag) && ((tg&7) == 2)) {
            uint64 ln = read_unsigned_varint(data,pos);
            
            if (size_call) {
                size_t sz=0;
                size_t pp=pos;
                while (pp < (pos+ln)) {
                    read_unsigned_varint(data,pp);
                    sz++;
                }
                size_call(sz);
            }
            if (call) {
                size_t pp=pos;
                
                size_t i=0;
                while (pp < (pos+ln)) {
                    uint64 v = read_unsigned_varint(data,pp);
                    call(i, v);
                    i++;
                }
            }
            
            pos += ln;
            return true;
            
        } else {
            return false;
        }
    }
};
       

struct handle_pbf_packed_int_delta {
    uint64 tag;
    handle_packed_int_size_call size_call;
    handle_packed_int_delta_call call;
    
    bool operator()(uint64 tg, const std::string& data, size_t& pos) {
        if ( ((tg >> 3) == tag) && ((tg&7) == 2)) {
            uint64 ln = read_unsigned_varint(data,pos);
            
            if (size_call) {
                size_t sz=0;
                size_t pp=pos;
                while (pp < (pos+ln)) {
                    read_unsigned_varint(data,pp);
                    sz++;
                }
                size_call(sz);
            }
            if (call) {
                size_t pp=pos;
                int64 v=0;
                size_t i=0;
                while (pp < (pos+ln)) {
                    v += read_varint(data,pp);
                    call(i, v);
                    i++;
                }
            }
            
            pos += ln;
            return true;
            
        } else {
            return false;
        }
    }
};
                
            
            

template <class H1>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }
    }
}

template <class H1, class H2>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1, H2 h2) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else if (h2(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }       
    }
}

template <class H1, class H2, class H3>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1, H2 h2, H3 h3) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else if (h2(tg,data,pos)) {}
        else if (h3(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }
    }
}

template <class H1, class H2, class H3, class H4>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1, H2 h2, H3 h3, H4 h4) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else if (h2(tg,data,pos)) {}
        else if (h3(tg,data,pos)) {}
        else if (h4(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }
        
    }
}
template <class H1, class H2, class H3, class H4, class H5>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1, H2 h2, H3 h3, H4 h4, H5 h5) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else if (h2(tg,data,pos)) {}
        else if (h3(tg,data,pos)) {}
        else if (h4(tg,data,pos)) {}
        else if (h5(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }
    }
}
template <class H1, class H2, class H3, class H4, class H5, class H6>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1, H2 h2, H3 h3, H4 h4, H5 h5, H6 h6) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else if (h2(tg,data,pos)) {}
        else if (h3(tg,data,pos)) {}
        else if (h4(tg,data,pos)) {}
        else if (h5(tg,data,pos)) {}
        else if (h6(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }
    }
}

template <class H1, class H2, class H3, class H4, class H5, class H6, class H7>
void read_pbf_messages(const std::string& data, size_t pos, size_t end, H1 h1, H2 h2, H3 h3, H4 h4, H5 h5, H6 h6, H7 h7) {
    
    while (pos < end) {
        
        uint64 tg = read_unsigned_varint(data, pos);
        if (h1(tg,data,pos)) {}
        else if (h2(tg,data,pos)) {}
        else if (h3(tg,data,pos)) {}
        else if (h4(tg,data,pos)) {}
        else if (h5(tg,data,pos)) {}
        else if (h6(tg,data,pos)) {}
        else if (h7(tg,data,pos)) {}
        else { handle_end_f(tg,data,pos); }
    }
}

    



    
}

#endif //PBFFORMAT_PROTOBUF_HPP
