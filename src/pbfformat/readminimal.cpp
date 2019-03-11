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
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/pbf/protobuf_messages.hpp"

#include "oqt/utils/logger.hpp"
#include <iostream>
#include <iterator>
#include <iomanip>

namespace oqt {


void readMinimalGroup(minimal::BlockPtr block, const std::string& group, size_t objflags);

void readMinimalBlock_alt(minimal::BlockPtr block, const std::string& data, size_t objflags);

minimal::BlockPtr read_minimal_block(int64 index, const std::string& data, size_t objflags) {

    auto result = std::make_shared<minimal::Block>();
    result->index = index;
    
    if ((objflags&32)==0) {
        readMinimalBlock_alt(result, data, objflags);
    } else {

        size_t pos=0;
        PbfTag tag = std::move(read_pbf_tag(data,pos));
        for ( ; tag.tag>0; tag=std::move(read_pbf_tag(data,pos))) {
            if (tag.tag == 2) {
                readMinimalGroup(result, tag.data,objflags);
            } else if (tag.tag==31) {
                result->quadtree = read_quadtree(tag.data);
            } else if (tag.tag == 32) {
                result->quadtree = un_zig_zag(tag.value);
            }
        }
    }
    
    result->uncompressed_size = data.size();
    result->file_progress=0;
    return result;
}



template <class T>
void readMinimalInfo(const std::string& data, T& obj) {
    size_t pos=0;
    PbfTag tag = read_pbf_tag(data,pos);

    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) {
            obj.version = tag.value;
        } else if (tag.tag==2) {
            obj.timestamp = tag.value;
        }
    }
    return;
}

template <class T>
std::tuple<PbfTag,PbfTag,PbfTag> readMinimalCommon(const std::string& data, T& obj, bool skipinfo) {

    PbfTag other1,other2,other3;

    size_t pos=0;
    PbfTag tag = read_pbf_tag(data,pos);

    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) {
            obj.id=int64(tag.value);
        } else if ((tag.tag==4) && (!skipinfo)) {
            readMinimalInfo(tag.data, obj);
        } else if (tag.tag==8) {
            other1=std::move(tag);
        } else if (tag.tag==9) {
            other2=std::move(tag);
        } else if (tag.tag==10) {
            other3=std::move(tag);

        } else if (tag.tag==20) {
            obj.quadtree=un_zig_zag(tag.value);
        }
    }

    return std::make_tuple(std::move(other1),std::move(other2),std::move(other3));
}

void readMinimalNode(minimal::BlockPtr block, const std::string& data, bool skipinfo) {
    minimal::Node node;
    PbfTag rem1,rem2,rem3;
    std::tie(rem1,rem2,rem3) = readMinimalCommon(data, node, skipinfo);
    
    if (rem1.tag==8) {
        node.lat=un_zig_zag(rem1.value);
    }
    if (rem2.tag==9) {
        node.lon=un_zig_zag(rem2.value);
    }
    block->nodes.push_back(std::move(node));
}

void readMinimalWay(minimal::BlockPtr block, const std::string& data, bool skipinfo) {
    minimal::Way way;
    PbfTag rem1,rem2,rem3;
    std::tie(rem1,rem2,rem3) = std::move(readMinimalCommon(data,way, skipinfo));
    if (rem1.tag==8) {
        
        way.refs_data=std::move(rem1.data);
    }
    
    block->ways.push_back(std::move(way));
}

void readMinimalRelation(minimal::BlockPtr block, const std::string& data, bool skipinfo) {
    minimal::Relation rel;
    PbfTag rem1,rem2,rem3;

    std::tie(rem1,rem2,rem3) = std::move(readMinimalCommon(data, rel, skipinfo));
    
    if (rem2.tag==9) {
        
        rel.refs_data=std::move(rem2.data);
    }
    if (rem3.tag==10) {
        
        rel.tys_data=std::move(rem3.data);
    }
    
    block->relations.push_back(std::move(rel));
}

