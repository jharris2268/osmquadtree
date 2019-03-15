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

#include "oqt/utils/pbf/protobuf_messages.hpp"

#include "oqt/pbfformat/idset.hpp"
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/elements/element.hpp"

#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/geometry.hpp"

#include "oqt/utils/logger.hpp"

#include "picojson.h"

#include <iostream>
#include <fstream>

namespace oqt {

namespace readblock_detail {

struct block_data {
    std::vector<std::string> stringtable;
    bool change;
    IdSetPtr ids;
    read_geometry_func readGeometry;
    bool skip_nodes;
    bool skip_ways;
    bool skip_relations;
    bool skip_geometries;
    bool skip_strings;
    bool skip_info;
    
};


struct node_data {
    int64 id, qt;
    ElementInfo info;
    std::vector<Tag> tags;
    
    int64 lon,lat;
};


struct way_data {
    int64 id, qt;
    ElementInfo info;    
    std::vector<Tag> tags;
    
    std::vector<int64> refs;
};

    
struct relation_data {
    int64 id, qt;
    ElementInfo info;
    
    std::vector<Tag> tags;
    
    std::vector<Member> members;
};

struct geometry_data {
    int64 id, qt;
    ElementInfo info;
    
    std::vector<Tag> tags;
    std::list<PbfTag> other_tags;
};
    

void read_info(ElementInfo& info, const block_data& block, const std::string& data, size_t pos, size_t lim) {

    read_pbf_messages(data,pos,lim,
        handle_pbf_value{1, [&info](uint64 vl) { info.version = vl; }},
        handle_pbf_value{2, [&info](uint64 vl) { info.timestamp = vl; }},
        handle_pbf_value{3, [&info](uint64 vl) { info.changeset = vl; }},
        handle_pbf_value{4, [&info](uint64 vl) { info.user_id = vl; }},
        handle_pbf_value{block.skip_strings ? 0ull: 5, [&info,&block](uint64 vl) { info.user = block.stringtable.at(vl); }},
        handle_pbf_value{6, [&info](uint64 vl) { info.visible = vl==1; }}
    );
    
}


ElementPtr read_node(const std::string& data, size_t pos, size_t lim, const block_data& block, changetype c) {
    
    node_data nd{0,0,{0,0,0,0,"",false},{},0,0};
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&nd](uint64 v) { nd.id=v; }},
        handle_pbf_packed_int{block.skip_strings ? 0ull: 2,
                [&nd](size_t s) { if (s>nd.tags.size()) { nd.tags.resize(s); } },
                [&nd,&block](size_t i, uint64 v) { nd.tags[i].key=block.stringtable.at(v); }
        },
        handle_pbf_packed_int{block.skip_strings ? 0ull: 3,
                [&nd](size_t s) { if (s>nd.tags.size()) { nd.tags.resize(s); } },
                [&nd,&block](size_t i, uint64 v) { nd.tags[i].val=block.stringtable.at(v); }
        },
        handle_pbf_data{block.skip_info ? 0ull: 4,[&nd,&block](const std::string& d, size_t p, size_t l) { read_info(nd.info, block, d, p, l); }},
        handle_pbf_value{8, [&nd](uint64 v) { nd.lat=un_zig_zag(v); }},
        handle_pbf_value{9, [&nd](uint64 v) { nd.lon=un_zig_zag(v); }},
        handle_pbf_value{20, [&nd](uint64 v) { nd.qt=un_zig_zag(v); }}
    );
    if (nd.id==0) { return nullptr; }
    
    return std::make_shared<Node>(c, nd.id, nd.qt, nd.info, nd.tags, nd.lon, nd.lat);
}
    
class dense_node_tags {
    public:
        dense_node_tags(std::vector<node_data>& nds_, const block_data& block_) : nds(nds_), block(block_), node_idx(0), tag_idx(0) {}
        
