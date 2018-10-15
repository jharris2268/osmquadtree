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

#include "oqt/calcqts/waynodes.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/pbfformat/fileblock.hpp"

#include <algorithm>

namespace oqt {
    
    

template <class T1, class T2>
bool sort_cmp_first(const std::pair<T1,T2>& l, const std::pair<T1,T2>& r) {
    return l.first < r.first;
}


class WayNodesImpl : public WayNodesWrite {
    
    
    public:
        WayNodesImpl(size_t cap) : waynodes(cap), l(0),key_(-1) {};
        WayNodesImpl(size_t cap, int64 k) : waynodes(cap), l(0),key_(k) {};


        virtual ~WayNodesImpl() {}


        virtual bool add(int64 w, int64 n, bool resize) {
            if (l==waynodes.size()) {
                if (resize) {
                    waynodes.resize(waynodes.size()+16384);
                    Logger::Message() << "resize waynode to " << waynodes.size()+16384;
                } else {
                    throw std::range_error("way_nodes_impl::add with resize=false");
                }
            }
            waynodes[l] = std::make_pair(w,n);
            l++;
            return l==waynodes.size();
        }
        
        
        virtual void reserve(size_t n) {
            if (n > (waynodes.size()-l)) {
                waynodes.resize(l+n);
            }
        }
        
        
        virtual void reset(bool empty) {
            l=0;
            if (empty) {
                std::vector<std::pair<int64,int64>> e;
                waynodes.swap(e);
            }
        };
        virtual size_t size() const { return l; }
        
        virtual void sort_way() {
            if (l==0) { return; }
            std::sort(waynodes.begin(),waynodes.begin()+l, [](const wn& lft, const wn& rht) { return std::tie(lft.first,lft.second) < std::tie(rht.first,rht.second); });
        }

        virtual void sort_node() {
            if (l==0) { return; }
            std::sort(waynodes.begin(),waynodes.begin()+l, [](const wn& lft, const wn& rht) { return std::tie(lft.second,lft.first) < std::tie(rht.second,rht.first); });
        }
        
        virtual int64 key() const { return key_; }
        
        
        virtual std::string str() const {
            std::stringstream ss;
            ss << "way_nodes_impl(" << key_ << " " << l << " objs [";
            if (l>0) {
                
                ss << way_at(0) << ", " << node_at(0);
                if (l>1) {
                    
                    ss << " => " << way_at(l-1) << ", " << node_at(l-1);
                }
            }
            ss<<"]";
            return ss.str();
        }
        
        virtual int64 way_at(size_t i) const { return waynodes.at(i).first; }
        virtual int64 node_at(size_t i) const { return waynodes.at(i).second; }

    private:
        typedef std::pair<int64,int64> wn;
        std::vector<wn> waynodes;
        size_t l;
        int64 key_;
       
};

std::shared_ptr<WayNodesWrite> make_way_nodes_write(size_t cap, int64 key) {
    return std::make_shared<WayNodesImpl>(cap,key);
}


keystring_ptr pack_waynodes_block(std::shared_ptr<WayNodes> tile) {
    size_t sz=tile->size();
    
    size_t node_len = packedDeltaLength_func([&tile](size_t i) { return tile->node_at(i); }, sz);
    size_t way_len = packedDeltaLength_func([&tile](size_t i) { return tile->way_at(i); }, sz);
    
    size_t len = pbfValueLength(1, zigZag(tile->key()))
        + pbfDataLength(2, node_len)
        + pbfDataLength(3, way_len)
        + pbfValueLength(4, sz);
    
    std::string buf(len,0);
    size_t pos=0;
    
    pos = writePbfValue(buf, pos, 1, zigZag(tile->key()));
    
    pos = writePbfDataHeader(buf, pos, 2, node_len);
    pos = writePackedDeltaInPlace_func(buf, pos, [&tile](size_t i) { return tile->node_at(i); }, sz);
    
    pos = writePbfDataHeader(buf, pos, 3, way_len);
    pos = writePackedDeltaInPlace_func(buf, pos, [&tile](size_t i) { return tile->way_at(i); }, sz);
    
    pos = writePbfValue(buf, pos, 4, sz);
    
    if (pos!=len) {
        Logger::Message() << "pack_noderefs_try2 failed (key=" << tile->key() << ", sz=" << sz << " node_len=" << node_len << ", way_len=" << way_len << ", len=" << len << " != pos=" << pos;
        throw std::domain_error("failed");
    }
    
    return std::make_shared<keystring>(tile->key(), prepare_file_block("WayNodes", buf, 1));
    
}
    
void unpack_noderefs(const std::string& data, int64 key, WayNodesImpl& waynodes, int64 minway, int64 maxway) {
    size_t pos=0;
    PbfTag tag = readPbfTag(data,pos);
    std::string nn,ww;

    for ( ; tag.tag>0; tag=readPbfTag(data,pos)) {
        if (tag.tag==1) {
            if (unZigZag(tag.value)!=key) {
                throw std::domain_error("wrong key");
            }
        } else if (tag.tag==2) {
            nn = tag.data;
        } else if (tag.tag==3) {
            ww = tag.data;

        } else if (tag.tag==4) {
            waynodes.reserve(tag.value);
        }
    }

    size_t np=0;
    size_t wp=0;
    int64 nd=0,wy=0;

    while (np < nn.size()) {
        nd += readVarint(nn,np);
        wy += readVarint(ww,wp);
        if ((wy>=minway) && ((maxway==0) || (wy<maxway))) {
            waynodes.add(wy,nd,true);
        }
   }
}

std::shared_ptr<WayNodes> read_waynodes_block(const std::string& data, int64 minway, int64 maxway) {
    size_t pos=0;
    PbfTag tag = readPbfTag(data,pos);
    std::string nn,ww;
    int64 ky=-1;
    size_t sz=0;
    for ( ; tag.tag>0; tag=readPbfTag(data,pos)) {
        if (tag.tag==1) {
            ky = unZigZag(tag.value);
        } else if (tag.tag==2) {
            nn = tag.data;
        } else if (tag.tag==3) {
            ww = tag.data;

        } else if (tag.tag==4) {
            sz=tag.value;
        }
    }

    size_t np=0;
    size_t wp=0;
    int64 nd=0,wy=0;
    
    auto res = std::make_shared<WayNodesImpl>(sz,ky);
    
    while (np < nn.size()) {
        nd += readVarint(nn,np);
        wy += readVarint(ww,wp);
        if ((wy>=minway) && ((maxway==0) || (wy<maxway))) {
            res->add(wy,nd,true);
        }
   }
   return res;
}


}