void readMinimalGeometry(minimal::BlockPtr block, const std::string& data, size_t ty) {
    minimal::Geometry geom;
    //geom.ty=ty;
    PbfTag rem1,rem2,rem3;
    std::tie(rem1,rem2,rem3) = readMinimalCommon(data,geom, false);
    geom.ty = ty;
    block->geometries.push_back(std::move(geom));
}


    
    
    
void readMinimalDense(minimal::BlockPtr block, const std::string& data, bool skipinfo) {
    
    std::string id,version,timestamp,quadtree,lon,lat;
    

    size_t pos=0;
    PbfTag tag = read_pbf_tag(data,pos);

    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) {
            //id = readPackedDelta(tag.data);
            id=std::move(tag.data);
        } else if ((tag.tag==5) && (!skipinfo)) {
            //timestamp = [&tag]{
                size_t p2=0;
                PbfTag t2=read_pbf_tag(tag.data,p2);
                for ( ; t2.tag>0; t2=read_pbf_tag(tag.data,p2)) {
                    if (t2.tag==1) {
                        version=std::move(t2.data);
                    }else if (t2.tag==2) {
                        //return readPackedDelta(t2.data);
                        timestamp= std::move(t2.data);
                    }
                }
                //return std::vector<int64>();
                //return std::string("");
            //}();
        } else if (tag.tag==8) {
            //lat = readPackedDelta(tag.data);
            lat=std::move(tag.data);
        } else if (tag.tag==9) {
            //lon = readPackedDelta(tag.data);
            lon=std::move(tag.data);
        } else if (tag.tag==20) {
            //quadtree = readPackedDelta(tag.data);
            quadtree=std::move(tag.data);
        }
    }

    //nodes.reserve(id.size());

    size_t p0=0,p1=0,p2=0,p3=0,p4=0,p5=0;
    int64 c0=0,c1=0,c2=0,c3=0,c4=0,c5=0;

    block->nodes.reserve(block->nodes.size()+id.size());


    while ( p0 < id.size()) {
        c0 += read_varint(id,p0);
        if (p1 < timestamp.size()) {
            c1+=read_varint(timestamp,p1);
        }
        if (p2 < quadtree.size()) {
            c2+=read_varint(quadtree,p2);
        }
        if (p3 < lon.size()) {
            c3+=read_varint(lon,p3);
        }
        if (p4 < lat.size()) {
            c4+=read_varint(lat,p4);
        }
        if (p5< version.size()) {
            c5 = read_unsigned_varint(version,p5);
        }
        //nodes.push_back(minimalnode{c0,c1,c2,c3,c4});
        minimal::Node mn; mn.id = c0; mn.timestamp=c1; mn.quadtree=c2; mn.changetype=0; mn.lon=c3; mn.lat=c4;
        mn.version=c5;
        block->nodes.push_back(std::move(mn));
    }

    //std::copy(nodes.begin(), nodes.end(), std::back_inserter(block->nodes));



}

void readMinimalGroup(minimal::BlockPtr block, const std::string& data, size_t objflags) {
    size_t pos=0;
    PbfTag tag = std::move(read_pbf_tag(data,pos));
    size_t np = block->nodes.size();
    size_t wp = block->ways.size();
    size_t rp = block->relations.size();
    
    for ( ; tag.tag>0; tag=std::move(read_pbf_tag(data,pos))) {
        if ((tag.tag==1) || (tag.tag==2)) { block->has_nodes=true; }
        
        if ((objflags&1) && (tag.tag==1)) {
            readMinimalNode(block, tag.data, objflags&16);
        } else if ((objflags&1) &&(tag.tag==2)) {
            readMinimalDense(block, tag.data, objflags&16);
        } else if ((objflags&2) && (tag.tag==3)) {
            readMinimalWay(block, tag.data, objflags&16);
        } else if ((objflags&4) && (tag.tag==4)) {
            readMinimalRelation(block, tag.data, objflags&16);
        } else if ((objflags&8) && (tag.tag>=20) && (tag.tag < 24)) {
            readMinimalGeometry(block,tag.data, tag.tag-17);
        } else if (tag.tag==10) {
            if (block->nodes.size()>np) {
                for (size_t i=np; i < block->nodes.size(); i++) {
                    block->nodes[i].changetype=tag.value;
                }
            }
            if (block->ways.size()>wp) {
                for (size_t i=wp; i < block->ways.size(); i++) {
                    block->ways[i].changetype=tag.value;
                }
            }
            if (block->relations.size()>rp) {
                for (size_t i=rp; i < block->relations.size(); i++) {
                    block->relations[i].changetype=tag.value;
                }
            }
        }
    }

    return;
}



