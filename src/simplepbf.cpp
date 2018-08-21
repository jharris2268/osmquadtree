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
namespace oqt {
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

uint64 readUVarint(const std::string& data, size_t& pos) {
    uint64 result = 0;
    int count = 0;
    uint8_t b=0;

    do {
        if (count == 10) return 0;
        b = data[(pos)++];

        result |= (static_cast<uint64_t>(b & 0x7F) << (7 * count));

        ++count;
    } while (b & 0x80);

    return result;
}



int64 unZigZag(uint64 uv) {
    int64 x = (int64) (uv>>1);
    if ((uv&1)!=0) {
        x = x^(-1);
    }
    return x;
}

int64 readVarint(const std::string& data, size_t& pos) {
    uint64_t uv = readUVarint(data,pos);
    return unZigZag(uv);
}

std::string readData(const std::string& data, size_t& pos) {
    size_t uv = readUVarint(data,pos);
    
    size_t p = pos;
    pos += uv;
    return std::move(data.substr(p,uv));
}

PbfTag readPbfTag(const std::string& data, size_t& pos) {
    if ((pos) == data.size()) {
        return PbfTag{0,0,""};
    }
    uint64_t tag = readUVarint(data,pos);
    
    if ((tag&7) == 0) {
        return PbfTag{tag>>3, readUVarint(data,pos), ""};
    } else if ((tag&7)==2) {
        return PbfTag{tag>>3, 0, readData(data,pos)};
    }
    logger_message() << "?? @ " << pos << "/" << data.size() << ": " << tag << " "  << (tag&7) << " " << (tag>>3);

    throw std::domain_error("only understand varint & data");
    return PbfTag{0,0,""};
}


std::vector<int64> readPackedDelta(const std::string& data) {
    size_t pos = 0;
    std::vector<int64> ans;

    int64 curr=0;
    size_t sz=0;
    for (auto it=data.begin(); it < data.end(); ++it) {
        if ((unsigned char)(*it) < 127) {
            sz++;
        }
    }
    ans.reserve(sz);

    while (pos<data.size()) {
        curr += readVarint(data,pos);
        ans.push_back(curr);
    }
    return std::move(ans);
}


std::vector<uint64> readPackedInt(const std::string& data)
{
    size_t pos = 0;
    std::vector<uint64> ans;
    size_t sz=0;
    for (auto it=data.begin(); it < data.end(); ++it) {
        if ((unsigned char)(*it) < 127) {
            sz++;
        }
    }
    ans.reserve(sz);

    while (pos<data.size()) {
        ans.push_back(readUVarint(data,pos));
    }
    return std::move(ans);
}




size_t writeVarint(std::string& data, size_t pos, int64 v) {

    return writeUVarint(data,pos,zigZag(v));
}
size_t writeUVarint(std::string& data, size_t pos, uint64 value) {
    while (value > 0x7F) {
      data[pos++] = (static_cast<char>(value) & 0x7F) | 0x80;
      value >>= 7;
    }
    data[pos++] = static_cast<char>(value) & 0x7F;

    return pos;
}

size_t UVarintLength(uint64 value) {
    for (size_t i=1; i < 11; i++) {
        if (value < (1ull << (7*i))) {
            return i;
        }
    }
    return 0xffffffffffffffffull;
}

size_t packedDeltaLength(const std::vector<int64>& vals, size_t last) {
    size_t l=0;
    int64 p =0;
    if (last == 0) { last = vals.size(); }
    for (size_t i=0; i < last; i++) {
        l += UVarintLength(zigZag(vals[i]-p));
        p=vals[i];
    }
    return l;
}


size_t writePackedDeltaInPlace(std::string& out, size_t pos, const std::vector<int64>& vals, size_t last) {
    int64 p=0;
    if (last == 0) { last = vals.size(); }
    for (size_t i=0; i < last; i++) {
        pos=writeVarint(out,pos,vals[i]-p);
        p=vals[i];
    }
    return pos;
}

size_t packedDeltaLength_func(std::function<int64(size_t)> func, size_t len) {
    size_t l=0;
    int64 p=0;
    for (size_t i=0; i < len; i++) {
        int64 q = func(i);
        l += UVarintLength(zigZag(q-p));
        p=q;
    }
    return l;
}
    
size_t writePackedDeltaInPlace_func(std::string& out, size_t pos, std::function<int64(size_t)> func, size_t len) {
    int64 p=0;
    for (size_t i=0; i < len; i++) {
        int64 q=func(i);
        pos=writeVarint(out,pos,q-p);
        p=q;
    }
    return pos;
}



std::string writePackedDelta(const std::vector<int64>& vals) {
    std::string out(10*vals.size(),0);
    size_t pos=writePackedDeltaInPlace(out,0,vals,0);
    out.resize(pos);
    return out;
}


std::string writePackedInt(const std::vector<uint64>& vals) {
    std::string out(10*vals.size(),0);
    size_t pos=0;

    for (auto v: vals) {
        pos=writeUVarint(out,pos,v);
    }
    out.resize(pos);
    return out;
}


size_t writePbfValue(std::string& data, size_t pos, uint64 t, uint64 v) {
    pos = writeUVarint(data,pos, t<<3);
    pos = writeUVarint(data,pos, v);
    return pos;
}


size_t pbfValueLength(uint64 t, size_t val) {
    return UVarintLength( (t<<3)) + UVarintLength(val);
}


size_t pbfDataLength(uint64 t, size_t data_len) {
    return UVarintLength( (t<<3)|2 ) + UVarintLength(data_len) + data_len;
}

size_t writePbfDataHeader(std::string& data, size_t pos, uint64 t, size_t ln) {
    pos = writeUVarint(data,pos, (t<<3) | 2);
    pos = writeUVarint(data,pos, ln);
    return pos;
}

size_t writePbfData(std::string& data, size_t pos, uint64 t, const std::string& v) {
    pos = writeUVarint(data,pos, (t<<3) | 2);
    pos = writeUVarint(data,pos, v.size());
    std::copy(v.begin(),v.end(),data.begin()+pos);
    pos+=v.size();
    return pos;
}


void sortPbfTags(std::list<PbfTag>& msgs) {
    msgs.sort([](const PbfTag& l, const PbfTag& r) { return l.tag<r.tag; });
}

std::string packPbfTags(const std::list<PbfTag>& msgs, bool forceData) {

    size_t ln=0;
    for (const auto& mm: msgs) {
        ln+=20;
        if (!mm.data.empty()) {
            ln+=mm.data.size();
        }
    }

    std::string out(ln,0);
    size_t pos=0;
    for (const auto& mm: msgs) {
        if (!mm.data.empty() || forceData) {
            pos=writePbfData(out,pos,mm.tag,mm.data);
        } else {
            pos=writePbfValue(out,pos,mm.tag,mm.value);
        }
    }
    out.resize(pos);
    return out;
}


uint64 zigZag(int64 v) {
    return (static_cast<uint64>(v) << 1) ^ (v >> 63);

}

packedobj_ptr make_packedobj_ptr(uint64 id, int64 quadtree, const std::string& data, changetype ct) {
    //return packedobj_ptr(new packedobj{id,quadtree,data,changetype})    ;
    return std::make_shared<packedobj>(id,quadtree,data,ct);
}

packedobj_ptr element::pack() {

    std::list<PbfTag> mm;
    for (auto tg: tags_) {
        mm.push_back(PbfTag{2,0,tg.key});
    }
    for (auto tg: tags_) {
        mm.push_back(PbfTag{3,0,tg.val});
    }
    mm.push_back(PbfTag{41,uint64(info_.version),""});
    mm.push_back(PbfTag{42,uint64(info_.timestamp),""});
    mm.push_back(PbfTag{43,uint64(info_.changeset),""});
    mm.push_back(PbfTag{44,uint64(info_.user_id),""});
    mm.push_back(PbfTag{45,0, info_.user});
    if (!info_.visible) {
        mm.push_back(PbfTag{46,0,""});
    }
    mm.splice(mm.end(),pack_extras());
    std::string out=packPbfTags(mm);
    
    return make_packedobj_ptr(InternalId(), quadtree_, out, changetype_);
}

std::list<PbfTag> node::pack_extras() const {
    return {PbfTag{8,zigZag(lat_),""},PbfTag{9,zigZag(lon_),""}};
}

std::list<PbfTag> way::pack_extras() const {
    return {PbfTag{8,0,writePackedDelta(refs_)}};
}

std::list<PbfTag> relation::pack_extras() const {
    std::list<PbfTag> mm;
    if (mems_.empty()) { return mm; }
    std::vector<int64> rfs_; rfs_.reserve(mems_.size());
    std::vector<uint64> tys_; tys_.reserve(mems_.size());

    for (auto m : mems_) {
        mm.push_back(PbfTag{8,0,m.role});
        rfs_.push_back(m.ref);
        tys_.push_back(m.type);
    }
    mm.push_back(PbfTag{9,0,writePackedDelta(rfs_)});
    mm.push_back(PbfTag{10,0,writePackedInt(tys_)});
    return mm;
}

/*
packedobj_ptr node::pack() {
    packedobj_ptr out = element::pack();
    
    std::string data=out->Data() + packPbfTags({PbfTag{8,zigZag(lat_),""},PbfTag{9,zigZag(lon_),""}});
    return make_packedobj_ptr(out->InternalId(), out->Quadtree(), data, out->ChangeType());
}

packedobj_ptr way::pack() {
    packedobj_ptr out = element::pack();
    
    std::string data=out->Data() + packPbfTags({PbfTag{8,0,writePackedDelta(refs_)}});
    return make_packedobj_ptr(out->InternalId(), out->Quadtree(), data,out->ChangeType());
}

packedobj_ptr relation::pack() {
    packedobj_ptr out = element::pack();

    std::string mmp;
    if (!mems_.empty()) {
        bool isv=true;//false;
        for (auto m : mems_) {
            if (!m.role.empty()) {
                isv=true;
                break;
            }
        }

        std::list<PbfTag> mm;
        //mm.reserve(2+(isv ? mems_.size():0));

        std::vector<int64> rfs_; rfs_.reserve(mems_.size());
        std::vector<uint64> tys_; tys_.reserve(mems_.size());

        for (auto m : mems_) {
            if (isv) {
                mm.push_back(PbfTag{8,0,m.role});
            }
            rfs_.push_back(m.ref);
            tys_.push_back(m.type);
        }
        mm.push_back(PbfTag{9,0,writePackedDelta(rfs_)});
        mm.push_back(PbfTag{10,0,writePackedInt(tys_)});
        mmp = packPbfTags(mm);
    }
    std::string data = out->Data() + mmp;
    uint64 id = (uint64(2)<<61) | (out->InternalId());

    return make_packedobj_ptr(id, out->Quadtree(), data,out->ChangeType());
}
*/



bool relation::filter_members(std::shared_ptr<idset> ids) {
    if (mems_.empty()) { return true; }
    memvector mems_new;
    mems_new.reserve(mems_.size());
    
    for (auto& m: mems_) {
        if (ids->contains(m.type, m.ref)) {
            mems_new.push_back(m);
        }
    }
    if (mems_.size()==mems_new.size()) {
        return false;
    }
    //mems_.swap(mems_new);
    mems_=mems_new;
    return true;
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

std::list<PbfTag> readAllPbfTags(const std::string& data) {
    size_t pos=0;
    std::list<PbfTag> mm;
    PbfTag tg=readPbfTag(data,pos);
    for ( ; tg.tag>0; tg=readPbfTag(data,pos)) {
        mm.push_back(tg);
    }
    return mm;
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

std::string fix_str(const std::string& s) {
    
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string t = converter.from_bytes(s);
    std::u32string u;
    u.reserve(t.size());    
    for (const auto& c: t) {
        if (c!=127) {
            u.push_back(c);
        }
    }
    return converter.to_bytes(u);
}
        
bool fix_tags(element& ele) {
    if (ele.tags_.empty()) { return false; }
    std::sort(ele.tags_.begin(),ele.tags_.end(),[](const tag&l, const tag& r) { return l.key<r.key; });
    bool r=false;
    for (auto& t: ele.tags_) {
        if (t.key.find(127)!=std::string::npos) {
            auto s=fix_str(t.key);
            if (s!=t.key) {
                t.key=s;
                r=true;
            }
        }
        if (t.val.find(127)!=std::string::npos) {
            auto s=fix_str(t.val);
            if (s!=t.val) {
                t.val=s;
                r=true;
            }
        }
    }
    return r;
}

bool fix_members(relation& rel) {
    if (rel.mems_.empty()) { return false; }
    bool r=false;
    for (auto& m: rel.mems_) {
        if ((!m.role.empty()) && (m.role.find(127)!=std::string::npos)) {
            auto s=fix_str(m.role);
            if (s!=m.role) {
                m.role=s;
                r=true;
            }
        }
    }
    return r;
}

bool isGeometryType(elementtype ty) {
    switch (ty) {
        case Node: return false;
        case Way: return false;
        case Relation: return false;
        case Point: return true;
        case Linestring: return true;
        case SimplePolygon: return true;
        case ComplicatedPolygon: return true;
        case WayWithNodes: return false;
        case Unknown: return false;
    }
    return false;
}
}