        void call(size_t i, uint64 v) {
            if (v==0) {
                node_idx++;
                tag_idx=0;
            } else {
                if ((tag_idx%2)==0) {
                    nds[node_idx].tags.push_back(Tag(block.stringtable.at(v),""));
                } else {
                    nds[node_idx].tags[tag_idx/2].val = block.stringtable.at(v);
                }
                tag_idx++;
            }
        }
    private:
        std::vector<node_data>& nds;
        const block_data& block;
        size_t node_idx;
        size_t tag_idx;
};
            
void read_dense_info(std::vector<node_data>& nds, const block_data& block, const std::string& data, size_t pos, size_t lim) {
    
    
    auto check_size = [&nds](size_t sz) {
        if (sz != nds.size()) { throw std::domain_error("unexpected dense info length"); }
    };
        
    read_pbf_messages(data,pos,lim,
        handle_pbf_packed_int{1, check_size, [&nds](size_t i, uint64 vl) { nds[i].info.version = vl; }},
        handle_pbf_packed_int_delta{2, check_size, [&nds](size_t i, int64 vl) { nds[i].info.timestamp = vl; }},
        handle_pbf_packed_int_delta{3, check_size, [&nds](size_t i, int64 vl) { nds[i].info.changeset = vl; }},
        handle_pbf_packed_int_delta{4, check_size, [&nds](size_t i, int64 vl) { nds[i].info.user_id = vl; }},
        handle_pbf_packed_int_delta{block.skip_strings ? 0ull: 5, check_size, [&nds,&block](size_t i, int64 vl) { nds[i].info.user = block.stringtable.at(vl); }},
        handle_pbf_packed_int{6, check_size, [&nds](size_t i, uint64 vl) { nds[i].info.visible = vl==1; }}
    );
    
}

void read_dense(std::vector<ElementPtr>& objects, const std::string& data, size_t pos, size_t lim, const block_data& block, changetype c) {

    std::vector<node_data> nds;
    dense_node_tags dnt(nds, block);
           
    read_pbf_messages(data, pos, lim, 
        handle_pbf_packed_int_delta{1,
            [&nds](size_t sz) { nds.resize(sz); },
            [&nds](size_t i, int64 v) { nds[i].id=v; }
        },
        handle_pbf_data{block.skip_info ? 0ull: 5, [&nds,&block](const std::string& d, size_t p, size_t l) { read_dense_info(nds,block,d,p,l); }},
        handle_pbf_packed_int_delta{8,
            [&nds](size_t sz) { if (sz!=nds.size()) { throw std::domain_error("unexpected size of dense lons"); } },
            [&nds](size_t i, int64 v) { nds[i].lat=v; }
        },
        handle_pbf_packed_int_delta{9,
            [&nds](size_t sz) { if (sz!=nds.size()) { throw std::domain_error("unexpected size of dense lats"); } },
            [&nds](size_t i, int64 v) { nds[i].lon=v; }
        },
        handle_pbf_packed_int{block.skip_strings ? 0ull: 10, nullptr, [&dnt](size_t i, int64 v) { dnt.call(i,v); }},
        
        handle_pbf_packed_int_delta{20,
            [&nds](size_t sz) { if (sz!=nds.size()) { throw std::domain_error("unexpected size of dense quadtree"); } },
            [&nds](size_t i, int64 v) { nds[i].qt=v; }
        }
    );
    
    for (const auto& nd: nds) {
        
        if (nd.id!=0) {
            objects.push_back(
                std::make_shared<Node>(c, nd.id, nd.qt, nd.info, nd.tags, nd.lon, nd.lat)
            );
        }
    }
}
    
    