void readQtVecObj(std::shared_ptr<quadtree_vector> block, const std::string& data, int64 off) {

    int64 id=off<<61;
    int64 qt=-1;

    size_t pos=0;
    PbfTag tag=read_pbf_tag(data,pos);
    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) {
            id |= int64(tag.value);
        } else if (tag.tag==20) {
            qt = un_zig_zag(tag.value);
        }
    }
    block->push_back(std::make_pair(id,qt));
}

void readQtVecDense(std::shared_ptr<quadtree_vector> block, const std::string& data) {
    std::string ids, qts;

    size_t pos=0;
    PbfTag tag=read_pbf_tag(data,pos);
    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag==1) {
            ids = tag.data;
        } else if (tag.tag==20) {
            qts = tag.data;
        }
    }

    int64 id=0, qt=0;
    size_t idp=0,qtp=0;

    while (idp < ids.size()) {
        id += read_varint(ids, idp);
        if (qtp < qts.size()) {
            qt += read_varint(qts,qtp);
            block->push_back(std::make_pair(id,qt));
        } else {
            block->push_back(std::make_pair(id,-1));
        }
    }
}


void readQtsVecGroup(std::shared_ptr<quadtree_vector> block, const std::string& data, size_t objflags) {

    block->reserve(8000);

    size_t pos=0;
    PbfTag tag = read_pbf_tag(data,pos);

    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if ((objflags&1) && (tag.tag==1)) {
            readQtVecObj(block, tag.data, 0);
        } else if ((objflags&1) &&(tag.tag==2)) {
            readQtVecDense(block, tag.data);
        } else if ((objflags&2) && (tag.tag==3)) {
            readQtVecObj(block, tag.data, 1);
        } else if ((objflags&4) && (tag.tag==4)) {
            readQtVecObj(block, tag.data, 2);
        } else if ((objflags&8) && (tag.tag>=20)) {
            readQtVecObj(block, tag.data, tag.tag-17);
        }
    }

    return;
}

std::shared_ptr<quadtree_vector> read_quadtree_vector_block(const std::string& data, size_t objflags) {

    auto result = std::make_shared<quadtree_vector>();

    size_t pos=0;
    PbfTag tag = read_pbf_tag(data,pos);
    for ( ; tag.tag>0; tag=read_pbf_tag(data,pos)) {
        if (tag.tag == 2) {
            readQtsVecGroup(result, tag.data,objflags);
        }
    }

    return result;
}






std::tuple<uint64,uint64,bool> read_pbf_tag_alt(const std::string& data, size_t& pos) {
    if (pos>=data.size()) { return std::make_tuple(0,0,false); }
    uint64 a = read_unsigned_varint(data,pos);
    uint64 b = read_unsigned_varint(data,pos);
    if ((a&7) == 0) {
        return std::make_tuple(a>>3,b,false);
    } else if ((a&7) == 2) {
        return std::make_tuple(a>>3,b,true);
    }
    throw std::domain_error("wtf");
    return std::make_tuple(0,0,false);
}

size_t numPacked(const std::string& data, size_t pos, size_t len) {
    size_t c=0;
    size_t pp = pos;
    
    
    while (pp < (pos+len)) {
        read_unsigned_varint(data,pp);
        c+=1;
    }
    if (pp != (pos+len)) {
        Logger::Message() << "?? readPackedDelta_altcb " << pos << ", " << len << ", @ " << pp;
        throw std::domain_error("wtf");
    }
    return c;
}
    
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
}
    


void readDenseInfo_alt(minimal::BlockPtr& block, size_t firstnd, const std::string& data, size_t pos, size_t lim) {
    read_pbf_messages(data,pos,lim, 
        handle_pbf_packed_int{1, nullptr, [&block,&firstnd](size_t i, uint64 v) { block->nodes.at(firstnd+i).version = v; }},
        handle_pbf_packed_int_delta{2, nullptr, [&block,&firstnd](size_t i, int64 v) { block->nodes.at(firstnd+i).timestamp = v; }}
    );
    
    
    /*
size_t readDenseInfo_alt(minimal::BlockPtr& block, size_t firstnd, const std::string& data, size_t posin, size_t len) {    
    size_t pos=posin;
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    while (pos < (posin+len)) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
        if (isdata && (tg == 1)) { //version
            
            pos = readPacked_altcb(data, pos, vl, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).version = v; });
            
        } else if (isdata && (tg==2)) { //timestamp
            
            pos = readPackedDelta_altcb(data, pos, vl, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).timestamp = v; });
        } else if (isdata) {
            
            pos += vl;
        } 
        
    }
    if (pos != (posin+len)) {
        Logger::Message() << "?? readDenseInfo_alt " << posin << ", " << len << ", @ " << pos;
        throw std::domain_error("wtf");
    }
    return pos;*/
}
            

