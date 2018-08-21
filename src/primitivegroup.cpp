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
#include <iostream>

namespace oqt {

std::shared_ptr<element> readNode(const std::string& data, const std::vector<std::string>& stringtable,
    changetype ct, std::shared_ptr<idset> ids);


int readDenseNodes(const std::string& data, const std::vector<std::string>& stringtable,
    changetype ct, std::vector<std::shared_ptr<element> >& objects, std::shared_ptr<idset> ids);


std::shared_ptr<element> readWay(const std::string& data, const std::vector<std::string>& stringtable,
    changetype ct, std::shared_ptr<idset> ids);
std::shared_ptr<element> readRelation(const std::string& data, const std::vector<std::string>& stringtable,
    changetype ct, std::shared_ptr<idset> ids);


std::shared_ptr<element> readGeometry_default(elementtype ty, const std::string& data, const std::vector<std::string>& stringtable, changetype ct);



void readPrimitiveGroupCommon(
        const std::string& data, const std::vector<std::string>& stringtable,
        std::vector<std::shared_ptr<element> >& objects,
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

            elementtype ty = (elementtype) (tag.tag-17);
            auto o = readGeometry(ty, tag.data, stringtable, ct);
            if (o) { objects.push_back(o); }
        }
        
    }

    return;
}

void readPrimitiveGroup(const std::string& data, const std::vector<std::string>& stringtable, std::vector<std::shared_ptr<element> >& objects, bool change, size_t objflags, std::shared_ptr<idset> ids,read_geometry_func readGeometry) {

    

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
    readCommon(elementtype ty, const std::string& data, const std::vector<std::string>& stringtable, std::shared_ptr<idset> ids) {


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


std::shared_ptr<element> readNode(
        const std::string& data, const std::vector<std::string>& stringtable,
        changetype ct, std::shared_ptr<idset> ids) {
    int64 id,qt;
    
    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(elementtype::Node,data,stringtable,ids);
    if (id==0) { return std::shared_ptr<element>(); }

    int64 lon=0,lat=0;
    for (const auto& t : rem) {
        if (t.tag == 8) {
            lat = unZigZag(t.value);
        } else if (t.tag == 9) {
            lon = unZigZag(t.value);
        }
    }
    return std::make_shared<node>(ct,id,qt,inf,tags, lon,lat);
    
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
bool has_an_id(std::shared_ptr<idset> id_set, elementtype ty, const std::vector<int64>& ids) {
    for (auto& i: ids) {
        if (id_set->contains(ty, i)){
            return true;
        }
    }
    return false;
}

int readDenseNodes(
        const std::string& data, const std::vector<std::string>& stringtable,
        changetype ct, std::vector<std::shared_ptr<element> >& objects, std::shared_ptr<idset> id_set) {

    std::vector<int64> ids, lons, lats, qts;
    std::vector<uint64> kvs;

    std::vector<int64> ts,cs,ui,us;
    std::vector<uint64> vs,vv;


    size_t pos = 0;
    PbfTag pbfTag = readPbfTag(data,pos);
    for ( ; pbfTag.tag>0; pbfTag = readPbfTag(data,pos)) {
        if      (pbfTag.tag==1) {
            ids = readPackedDelta(pbfTag.data);
            if (id_set && !has_an_id(id_set,elementtype::Node, ids)) {
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
        if (!id_set || id_set->contains(elementtype::Node,id)) {
            objects.push_back(std::make_shared<node>(ct,id,qt,inf,tags,lon,lat));
            pp++;
        }

    }
    return pp;
}

std::shared_ptr<element> readWay(const std::string& data, const std::vector<std::string>& stringtable, changetype ct, std::shared_ptr<idset> ids) {

    int64 id,qt;

    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(elementtype::Way,data,stringtable,ids);
    if (id==0) { return std::shared_ptr<element>(); }

    std::vector<int64> refs;
    
    for (const auto& t : rem) {
        if (t.tag == 8) {
            refs = readPackedDelta(t.data);

        }
    }
    return std::make_shared<way>(ct,id,qt,inf,tags,refs);

}

std::shared_ptr<element> readRelation(const std::string& data, const std::vector<std::string>& stringtable,
        changetype ct, std::shared_ptr<idset> ids) {
    int64 id,qt;

    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(elementtype::Relation,data,stringtable,ids);
    if (id==0) { return std::shared_ptr<element>(); }
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
            mems[i].type = Node;
        } else if (ty[i]==1) {
            mems[i].type = Way;
        } else if (ty[i]==2) {
            mems[i].type = Relation;
        } else {
            throw std::domain_error("wrong member type?? "+std::to_string(ty[i]));
        }
        mems[i].ref = rf[i];
        if (i < rl.size()) {
            mems[i].role = stringtable.at(rl[i]);
        }
    }

    return std::make_shared<relation>(ct,id,qt,inf,tags,mems);


}


std::shared_ptr<element> readGeometry_default(elementtype ty, const std::string& data, const std::vector<std::string>& stringtable, changetype ct) {
    int64 id,qt;

    info inf; tagvector tags;
    std::list<PbfTag> rem;

    std::tie(id,inf,tags,qt,rem) = readCommon(ty,data,stringtable,nullptr);
    int64 minzoom=-1;
    if ((!rem.empty()) && (rem.back().tag==22)) {
        minzoom=rem.back().value;        
    }    
    return std::make_shared<geometry_packed>(ty,ct,id,qt,inf,tags,minzoom,rem);
};
}
