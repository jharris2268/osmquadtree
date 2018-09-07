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

#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/pbfformat/protobuf.hpp"
#include "oqt/pbfformat/packedint.hpp"


#include "oqt/elements/quadtree.hpp"
#include <map>
#include <algorithm>
#include <iterator>
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/geometry.hpp"


namespace oqt {
namespace writeblock_detail {


uint64 getString(std::map<std::string,uint64>& stringtable, const std::string& str) {
    auto it=stringtable.find(str);
    if (it!=stringtable.end()) {
        return it->second;
    }
    uint64 a=stringtable.size();
    stringtable[str]=a;
    return a;
}

std::string packObject(
    std::map<std::string,uint64>& stringtable,
    std::shared_ptr<element> obj,
    bool includeQts,bool includeInfos,bool includeRefs) {
        
    std::list<PbfTag> msgs;

    msgs.push_back(PbfTag{1,uint64(obj->Id())});

    std::vector<uint64> kk,vv;
    
    for (const auto& tg: obj->Tags()) {
        kk.push_back(getString(stringtable, tg.key));
    }
    for (const auto& tg: obj->Tags()) {
        vv.push_back(getString(stringtable, tg.val));
    }
    std::list<PbfTag> infmsgs;
    if (includeInfos) {
        
        infmsgs.push_back(PbfTag{1,uint64(obj->Info().version),""});
        infmsgs.push_back(PbfTag{2,uint64(obj->Info().timestamp),""});
        infmsgs.push_back(PbfTag{3,uint64(obj->Info().changeset),""});
        infmsgs.push_back(PbfTag{4,uint64(obj->Info().user_id),""});
        infmsgs.push_back(PbfTag{5,getString(stringtable,obj->Info().user),""});
        /*if (!obj->Info().visible) {
            infmsgs.push_back(PbfTag{6,0,""});
        }*/
    }
    
    if ((!kk.empty()) && (!vv.empty())) {
        if (kk.size()!=vv.size()) {
            throw std::domain_error("tags wrong");
        }
        
        msgs.push_back(PbfTag{2,0,writePackedInt(kk)});
        msgs.push_back(PbfTag{3,0,writePackedInt(vv)});
    }
    if (!infmsgs.empty() && includeInfos) {
        msgs.push_back(PbfTag{4,0,packPbfTags(infmsgs)});
    }
    
    if (obj->Type()==Node) {
        auto nd = std::dynamic_pointer_cast<node>(obj);
        msgs.push_back(PbfTag{8,zigZag(nd->Lat()),""});
        msgs.push_back(PbfTag{9,zigZag(nd->Lon()),""});
    } else if (obj->Type()==Way) {
        auto wy = std::dynamic_pointer_cast<way>(obj);
        msgs.push_back(PbfTag{8, 0, writePackedDelta(wy->Refs())});
    } else if (obj->Type()==Relation) {
        auto rl = std::dynamic_pointer_cast<relation>(obj);
        if (!rl->Members().empty()) {
            msgs.push_back(PbfTag{8,0,writePackedIntFunc<member>(rl->Members(), [&stringtable](const member& m) { return getString(stringtable, m.role); })});
            msgs.push_back(PbfTag{9,0,writePackedDeltaFunc<member>(rl->Members(), [](const member& m) { return m.ref; })});
            msgs.push_back(PbfTag{10,0,writePackedIntFunc<member>(rl->Members(), [](const member& m) { return (uint64) m.type; })});
        }
        
    } else {
        auto geom = std::dynamic_pointer_cast<basegeometry>(obj);
        if (!geom) {
            throw std::domain_error("unexpected type ??"+std::to_string(obj->Type()));
        }
        for (const auto& m: geom->pack_extras()) {
            msgs.push_back(m);
        }
    }
        

    if (includeQts && (obj->Quadtree()>=0)) {
        msgs.push_back(PbfTag{20,zigZag(obj->Quadtree())});
    }

    sortPbfTags(msgs);
    
    return packPbfTags(msgs);
}

template <class Iter>
std::string packDenseNodes(
    std::map<std::string,uint64>& stringtable,
    Iter beg, Iter end, size_t nobj,bool includeQts,bool includeInfos) {

    //size_t nobj = (end-beg);
    std::vector<int64> ids, qts, ts, cs, ui, us, lons,lats;
    std::vector<uint64> vs, /*iv,*/ kvs;

    ids.reserve(nobj);
    if (includeQts) {
        qts.reserve(nobj);
    }
    if (includeInfos) {
        ts.reserve(nobj);
        ui.reserve(nobj);
        us.reserve(nobj);
        vs.reserve(nobj);
        //iv.reserve(nobj);
    }
    lons.reserve(nobj);
    lats.reserve(nobj);

    
    kvs.reserve(nobj*5);
    
    for (auto it=beg; it!=end; ++it) {
        auto obj = std::dynamic_pointer_cast<node>(*it);        
        
        ids.push_back(obj->Id());
        if (includeQts) {
            qts.push_back(obj->Quadtree());
        }

        
        for (const auto& tg: obj->Tags()) {
            getString(stringtable, tg.key);
        }
        for (const auto& tg: obj->Tags()) {
            kvs.push_back(getString(stringtable, tg.key));
            kvs.push_back(getString(stringtable, tg.val));
        }
        kvs.push_back(0);
        
        
        if (includeInfos) {
            vs.push_back(obj->Info().version);
            ts.push_back(obj->Info().timestamp);
            cs.push_back(obj->Info().changeset);
            ui.push_back(obj->Info().user_id);
            us.push_back(getString(stringtable, obj->Info().user));
            //iv.push_back(obj->Info().user_id ? 1 : 0);
        }
        
        lons.push_back(obj->Lon());
        lats.push_back(obj->Lat());
        
    }
    std::list<PbfTag> msgs;

    msgs.push_back(PbfTag{1,0,writePackedDelta(ids)});
    
    if (includeInfos && (!vs.empty())) {
        std::list<PbfTag> infmsgs;
        infmsgs.push_back(PbfTag{1,0,writePackedInt(vs)});
        infmsgs.push_back(PbfTag{2,0,writePackedDelta(ts)});
        infmsgs.push_back(PbfTag{3,0,writePackedDelta(cs)});
        infmsgs.push_back(PbfTag{4,0,writePackedDelta(ui)});
        infmsgs.push_back(PbfTag{5,0,writePackedDelta(us)});
        /*if (size_t(std::count(iv.begin(),iv.end(),1))!=iv.size()) {
            infmsgs.push_back(PbfTag{6,0,writePackedInt(iv)});
        }*/        
        msgs.push_back(PbfTag{5,0,packPbfTags(infmsgs)});
    }

    msgs.push_back(PbfTag{8,0,writePackedDelta(lats)});
    msgs.push_back(PbfTag{9,0,writePackedDelta(lons)});
    msgs.push_back(PbfTag{10,0,writePackedInt(kvs)});
    if (includeQts && (size_t(std::count(qts.begin(),qts.end(),-1))!=qts.size())) {
        msgs.push_back(PbfTag{20,0,writePackedDelta(qts)});
    }

    return packPbfTags(msgs);
}



template <class Iter>
std::string packPrimitiveGroup(
    std::map<std::string,uint64>& stringtable,
    Iter beg, Iter end,size_t nobj,uint64 ct,
    bool includeQts, bool includeInfos, bool includeRefs) {

    std::list<PbfTag> msgs;

    if ((*beg)->Type()==0) {
        msgs.push_back(PbfTag{2,0,packDenseNodes(stringtable,beg,end,nobj,includeQts,includeInfos)});
    } else {
        for (auto it=beg; it != end; ++it) {
            if ((*beg)->Type()==0) {
                msgs.push_back(PbfTag{1,0,packObject(stringtable,*it,includeQts,includeInfos,includeRefs)});
            } else if ((*beg)->Type()==1) {
                msgs.push_back(PbfTag{3,0,packObject(stringtable,*it,includeQts,includeInfos,includeRefs)});
            } else if ((*beg)->Type()==2) {
                msgs.push_back(PbfTag{4,0,packObject(stringtable,*it,includeQts,includeInfos,includeRefs)});
            } else {
                uint64 ty = (*beg)->Type();
                msgs.push_back(PbfTag{ty+17,0,packObject(stringtable,*it,includeQts,includeInfos,includeRefs)});
            }
        }
    }
    if (ct>0) {
        msgs.push_back(PbfTag{10,ct,""});
    }

    return packPbfTags(msgs);
}



std::string packStringTable(const std::map<std::string,uint64>& stringtable) {
    std::vector<std::string> st(stringtable.size());
    for (const auto& s: stringtable) {
        st.at(s.second)=s.first;
    }
    st[0]="";
    std::list<PbfTag> msgs;
    for (const auto& s: st) {
        msgs.push_back(PbfTag{1,0,s});
    }
    return packPbfTags(msgs,true);
}

std::string packQuadtree(int64 qt) {

    auto t = quadtree::tuple(qt);
    std::list<PbfTag> msgs;
    /*msgs.push_back(PbfTag{1,uint64(t.x),""});
    msgs.push_back(PbfTag{2,uint64(t.y),""});
    msgs.push_back(PbfTag{3,uint64(t.z),""});*/
    msgs.push_back(PbfTag{1,uint64(std::get<0>(t)),""});
    msgs.push_back(PbfTag{2,uint64(std::get<1>(t)),""});
    msgs.push_back(PbfTag{3,uint64(std::get<2>(t)),""});
    
    return packPbfTags(msgs);
}






std::string packHeaderBbox(const bbox& bb) {
    std::list<PbfTag> msgs;
    msgs.push_back(PbfTag{1,zigZag(bb.minx*100),""}); //left
    msgs.push_back(PbfTag{2,zigZag(bb.maxx*100),""}); //right
    msgs.push_back(PbfTag{3,zigZag(bb.maxy*100),""}); //top
    msgs.push_back(PbfTag{4,zigZag(bb.miny*100),""});//bottom
    return packPbfTags(msgs);
}

std::string packBlockIdx(int64 qt, bool isc, int64 len) {
    std::list<PbfTag> msgs;
    //msgs.push_back(PbfTag{1,0,writeblock_detail::packQuadtree(qt)});
    msgs.push_back(PbfTag{2,uint64(isc?1:0),""});
    msgs.push_back(PbfTag{3,zigZag(len),""});
    msgs.push_back(PbfTag{4,zigZag(qt),""});
    return packPbfTags(msgs);
}
}
std::string writePbfHeader(std::shared_ptr<header> head) {
    std::list<PbfTag> msgs;
    msgs.push_back(PbfTag{1,0,writeblock_detail::packHeaderBbox(head->box)});
    if (head->features.empty()) {
        msgs.push_back(PbfTag{4,0,"OsmSchema-V0.6"});
        msgs.push_back(PbfTag{4,0,"DenseNodes"});
    } else {
        for (auto& f: head->features) {
            msgs.push_back(PbfTag{4,0,f});
        }
    }
    if (head->writer.empty()) {
        msgs.push_back(PbfTag{16,0,"osmquadtree-cpp"});
    } else {
        msgs.push_back(PbfTag{16,0,head->writer});
    }

    if (!head->index.empty()) {
        for (auto& ii: head->index) {
            int64 qt,fl,ln;
            std::tie(qt,fl,ln)=ii;
            msgs.push_back(PbfTag{22,0,writeblock_detail::packBlockIdx(qt,false,ln)});
        }
    }
    return packPbfTags(msgs);
}

std::string writePbfBlock(std::shared_ptr<primitiveblock> block, bool includeQts, bool change, bool includeInfo, bool includeRefs) {
    
    std::list<PbfTag> msgs;

    std::map<std::string,uint64> stringtable;
    stringtable["%%$6535ff"]=0;

    if (!block->objects.empty()) {
        auto it=block->objects.begin();
        while (it < block->objects.end()) {
            
            auto jt=it;
            ++jt;
            while ( jt!=block->objects.end() && ((*it)->Type()==(*jt)->Type()) && ( (!change) || ((*it)->ChangeType()==(*jt)->ChangeType()))) {
                ++jt;
            }

            msgs.push_back(PbfTag{2,0,writeblock_detail::packPrimitiveGroup(stringtable,it,jt,jt-it,change ? (*it)->ChangeType() : 0, includeQts, includeInfo, includeRefs)});
            it=jt;
        }
    }


    msgs.push_front(PbfTag{1,0,writeblock_detail::packStringTable(stringtable)});

    if (block->quadtree>=0) {
        msgs.push_back(PbfTag{32,zigZag(block->quadtree),""});
    }
    if (block->startdate>0) {
        msgs.push_back(PbfTag{33,uint64(block->startdate),""});
    }
    if (block->enddate>0) {
        msgs.push_back(PbfTag{34,uint64(block->enddate),""});
    }

    return packPbfTags(msgs);
    
    
}



}
