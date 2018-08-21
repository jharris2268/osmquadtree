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

void readPrimitiveGroup(const std::string& data, const std::vector<std::string>& stringtable, std::vector<std::shared_ptr<element> >& objects, bool change,size_t objflags, std::shared_ptr<idset> ids,read_geometry_func readGeometry);

tagvector makeTags(
    const std::vector<uint64>& keys, const std::vector<uint64>& vals,
    const std::vector<std::string>& stringtable);

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
