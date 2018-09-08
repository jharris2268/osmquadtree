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

#include "oqt/simplepbf.hpp"
#include "oqt/utils.hpp"
#include <iostream>
#include <algorithm>

#include <string>
#include <locale>
#include <codecvt>

#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/packedobj.hpp"
#include "oqt/elements/geometry.hpp"


namespace oqt {
    
    /*
std::string packedobj::pack() const {
    std::string out(30+data.size(),0);
    size_t pos=0;

    pos = writePbfValue(out,pos,1,uint64(id));
    std::copy(data.begin(),data.end(),out.begin()+pos);
    pos += data.size();
    pos = writePbfValue(out,pos,20,zigZag(qt));
    if (ct>0) {
        pos=writePbfValue(out,pos,25,ct);
    }
    out.resize(pos);
    return out;
}


packedobj_ptr make_packedobj_ptr(uint64 id, int64 quadtree, const std::string& data, changetype ct) {
    //return packedobj_ptr(new packedobj{id,quadtree,data,changetype})    ;
    return std::make_shared<packedobj>(id,quadtree,data,ct);
}





packedobj_ptr read_packedobj(const std::string& data) {

    //packedobj_ptr res(new packedobj);
    uint64 id=0; int64 qt=-1; changetype ct=changetype::Normal;
    
    size_t pos=0;
    std::list<PbfTag> mm;
    PbfTag tg=readPbfTag(data,pos);
    for ( ; tg.tag>0; tg=readPbfTag(data,pos)) {
        if (tg.tag==1) {
            id = tg.value;
        } else if (tg.tag==20) {
            qt = unZigZag(tg.value);
        } else if (tg.tag==25) {
            ct = (changetype) tg.value;
        } else {
            mm.push_back(tg);

        }
    }
    return make_packedobj_ptr(id,qt,packPbfTags(mm),ct);
}



std::tuple<info,tagvector,std::list<PbfTag>> unpack_packedobj_common(const std::string& data) {
    std::list<PbfTag> mm;
    info inf; tagvector tv;
    inf.visible=true;
    size_t tv_val_idx=0;
    
    size_t pos = 0;
    
    PbfTag tg=readPbfTag(data,pos);
    for ( ; tg.tag>0; tg=readPbfTag(data,pos)) {
        if (tg.tag==2) {
            tv.push_back(tag(tg.data,""));
        } else if (tg.tag==3) {
            if (tv_val_idx < tv.size()) {
                tv[tv_val_idx].val = tg.data;
                tv_val_idx++;
            }
        } else if (tg.tag==41) {
            inf.version = tg.value;
        } else if (tg.tag==42) {
            inf.timestamp = tg.value;
        } else if (tg.tag==43) {
            inf.changeset = tg.value;
        } else if (tg.tag==44) {
            inf.user_id = tg.value;
        } else if (tg.tag==45) {
            inf.user = tg.data;
        } else if (tg.tag==46) {
            inf.visible = (tg.value!=0);
        } else {
            mm.push_back(tg);
        }
    }
    return std::make_tuple(inf,tv,mm);
}
std::shared_ptr<element> unpack_packedobj_node(int64 id, changetype ct, int64 qt, const std::string& d) {
    info inf; tagvector tv; std::list<PbfTag> mm;
    std::tie(inf,tv,mm) = unpack_packedobj_common(d);
    int64 lat=0, lon=0;
    for (auto tg: mm) {
        if (tg.tag==8) { lat=unZigZag(tg.value); }
        else if (tg.tag==9) { lon=unZigZag(tg.value); }
    }
    return std::make_shared<node>(ct,id, qt, inf, tv, lon, lat);
}

std::shared_ptr<element> unpack_packedobj_way(int64 id, changetype ct, int64 qt, const std::string& d) {
    info inf; tagvector tv; std::list<PbfTag> mm;
    std::tie(inf,tv,mm) = unpack_packedobj_common(d);
    std::vector<int64> rfs;
    for (auto tg: mm) {
        if (tg.tag==8) { rfs = readPackedDelta(tg.data); };
    }
    return std::make_shared<way>(ct,id,qt, inf, tv, rfs);
}

std::shared_ptr<element> unpack_packedobj_relation(int64 id, changetype ct, int64 qt, const std::string& d) {
    info inf; tagvector tv; std::list<PbfTag> mm;
    std::tie(inf,tv,mm) = unpack_packedobj_common(d);
    
    memvector mv;
    mv.reserve(mm.size());
            
    for (auto tg: mm) {
        if (tg.tag==8) { mv.push_back(member(Node,0,tg.data)); }
        else if (tg.tag==9) {
            auto rfs = readPackedDelta(tg.data);
            if (mv.size() < rfs.size()) { mv.resize(rfs.size()); }
            for (size_t i=0; i < rfs.size(); i++) {
                mv[i].ref = rfs[i];
            }
        } else if (tg.tag==10) {
            auto tys = readPackedInt(tg.data);
            if (mv.size() < tys.size()) { mv.resize(tys.size()); }
            for (size_t i=0; i < tys.size(); i++) {
                if (tys[i]==0) { mv[i].type = Node; }
                else if (tys[i]==1) { mv[i].type = Way; }
                else if (tys[i]==2) { mv[i].type = Relation; }
                else { throw std::domain_error("wrong member type "+std::to_string(tys[i])); }
            }
        }
    }
    return std::make_shared<relation>(ct,id,qt, inf, tv, mv);
}


element_ptr unpack_packedobj(packedobj_ptr p) {
    return unpack_packedobj(p->Type(), p->Id(),p->ChangeType(),p->Quadtree(),p->Data());
}
std::shared_ptr<element> unpack_packedobj(int64 ty, int64 id, changetype ct, int64 qt, const std::string& d) {
    if (ty==0) {
        return unpack_packedobj_node(id,ct,qt,d);
    } else if (ty==1) {
        return unpack_packedobj_way(id,ct,qt,d);
    } else if (ty==2) {
        return unpack_packedobj_relation(id,ct,qt,d);
    }
    throw std::domain_error("unreconised type");
    return std::shared_ptr<element>();
}
*/




}