void readMinimalDense_alt(minimal::BlockPtr& block, const std::string& data, size_t pos, size_t lim) {
    size_t firstnd = block->nodes.size();
    
    
    read_pbf_messages(data,pos,lim,
        handle_pbf_data{1, [&block,&firstnd](const std::string& d, size_t p, size_t l) {
            size_t num_ids = numPacked(d,p,l-p);
            block->nodes.resize(firstnd+num_ids);
            readPackedDelta_altcb(d,p,l-p,[&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).id = v; });
        }},
        handle_pbf_data{5, [&block,&firstnd](const std::string& d, size_t p, size_t l) {
            readDenseInfo_alt(block,firstnd,d,p,l);
        }},
        handle_pbf_data{8, [&block,&firstnd](const std::string& d, size_t p, size_t l) {
            readPackedDelta_altcb(d,p,l-p, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).lat = v; });
        }},
        handle_pbf_data{9, [&block,&firstnd](const std::string& d, size_t p, size_t l) {
            readPackedDelta_altcb(d,p,l-p, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).lon = v; });
        }},
        handle_pbf_data{20, [&block,&firstnd](const std::string& d, size_t p, size_t l) {
            readPackedDelta_altcb(d,p,l-p, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).quadtree = v; });
        }}
    );
    /*
    
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    if ((tg != 1) || !isdata) {
        throw std::domain_error("ids should be first");
    }
    size_t num_ids = numPacked(data,pos,vl);
    
    block->nodes.resize(firstnd + num_ids);
    
    pos = readPackedDelta_altcb(data, pos, vl, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).id = v; });
    
    while (pos < lim) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    
        if (isdata && (tg==5)) {
            pos = readDenseInfo_alt(block,firstnd,data,pos,vl);
        } else if (isdata && (tg == 8)) {
            pos = readPackedDelta_altcb(data, pos, vl, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).lat = v; });
            
        } else if (isdata && (tg==9)) {
            pos = readPackedDelta_altcb(data, pos, vl, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).lon = v; });
           
        } else if (isdata && (tg==20)) {
            pos = readPackedDelta_altcb(data, pos, vl, [&block,&firstnd](const size_t& i, const int64& v) { block->nodes.at(firstnd+i).quadtree = v; });
            
        } else if (isdata) {
            pos += vl;
        }
    }
    */
}

template <class T>
void/*size_t*/ readMinimalInfo_alt(T& obj, const std::string& data, size_t pos, size_t lim) {
    
    /*
    read_pbf_messages(data,pos,lim,
        handle_pbf_value{1,[&obj](uint64 v) { obj.version=v; }},
        handle_pbf_value{2,[&obj](uint64 v) { obj.timestamp=v; }}
    );
    
    */
    
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    while (pos < lim) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    
        if (!isdata && (tg==1)) {
            obj.version = vl;
        } else if (!isdata && (tg==2)) {
            obj.timestamp = vl;
        } else if (isdata) {
            pos += vl;
        }
    }
    if (pos != lim) { throw std::domain_error("wtf"); }
    //return pos;*/
}
    