ElementPtr read_way(const std::string& data, size_t pos, size_t lim, const block_data& block, changetype c) { 
    way_data wy{0,0,{0,0,0,0,"",false},{},{}};
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&wy](uint64 v) { wy.id=v; }},
        handle_pbf_packed_int{block.skip_strings ? 0ull: 2,
                [&wy](size_t s) { if (s>wy.tags.size()) { wy.tags.resize(s); } },
                [&wy,&block](size_t i, uint64 v) { wy.tags[i].key=block.stringtable.at(v); }
        },
        handle_pbf_packed_int{block.skip_strings ? 0ull: 3,
                [&wy](size_t s) { if (s>wy.tags.size()) { wy.tags.resize(s); } },
                [&wy,&block](size_t i, uint64 v) { wy.tags[i].val=block.stringtable.at(v); }
        },
        handle_pbf_data{block.skip_info ? 0ull: 4, [&wy,&block](const std::string& d, size_t p, size_t l) { read_info(wy.info, block, d, p, l); }},
        handle_pbf_packed_int_delta{8,
                [&wy](size_t sz) { wy.refs.resize(sz); },
                [&wy](size_t i, int64 v) { wy.refs[i]=v; }
        },
        handle_pbf_value{20, [&wy](uint64 v) { wy.qt=un_zig_zag(v); }}
    );
    if (wy.id==0) { return nullptr; }
    
    return std::make_shared<Way>(c, wy.id, wy.qt, wy.info, wy.tags, wy.refs);
}


ElementPtr read_relation(const std::string& data, size_t pos, size_t lim, const block_data& block, changetype c) {
    relation_data rl{0,0,{0,0,0,0,"",false},{},{}};
    
    
    read_pbf_messages(data, pos, lim, 
        handle_pbf_value{1, [&rl](uint64 v) { rl.id=v; }},
        handle_pbf_packed_int{block.skip_strings ? 0ull: 2,
                [&rl](size_t s) { if (s>rl.tags.size()) { rl.tags.resize(s); } },
                [&rl,&block](size_t i, uint64 v) { rl.tags[i].key=block.stringtable.at(v); }
        },
        handle_pbf_packed_int{block.skip_strings ? 0ull: 3,
                [&rl](size_t s) { if (s>rl.tags.size()) { rl.tags.resize(s); } },
                [&rl,&block](size_t i, uint64 v) { rl.tags[i].val=block.stringtable.at(v); }
        },
        handle_pbf_data{block.skip_info ? 0ull: 4, [&rl,&block](const std::string& d, size_t p, size_t l) { read_info(rl.info, block, d, p, l); }},
        
        handle_pbf_packed_int{block.skip_strings ? 0ull: 8,
                [&rl](size_t sz) { if (sz>rl.members.size()) { rl.members.resize(sz); } },
                [&rl,&block](size_t i, int64 v) { rl.members[i].role=block.stringtable.at(v); }
        },
        handle_pbf_packed_int_delta{9,
                [&rl](size_t sz) { if (sz>rl.members.size()) { rl.members.resize(sz);  }},
                [&rl](size_t i, int64 v) { rl.members[i].ref=v; }
        },
        handle_pbf_packed_int{10,
                [&rl](size_t sz) { if (sz>rl.members.size()) { rl.members.resize(sz); } },
                [&rl](size_t i, int64 v) { rl.members[i].type=(ElementType) v; }
        },
        
        handle_pbf_value{20, [&rl](uint64 v) { rl.qt=un_zig_zag(v); }}
    );
    if (rl.id==0) { return nullptr; }
    
    return std::make_shared<Relation>(c, rl.id, rl.qt, rl.info, rl.tags, rl.members);
}

ElementPtr read_geometry_default(const std::string& data, size_t pos, size_t lim, ElementType ty, const block_data& block, changetype c) { return nullptr; }



