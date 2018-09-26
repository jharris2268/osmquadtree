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

#include "oqt/utils/pbf/protobuf.hpp"

#include "oqt/pbfformat/idset.hpp"
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/elements/element.hpp"

#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/geometry.hpp"


#include <iostream>

namespace oqt {


info readInfo(const std::string& data,const std::vector<std::string>& stringtable) {

    info ans;
    size_t pos = 0;

    bool vv=true;

    PbfTag tag = readPbfTag(data,pos);
    for ( ; tag.tag>0; tag = readPbfTag(data,pos)) {
        if      (tag.tag==1) { ans.version = int64(tag.value); }
        else if (tag.tag==2) { ans.timestamp = int64(tag.value); }
        else if (tag.tag==3) { ans.changeset = int64(tag.value); }
        else if (tag.tag==4) { ans.user_id = int64(tag.value); }
        else if (tag.tag==5) { ans.user = stringtable.at(tag.value); }
        else if (tag.tag==6) { vv = tag.value!=0; }

    }
    ans.visible=vv;
    
    return ans;

}

tagvector makeTags(
    const std::vector<uint64>& keys, const std::vector<uint64>& vals,
    const std::vector<std::string>& stringtable) {

    tagvector ans(keys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        ans[i].key=stringtable.at(keys.at(i));
        ans[i].val=stringtable.at(vals.at(i));
    }
    return ans;


}


std::tuple<int64,info,tagvector,int64,std::list<PbfTag> >
    readCommon(ElementType ty, const std::string& data, const std::vector<std::string>& stringtable, std::shared_ptr<idset> ids) {


    std::vector<uint64> keys,vals;
    std::tuple<int64,info,tagvector,int64,std::list<PbfTag> > r;
    std::get<0>(r)=0; std::get<3>(r)=-1;

    size_t pos = 0;
    PbfTag tag = readPbfTag(data,pos);
    for ( ; tag.tag>0; tag = readPbfTag(data,pos)) {
        if (tag.tag==1) {
            int64 id = int64(tag.value);
            if (ids && (!ids->contains(ty,id))) {
                return r;
            }
            std::get<0>(r)=id;
        } else if (tag.tag==2) {
            keys = readPackedInt(tag.data);
        } else if (tag.tag==3) {
            vals = readPackedInt(tag.data);
        } else if (tag.tag==4) {
            std::get<1>(r) = readInfo(tag.data,stringtable);
        } else if (tag.tag==20) {
            std::get<3>(r) = unZigZag(tag.value);
        } else {
            std::get<4>(r).push_back(tag);
        }
    }

    std::get<2>(r) = makeTags(keys,vals,stringtable);
    return r;
}


ElementPtr readNode(
        const std::string& data, const std::vector<std::string>& stringtable,
        changetype ct, std::shared_ptr<idset> ids) {
    int64 id,qt;
    
    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(ElementType::Node,data,stringtable,ids);
    if (id==0) { return ElementPtr(); }

    int64 lon=0,lat=0;
    for (const auto& t : rem) {
        if (t.tag == 8) {
            lat = unZigZag(t.value);
        } else if (t.tag == 9) {
            lon = unZigZag(t.value);
        }
    }
    return std::make_shared<Node>(ct,id,qt,inf,tags, lon,lat);
    
}

std::tuple<std::vector<uint64>,std::vector<int64>,std::vector<int64>,std::vector<int64>,std::vector<int64>,std::vector<uint64> >
    readDenseInfo(const std::string& data)
{
    std::vector<int64> ts,cs,ui,us;
    std::vector<uint64> vs, vv;


    size_t pos = 0;
    PbfTag tag = readPbfTag(data,pos);
    for ( ; tag.tag>0; tag = readPbfTag(data,pos)) {
        if (tag.tag==1) { vs = readPackedInt(tag.data); }
        if (tag.tag==2) { ts = readPackedDelta(tag.data); }
        if (tag.tag==3) { cs = readPackedDelta(tag.data); }
        if (tag.tag==4) { ui = readPackedDelta(tag.data); }
        if (tag.tag==5) { us = readPackedDelta(tag.data); }
        if (tag.tag==6) { vv = readPackedInt(tag.data); }
    }
    return std::make_tuple(vs,ts,cs,ui,us,vv);
}
bool has_an_id(std::shared_ptr<idset> id_set, ElementType ty, const std::vector<int64>& ids) {
    for (auto& i: ids) {
        if (id_set->contains(ty, i)){
            return true;
        }
    }
    return false;
}

int readDenseNodes(
        const std::string& data, const std::vector<std::string>& stringtable,
        changetype ct, std::vector<ElementPtr >& objects, std::shared_ptr<idset> id_set) {

    std::vector<int64> ids, lons, lats, qts;
    std::vector<uint64> kvs;

    std::vector<int64> ts,cs,ui,us;
    std::vector<uint64> vs,vv;


    size_t pos = 0;
    PbfTag pbfTag = readPbfTag(data,pos);
    for ( ; pbfTag.tag>0; pbfTag = readPbfTag(data,pos)) {
        if      (pbfTag.tag==1) {
            ids = readPackedDelta(pbfTag.data);
            if (id_set && !has_an_id(id_set,ElementType::Node, ids)) {
                return 0;
            }
        }
        else if (pbfTag.tag==5) { std::tie(vs,ts,cs,ui,us,vv) = readDenseInfo(pbfTag.data); }
        else if (pbfTag.tag==8) { lats = readPackedDelta(pbfTag.data); }
        else if (pbfTag.tag==9) { lons = readPackedDelta(pbfTag.data); }
        else if (pbfTag.tag==10) { kvs = readPackedInt(pbfTag.data); }
        else if (pbfTag.tag==20) { qts = readPackedDelta(pbfTag.data); }
    }

    size_t tagi = 0;
    int pp=0;
    for (size_t i = 0; i < ids.size(); i++) {
        int64 id = ids.at(i);
        tagvector tags;
        info inf;

        if (i<vs.size()) {
            inf = info(
                vs.at(i),
                ts.at(i),
                cs.at(i),
                ui.at(i),
                stringtable.at(us.at(i)),
                true
            );
            if (vv.size()>0) {
                inf.visible = vv.at(i)!=0;
            } 
        }
        if (tagi < kvs.size()) {
            
            while ((tagi < kvs.size()) && kvs.at(tagi)!=0) {
                
                tags.push_back(tag(
                    stringtable.at(kvs.at(tagi)),
                    stringtable.at(kvs.at(tagi+1))
                ));
                tagi+=2;
            }
            tagi += 1;
            
        }

        int64 qt = -1;

        
        if (i < qts.size()) {
            
            qt=qts.at(i);
        }
        int64 lon=0,lat = 0;
        if (i < lons.size()) {
            lon=lons.at(i);
            lat=lats.at(i);
        }
        if (!id_set || id_set->contains(ElementType::Node,id)) {
            objects.push_back(std::make_shared<Node>(ct,id,qt,inf,tags,lon,lat));
            pp++;
        }

    }
    return pp;
}

ElementPtr readWay(const std::string& data, const std::vector<std::string>& stringtable, changetype ct, std::shared_ptr<idset> ids) {

    int64 id,qt;

    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(ElementType::Way,data,stringtable,ids);
    if (id==0) { return ElementPtr(); }

    std::vector<int64> refs;
    
    for (const auto& t : rem) {
        if (t.tag == 8) {
            refs = readPackedDelta(t.data);

        }
    }
    return std::make_shared<Way>(ct,id,qt,inf,tags,refs);

}

ElementPtr readRelation(const std::string& data, const std::vector<std::string>& stringtable,
        changetype ct, std::shared_ptr<idset> ids) {
    int64 id,qt;

    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(ElementType::Relation,data,stringtable,ids);
    if (id==0) { return ElementPtr(); }
    std::vector<uint64> ty, rl;
    std::vector<int64> rf;
    for (const auto& t : rem) {
        if (t.tag== 8) {
            rl = readPackedInt(t.data);
        } else if (t.tag == 9) {
            rf = readPackedDelta(t.data);
        } else if (t.tag == 10) {
            ty = readPackedInt(t.data);
        }
    }
    memvector mems(rf.size());
    for (size_t i=0; i < rf.size(); i++) {
        if (ty[i]==0) {
            mems[i].type = ElementType::Node;
        } else if (ty[i]==1) {
            mems[i].type = ElementType::Way;
        } else if (ty[i]==2) {
            mems[i].type = ElementType::Relation;
        } else {
            throw std::domain_error("wrong member type?? "+std::to_string(ty[i]));
        }
        mems[i].ref = rf[i];
        if (i < rl.size()) {
            mems[i].role = stringtable.at(rl[i]);
        }
    }

    return std::make_shared<Relation>(ct,id,qt,inf,tags,mems);


}


ElementPtr readGeometry_default(ElementType ty, const std::string& data, const std::vector<std::string>& stringtable, changetype ct) {
    int64 id,qt;

    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(ty,data,stringtable,nullptr);
    int64 minzoom=-1;
    if ((!rem.empty()) && (rem.back().tag==22)) {
        minzoom=rem.back().value;        
    }    
    return std::make_shared<GeometryPacked>(ty,ct,id,qt,inf,tags,minzoom,rem);
};

void readPrimitiveGroupCommon(
        const std::string& data, const std::vector<std::string>& stringtable,
        std::vector<ElementPtr >& objects,
        changetype ct, size_t objflags, std::shared_ptr<idset> ids,
        read_geometry_func readGeometry) {

    
    if (!readGeometry) {
        readGeometry = readGeometry_default;
    }

    size_t pos=0;

    PbfTag tag = readPbfTag(data,pos);
    for ( ; tag.tag>0; tag = readPbfTag(data,pos)) {

        if ((tag.tag==1) && (objflags&1)) {
            auto o = readNode(tag.data, stringtable,ct,ids);
            if (o) { objects.push_back(o); }

        } else if ((tag.tag==2) && (objflags&1)) {
            readDenseNodes(tag.data, stringtable, ct,objects,ids);
        } else if ((tag.tag==3) && (objflags&2)) {
            auto o = readWay(tag.data, stringtable,ct,ids);
            if (o) { objects.push_back(o); }
        } else if ((tag.tag==4) && (objflags&4)) {
            auto o = readRelation(tag.data, stringtable,ct,ids);
            if (o) { objects.push_back(o); }
        } else if ((tag.tag>=20) && (objflags&8) ) {

            ElementType ty = (ElementType) (tag.tag-17);
            auto o = readGeometry(ty, tag.data, stringtable, ct);
            if (o) { objects.push_back(o); }
        }
        
    }

    return;
}

void readPrimitiveGroup(const std::string& data, const std::vector<std::string>& stringtable, std::vector<ElementPtr >& objects, bool change, size_t objflags, std::shared_ptr<idset> ids,read_geometry_func readGeometry) {

    

    if (!change) {
        readPrimitiveGroupCommon(data,stringtable,objects,changetype::Normal,objflags,ids,readGeometry);
        return;
    }

    size_t pos=0;

    changetype ct=changetype::Normal;

    PbfTag tag = readPbfTag(data,pos);
    for ( ; tag.tag>0; tag = readPbfTag(data,pos)) {
        if (tag.tag==10) { ct=(changetype) tag.value; }
    }
    readPrimitiveGroupCommon(data,stringtable,objects, ct,objflags,ids,readGeometry);
}




std::vector<std::string> readStringTable(const std::string& data) {

    size_t pos=0;

    std::vector<std::string> result;
    PbfTag pbfTag = readPbfTag(data,pos);
    for ( ; pbfTag.tag>0; pbfTag = readPbfTag(data,pos)) {
        if (pbfTag.tag==1) {
            result.push_back(pbfTag.data);
        }
    }
    return result;
}

int64 readQuadTree(const std::string& data) {
    uint64 x=0,y=0,z=0;

    size_t pos=0;
    PbfTag pbfTag = readPbfTag(data,pos);
    for ( ; pbfTag.tag>0; pbfTag = readPbfTag(data,pos)) {
        if (pbfTag.tag==1) { x=pbfTag.value; }
        if (pbfTag.tag==2) { y=pbfTag.value; }
        if (pbfTag.tag==3) { z=pbfTag.value; }
    }
    return quadtree::from_tuple(x,y,z);
    /*
    int64 ans=0;
    int64 scale = 1;

    for (uint64 i=0; i < z; i++) {
        ans += (int64((x>>i)&1) | int64(((y>>i)&1)<<1) ) * scale;
        scale *= 4;
    }

    ans <<= (63 - (2 * z));
    ans |= int64(z);
    return ans;*/
}

void readHeaderBbox(const std::string& data, bbox& bb) {
    size_t pos=0;
    for (PbfTag tg=readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        switch (tg.tag) {
            case 1: bb.minx=unZigZag(tg.value)/100; break; //left
            case 2: bb.maxx=unZigZag(tg.value)/100; break; //right
            case 3: bb.maxy=unZigZag(tg.value)/100; break; //top
            case 4: bb.miny=unZigZag(tg.value)/100; break; //bottom
        }
    }
    
    if (bb.miny>bb.maxy) {
        bb = bbox(bb.minx,bb.maxx,bb.maxy,bb.miny);
    }
}
std::tuple<int64,bool,int64> readBlockIdx(const std::string& data) {
    int64 qt=0, len=0;
    bool isc=false;
    size_t pos=0;
    for (PbfTag tg=readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        //switch (tg.tag) {
        if (tg.tag==1) { qt=readQuadTree(tg.data);}
        else if (tg.tag==2) {isc=tg.value!=0;}
        else if (tg.tag==3) {len=unZigZag(tg.value);}
        else if (tg.tag==4) { qt = unZigZag(tg.value); }

    }

    return std::make_tuple(qt,isc,len);
}




std::shared_ptr<primitiveblock> readPrimitiveBlock(int64 idx, const std::string& data, bool change, size_t objflags, std::shared_ptr<idset> ids, read_geometry_func readGeometry) {


    std::vector<std::string> stringtable;

    std::shared_ptr<primitiveblock> primblock(new primitiveblock(idx,(change || ids || (objflags!=7)) ? 0 : 8000));


    std::vector<uint64> kk,vv;


    size_t pos=0;

    std::vector<std::string> blocks;
    PbfTag pbfTag = readPbfTag(data,pos);
    for ( ; pbfTag.tag>0; pbfTag = readPbfTag(data,pos)) {
        if (pbfTag.tag==1) {
            stringtable = readStringTable(pbfTag.data);
        } else if (pbfTag.tag==2) {
            blocks.push_back(pbfTag.data);
        } else if (pbfTag.tag==31) {
            primblock->quadtree = readQuadTree(pbfTag.data);
        } else if (pbfTag.tag == 32) {
            primblock->quadtree = unZigZag(pbfTag.value);
        } else if (pbfTag.tag==33) {
            primblock->startdate = int64(pbfTag.value);
        } else if (pbfTag.tag==34) {
            primblock->enddate = int64(pbfTag.value);
        }
    }


    

    for (size_t i=0; i < blocks.size(); i++) {
        readPrimitiveGroup(blocks[i], stringtable, primblock->objects,change, objflags,ids, readGeometry);
    }


    return primblock;
}









std::shared_ptr<header> readPbfHeader(const std::string& data, int64 fl) {
    auto res=std::make_shared<header>();
    size_t pos=0;
    for (PbfTag tg=readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        switch (tg.tag) {
            case 1: readHeaderBbox(tg.data, res->box); break;
            case 4: res->features.push_back(tg.data); break;
            case 16: res->writer=tg.data; break;
            case 22:
                int64 qt,len; bool isc=false;
                std::tie(qt,isc,len) = readBlockIdx(tg.data);
                res->index.push_back(std::make_tuple(qt,fl,len));
                fl+=len;

                break;
        }
    }


    return res;

}
}