void readMinimalWay_alt(minimal::BlockPtr& block, const std::string& data, size_t pos, size_t lim) {
    //if (block->ways.capacity() == block->ways.size()) { block->ways.reserve(block->ways.size()+1000); }
    
    minimal::Way w;
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&w](uint64 v) { w.id=v; }},
        handle_pbf_data{4, [&w](const std::string& d, size_t p, size_t l) { readMinimalInfo_alt(w, d, p, l); }},
        handle_pbf_data{8, [&w](const std::string& d, size_t p, size_t l) { w.refs_data=d.substr(p,l-p); }},
        handle_pbf_value{20, [&w](uint64 v) { w.quadtree = un_zig_zag(v); }}
    );
    /*if (w.timestamp<1100000000) {
        read_pbf_messages(data,pos,lim,
            handle_pbf_data{4,[](const std::string& d, size_t p, size_t l) {
                read_pbf_messagesxx(d,p,l,
                    handle_pbf_value{1,[](uint64 v) { std::cout << "vers=" << v; }},
                    handle_pbf_value{2,[](uint64 v) { std::cout << " timestamp=" << v; }}
                );
            }}
        );
        std::cout << "  [" << block->ways.size() << "] ?? way " << w.id << " " << w.version << " " << w.timestamp << std::endl;
    }*/
    
    block->ways.emplace_back(std::move(w));
    /*
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    
    while (pos < lim) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    
        if (!isdata && (tg==1)) {
            
            w.id = vl;
        } else if (isdata && (tg==4)) {
          pos = readMinimalInfo_alt(w, data, pos, pos+vl);
        } else if (isdata && (tg==8)) {
            
            w.refs_data=std::move(data.substr(pos,vl));
            pos+=vl;
        } else if (tg==20) {
            w.quadtree = un_zig_zag(vl);
        } else if (isdata) {
            pos+=vl;
        }
        
    }
    
    block->ways.push_back(std::move(w));*/
}
void readMinimalRelation_alt(minimal::BlockPtr& block, const std::string& data, size_t pos, size_t lim) {
    //if (block->relations.capacity() == block->relations.size()) { block->relations.reserve(block->relations.size()+1000); }
    
    minimal::Relation r;
    
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&r](uint64 v) { r.id=v; }},
        handle_pbf_data{4, [&r](const std::string& d, size_t p, size_t l) { readMinimalInfo_alt(r, d, p, l); }},
        handle_pbf_data{9, [&r](const std::string& d, size_t p, size_t l) { r.refs_data=d.substr(p,l-p); }},
        handle_pbf_data{10, [&r](const std::string& d, size_t p, size_t l) { r.tys_data=d.substr(p,l-p); }},
        handle_pbf_value{20, [&r](uint64 v) { r.quadtree = un_zig_zag(v); }}
    );
    block->relations.emplace_back(std::move(r));
    
    
    /*
    
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    while (pos < lim) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    
        if (!isdata && (tg==1)) {
            r.id = vl;
        } else if (isdata && (tg==4)) {
          pos = readMinimalInfo_alt(r, data, pos, pos+vl);
        } else if (isdata && (tg==9)) {
            r.refs_data=std::move(data.substr(pos,vl));
            pos+=vl;
        } else if (tg==10) {
            r.tys_data=std::move(data.substr(pos,vl));
            pos+=vl;
        } else if (tg==20) {
            r.quadtree = un_zig_zag(vl);
        } else if (isdata) {
            pos+=vl;
        }
    }
    block->relations.push_back(std::move(r));*/
}
void readMinimalGeometry_alt(minimal::BlockPtr& block, size_t gm_type, const std::string& data, size_t pos, size_t lim) {
    //if (block->relations.capacity() == block->relations.size()) { block->relations.reserve(block->relations.size()+1000); }
    
    minimal::Geometry r;
    r.ty = gm_type;
    
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&r](uint64 v) { r.id=v; }},
        handle_pbf_data{4, [&r](const std::string& d, size_t p, size_t l) { readMinimalInfo_alt(r, d, p, l); }},
        handle_pbf_value{20, [&r](uint64 v) { r.quadtree = un_zig_zag(v); }}
    );
    block->geometries.emplace_back(std::move(r));
    
    /*
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    while (pos < lim) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    
        if (!isdata && (tg==1)) {
            r.id = vl;
        } else if (isdata && (tg==4)) {
          pos = readMinimalInfo_alt(r, data, pos, pos+vl);
        } else if (tg==20) {
            r.quadtree = un_zig_zag(vl);
        } else if (isdata) {
            pos+=vl;
        }
    }
    block->geometries.push_back(std::move(r));*/
}