void read_primitive_group(std::vector<ElementPtr>& objects,
    const block_data& block,
    const std::string& data, size_t pos, size_t lim) {
    
    
    changetype c = changetype::Normal;
    if (block.change) {
        read_pbf_messages(data, pos, lim, 
            handle_pbf_value{10, [&c](uint64 vl) { c = (changetype) vl; }}
        );
    }
    
    read_pbf_messages(data,pos,lim,
        handle_pbf_data{block.skip_nodes ? 0ull: 1, [&objects,&block,c](const std::string& d, size_t p, size_t l) {
            auto obj = read_node(d,p,l, block, c);
            if (obj) { objects.push_back(obj); }
        }},
        handle_pbf_data{block.skip_nodes ? 0ull: 2, [&objects,&block,c](const std::string& d, size_t p, size_t l) {
            read_dense(objects, d,p,l, block, c);
        }},
        handle_pbf_data{block.skip_ways ? 0ull: 3, [&objects,&block,c](const std::string& d, size_t p, size_t l) {
                auto obj = read_way(d,p,l, block, c);
                if (obj) { objects.push_back(obj); }
        }},
        handle_pbf_data{block.skip_relations ? 0ull: 4, [&objects,&block,c](const std::string& d, size_t p, size_t l) {
                auto obj = read_relation(d,p,l, block, c);
                if (obj) { objects.push_back(obj); }
        }},
        handle_pbf_data_mt{20,24, [&objects,&block,c](uint64 tg, const std::string& d, size_t p, size_t l) {
            if (!block.skip_geometries) {
                ElementType ty = (ElementType) (tg-17);
                ElementPtr obj;
                if (!block.readGeometry) {
                    obj = read_geometry_default(d, p, l, ty, block, c);
                } else {
                    obj = block.readGeometry(ty, d.substr(p, l-p), block.stringtable, c);
                }
                if (obj) {
                    objects.push_back(obj);
                }
            }
        }}
    );
}

int64 read_quadtree(const std::string& data, size_t pos, size_t lim) {
    uint64 x,y,z;
    
    read_pbf_messages(data, pos, lim,
        handle_pbf_value{1, [&x](uint64 v) { x=v; }},
        handle_pbf_value{2, [&y](uint64 v) { y=v; }},
        handle_pbf_value{3, [&z](uint64 v) { z=v; }}
    );

    return quadtree::from_tuple(x,y,z);
}


void read_string_table(std::vector<std::string>& stringtable, const std::string& data, size_t pos, size_t lim) {
    
    read_pbf_messages(data, pos, lim,
        handle_pbf_data{1, [&stringtable](const std::string& d, size_t p, size_t l) { stringtable.push_back(d.substr(p,l-p)); }}
    );
}
        
}

using oqt::readblock_detail::block_data;
using oqt::readblock_detail::read_string_table;
using oqt::readblock_detail::read_quadtree;
using oqt::readblock_detail::read_primitive_group;
PrimitiveBlockPtr read_primitive_block_new(int64 idx, const std::string& data, bool change, ReadBlockFlags objflags, IdSetPtr ids, read_geometry_func readGeometry) {
    
    block_data block{{},change,ids,readGeometry,
        has_flag(objflags,ReadBlockFlags::SkipNodes),
        has_flag(objflags,ReadBlockFlags::SkipWays),
        has_flag(objflags,ReadBlockFlags::SkipRelations),
        has_flag(objflags,ReadBlockFlags::SkipGeometries),
        has_flag(objflags,ReadBlockFlags::SkipStrings),
        has_flag(objflags,ReadBlockFlags::SkipInfo)};
   
    auto primblock = std::make_shared<PrimitiveBlock>(idx,(change || ids || (objflags!=ReadBlockFlags::Empty)) ? 0: 8000);


    std::vector<std::pair<size_t,size_t>> group_poses;
    

    read_pbf_messages(data, 0, data.size(),
        handle_pbf_data{block.skip_strings ? 0ull : 1, [&block](const std::string& d, size_t p, size_t l) { read_string_table(block.stringtable, d, p, l); }},
        handle_pbf_data{2, [&group_poses](const std::string&, size_t p, size_t l) { group_poses.push_back(std::make_pair(p,l)); }},
        handle_pbf_data{31,[&primblock](const std::string& d, size_t p, size_t l) { primblock->SetQuadtree(read_quadtree(d,p,l)); }},
        handle_pbf_value{32, [&primblock](uint64 vl) { primblock->SetQuadtree(un_zig_zag(vl)); }},
        handle_pbf_value{33, [&primblock](uint64 vl) { primblock->SetStartDate(vl); }},
        handle_pbf_value{34, [&primblock](uint64 vl) { primblock->SetEndDate(vl); }}
    );
    
    for (const auto& pl: group_poses) {
        read_primitive_group(primblock->Objects(), block, data, pl.first, pl.second);
    }

    return primblock;
}
}
