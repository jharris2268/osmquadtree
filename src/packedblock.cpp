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

#include "oqt/packedblock.hpp"
#include <list>
#include <iostream>
#include <deque>
namespace oqt {
/*uint32_t*/void fetchInfo(std::list<PbfTag>& nl, const std::vector<std::string>& st, const std::string& data) {
    size_t pos=0;
    //uint32_t vs=0;
    for (auto tg = readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        //if (tg.tag==1) { vs=tg.value; }

        if (tg.tag==5) {
            nl.push_back(PbfTag{45,0,st.at(tg.value)});
        } else {
            nl.push_back(PbfTag{40+tg.tag,tg.value,""});
        }
    }
    //return vs;
}

void fetchStrings(std::list<PbfTag>& ss, uint64 tg, const std::vector<std::string>& st, const std::string& data) {
    size_t pos=0;
    while ( pos < data.size() ) {
        const std::string& v=st.at(readUVarint(data,pos));
        ss.push_back(PbfTag{tg,0,v});
    }
}




void readPackedObject(
    //std::shared_ptr<packedblock> res,
    std::vector<packedobj_ptr>& res,
    uint64 tyoff,
    const std::vector<std::string>& stringtable,
    const std::string& data, changetype ct,
    std::shared_ptr<idset> ids) {


    int64 id=0, quadtree=0;
    //uint32_t vs=0;
    std::list<PbfTag> nl;

    size_t pos=0;
    for (auto tg = readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        if (tg.tag==1) {
            id=int64(tg.value);
            if (ids && !ids->contains((elementtype) tyoff, id)) {
                return;
            }
        } else if ((tg.tag==2) || (tg.tag==3) || ((tyoff==2)&&(tg.tag==8))) {
            fetchStrings(nl,tg.tag,stringtable,tg.data);
        } else if (tg.tag==4) {

            /*vs=*/fetchInfo(nl,stringtable,tg.data);
        } else if (tg.tag==20) {
            quadtree = unZigZag(tg.value);
        } else if ( (tyoff==0) && ((tg.tag==8) || (tg.tag==9))) {
            nl.push_back(tg);
        } else {
            nl.push_back(tg);
        }
    }

    //res->add(id | tyoff<<61,quadtree,packPbfTags(nl), ct, vs);
    res.push_back(std::make_shared<packedobj>( ((uint64) id) | tyoff<<61,quadtree,packPbfTags(nl), ct));
}


void readPackedDenseAlt(
    //std::shared_ptr<packedblock> res,
    std::vector<packedobj_ptr>& res,
    const std::vector<std::string>& stringtable,
    const std::string& data, changetype ct,
    std::shared_ptr<idset> checkids) {

    std::vector<int64> ids, qts, lns, lts;
    std::vector<int64> ts,cs,ui,us,iv;
    std::vector<uint64> vs,kvs;

    size_t pos=0;
    for (auto tg=readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        if (tg.tag==1) { ids=readPackedDelta(tg.data); }
        else if (tg.tag==5) {
            size_t tp=0;
            for (auto it = readPbfTag(tg.data,tp); it.tag>0; it = readPbfTag(tg.data,tp)) {
                if (it.tag==1) { vs=readPackedInt(it.data); }
                else if (it.tag==2) { ts=readPackedDelta(it.data); }
                else if (it.tag==3) { cs=readPackedDelta(it.data); }
                else if (it.tag==4) { ui=readPackedDelta(it.data); }
                else if (it.tag==5) { us=readPackedDelta(it.data); }
                else if (it.tag==6) { iv=readPackedDelta(it.data); }
            }
        } else if (tg.tag==8) { lts=readPackedDelta(tg.data); }
        else if (tg.tag==9) { lns=readPackedDelta(tg.data); }
        else if (tg.tag==10) { kvs=readPackedInt(tg.data); }
        else if (tg.tag==20) { qts=readPackedDelta(tg.data); }
    }

    res.reserve(res.size()+ids.size());
    size_t tagi=0;

    for (size_t i=0; i < ids.size(); i++) {
        int64 id=ids[i];
        int64 qt=-1;
        if (qts.size()>i) {
            qt=qts[i];
        }

        std::list<PbfTag> mm;

        if (kvs.size()>tagi) {
            std::list<PbfTag> vv;
            while (( (tagi+1) < kvs.size()) && (kvs[tagi]!=0)) {
                mm.push_back(PbfTag{2,0,stringtable.at(kvs[tagi])});
                vv.push_back(PbfTag{3,0,stringtable.at(kvs[tagi+1])});
                tagi+=2;
            }
            tagi++;
            mm.splice(mm.end(),vv);
        }

        if (vs.size()>i) {
            mm.push_back(PbfTag{41,vs[i],""});
            if (ts.size()>i) {
                mm.push_back(PbfTag{42,uint64(ts[i]),""});
            }
            if (cs.size()>i) {
                mm.push_back(PbfTag{43,uint64(cs[i]),""});
            }
            if (ui.size()>i) {
                mm.push_back(PbfTag{44,uint64(ui[i]),""});
            }
            if (us.size()>i) {
                mm.push_back(PbfTag{45,0,stringtable.at(us[i])});
            }
            if ((iv.size()>i) && (iv[i]==0)) {
                mm.push_back(PbfTag{46,0,""});
            }
        }

        if (lts.size()>i) {
            mm.push_back(PbfTag{8,zigZag(lts[i]),""});
            mm.push_back(PbfTag{9,zigZag(lns[i]),""});
        }
        if (!checkids || checkids->contains(elementtype::Node,id)) {
            //res->add(id,qt,packPbfTags(mm),ct,(vs.size()>i ? vs[i] : 0));
            res.push_back(std::make_shared<packedobj>((uint64) id,qt,packPbfTags(mm), ct));
        }

    }
}

void readPackedGroup(
    //std::shared_ptr<packedblock> res,
    //std::deque<packedobj_ptr>& res,
    std::vector<packedobj_ptr>& res,
    const std::vector<std::string>& stringtable,
    const std::string& data,
    size_t objflags, bool change,
    std::shared_ptr<idset> ids) {

    changetype ct = changetype::Normal;
    if (change) {
        size_t pos=0;
        PbfTag tag = readPbfTag(data,pos);
        for ( ; tag.tag>0; tag = readPbfTag(data,pos)) {
            if (tag.tag==10) { ct=(changetype) tag.value; }
        }
    }
    
    res.reserve(res.size()+data.size()/100);

    size_t pos=0;
    
    for (auto tg = readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {

        if (tg.tag==1) {
            if (objflags&1) {
                readPackedObject(res,0,stringtable,tg.data,ct,ids);
            }
        } else if (tg.tag==2) {
            if (objflags&1) {

                readPackedDenseAlt(res,stringtable,tg.data,ct,ids);
            }
        } else if (tg.tag==3) {
            if (objflags&2) {
                readPackedObject(res,1,stringtable,tg.data,ct,ids);
            }
        } else if (tg.tag==4) {
            if (objflags&4) {
                readPackedObject(res,2,stringtable,tg.data,ct,ids);
            }
        } else if ((tg.tag>=20) && (objflags&8)) {
            readPackedObject(res,tg.tag-17,stringtable,tg.data,ct,ids);
        }
    }
}


std::shared_ptr<packedblock> readPackedBlock(int64 index, const std::string& data, size_t objflags, bool change,std::shared_ptr<idset> ids) {

    size_t pos=0;

    //int64 quadtree=-1;
    std::list<std::string> objs;
    std::vector<std::string> st;
    //int64 startdate=0,enddate=0;
    auto res = std::make_shared<packedblock>(index,0);
    
    for (auto tg = readPbfTag(data,pos); tg.tag>0; tg=readPbfTag(data,pos)) {
        if (tg.tag==1) {
            st=readStringTable(tg.data);
        } else if (tg.tag==2) {
            objs.push_back(tg.data);

        } else if (tg.tag==31) {
            res->quadtree=readQuadTree(tg.data);
        } else if (tg.tag==32) {
            res->quadtree=unZigZag(tg.value);
        } else if (tg.tag==33) {
            res->startdate=int64(tg.value);//unZigZag(tg.value);
        } else if (tg.tag==34) {
            res->enddate=int64(tg.value);//unZigZag(tg.value);
        }
    }

    //std::deque<packedobj_ptr> temp;
    for (auto oo : objs) {
        readPackedGroup(res->objects, st, oo, objflags, change,ids);
    }
    /*size_t sz=0;
    for (auto& p : temp) {
        sz+=p->Data().size();
    }*/
    //auto res = std::make_shared<packedblock>(index,quadtree,temp.size(),sz,startdate,enddate);
    
    
    /*for (auto& p : temp) {
        res->add(p);//std::move(p));
    }*/
    return res;
}
}