void readMinimalGroup_alt(minimal::BlockPtr& block, const std::string& data, size_t objflags, size_t pos, size_t lim) {
    
    
    
    size_t np=block->nodes.size(), wp=block->ways.size(), rp=block->relations.size(), gp=block->geometries.size();
    size_t ct=0;
    
    
    
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_data{1, [](const std::string& d, size_t p, size_t l) { throw std::domain_error("can't handle non-dense nodes"); }},
        handle_pbf_data{2, [&block,&objflags](const std::string& d, size_t p, size_t l) {
            block->has_nodes=true;
            if (objflags&1) { readMinimalDense_alt(block,d,p,l); }
        }},
        handle_pbf_data{3, [&block,&objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&2) { readMinimalWay_alt(block,d,p,l); }
        }},
        handle_pbf_data{4, [&block,&objflags](const std::string& d, size_t p, size_t l) {
            if (objflags&4) { readMinimalRelation_alt(block,d,p,l); }
        }},
        handle_pbf_data_mt{20,24,[&block,&objflags](uint64 tg, const std::string& d, size_t p, size_t l) {
            if (objflags&8) { readMinimalGeometry_alt(block, tg-17, d,p,l); }
        }},
        handle_pbf_value{10, [&ct](uint64 vl) { ct=vl; }}
        
    );
    
    /*
    uint64 tg=0, vl=0;
    bool isdata=false;
    while (pos < lim) {
        std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
    
        if (isdata && (tg==2)) { block->has_nodes=true; }
        
        if (isdata && (tg==1) && (objflags&1)) {
            throw std::domain_error("can't handle non-dense nodes");
        } else if (isdata && (tg == 2) && (objflags&1)) {
            readMinimalDense_alt(block, data,pos,pos+vl);
            pos+=vl;
        } else if (isdata && (tg==3) && (objflags&2)) {
            readMinimalWay_alt(block,data,pos,pos+vl);
            pos+=vl;
        } else if (isdata && (tg==4) && (objflags&4)) {
            readMinimalRelation_alt(block, data, pos, pos+vl);
            pos+=vl;
        } else if (isdata && (objflags&8) && (tg>=20) && (tg < 24)) {
            readMinimalGeometry_alt(block, tg-17, data, pos, pos+vl);
            pos+=vl;
        } else if (tg==10) {
            ct=vl;
        } else if (isdata) {
            pos+=vl;
        }
    }
    */
    
    if (ct!=0) {
        if (block->nodes.size()>np) {
            for (size_t i=np; i < block->nodes.size(); i++) {
                block->nodes[i].changetype=ct;
            }
        }
        if (block->ways.size()>wp) {
            for (size_t i=wp; i < block->ways.size(); i++) {
                block->ways[i].changetype=ct;
            }
        }
        if (block->relations.size()>rp) {
            for (size_t i=rp; i < block->relations.size(); i++) {
                block->relations[i].changetype=ct;
            }
        }
        if (block->geometries.size()>gp) {
            for (size_t i=gp; i < block->geometries.size(); i++) {
                block->geometries[i].changetype=ct;
            }
        }
    }
               
}
    

void readMinimalBlock_alt(minimal::BlockPtr block, const std::string& data, size_t objflags) {
    size_t pos=0;
    size_t lim=data.size();
    
    /*
    uint64 tg=0, vl=0;
    bool isdata=false;
    
    
    try {
        while (pos < lim) {
            std::tie(tg,vl,isdata) = read_pbf_tag_alt(data,pos);
        
            if (isdata && (tg == 2)) {
                //Logger::Message() << "call readMinimalGroup_alt " << pos << " to " << pos+vl << " / " << data.size();
                readMinimalGroup_alt(block, data, objflags, pos, pos+vl);
                pos+=vl;
            } else if (tg==31) {
                block->quadtree = read_quadtree(data.substr(pos,vl));
                pos+=vl;
            } else if (tg == 32) {
                block->quadtree = un_zig_zag(vl);
            } else if (isdata) {
                pos+=vl;
            }
        }
    
    } catch (const std::out_of_range& e) {
        Logger::Message() << "?? " << block->index << " " << pos << "/" << lim << " [" << data.size() << "] {" << tg << ", " << vl << ", " << isdata << "}";
    }
    */
     
    handle_data_func f1 = [&block,objflags](const std::string& d, size_t s, size_t e) { readMinimalGroup_alt(block,d,objflags,s,e); };
    handle_data_func f2 = [&block](const std::string& d, size_t s, size_t e) { block->quadtree=read_quadtree(d.substr(s,e-s)); };
    handle_value_func f3 = [&block](uint64 vl) { block->quadtree = un_zig_zag(vl); };
    
       
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_data{2, f1},
        handle_pbf_data{31, f2},
        handle_pbf_value{32, f3}
    );
    
    
}








}
