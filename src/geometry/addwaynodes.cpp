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

#include "oqt/geometry/addwaynodes.hpp"
#include "oqt/elements/quadtree.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/splitcallback.hpp"


#include <map>
#include <unordered_map>
#include <algorithm>

namespace oqt {
namespace geometry {

//typedef std::map<int64,LonLat> loctile;
typedef std::pair<int64,LonLat> llp;
typedef std::vector<llp> loctile;

LonLat get_ll(const std::map<int64,std::shared_ptr<loctile>>& tiles, int64 n) {
    for (auto jt=tiles.rbegin(); jt!= tiles.rend(); ++jt) {
        const auto& tile = *jt;
        
        if (tile.second->empty()) {
            continue;
        }
        
        /*auto it=tile.second->find(n);
        if (it!=tile.second->end()) {
            return it->second;
        }*/
        
        auto it = std::lower_bound(tile.second->begin(),tile.second->end(),std::make_pair(n,LonLat{0,0}),[](const llp& l, const llp& r)->bool { return l.first<r.first; });
        if (it < tile.second->end()) {
            if (it->first==n) {
                return it->second;
            }
        }
    }
    throw std::range_error("ref not present");
    return LonLat{0,0};
}
std::vector<LonLat> get_lonlats(const std::map<int64,std::shared_ptr<loctile>>& tiles, std::shared_ptr<Way> way) {
            
    std::vector<LonLat> res;
    res.reserve(way->Refs().size());
    for (auto& r : way->Refs()) {
        res.push_back(get_ll(tiles, r));
    }
    return res;
}

void populate_tile(loctile& lls, PrimitiveBlockPtr block) {
            
    lls.reserve(block->size());
    for (auto& o : block->Objects()) {
        if (o->Type()==ElementType::Node) {
            auto n = std::dynamic_pointer_cast<Node>(o);
            //lls.insert(std::make_pair(o->Id(), LonLat{n->Lon(),n->Lat()}));
            lls.push_back(std::make_pair(o->Id(), LonLat{n->Lon(),n->Lat()}));
        }
    }
    if (!lls.empty()) {
        std::sort(lls.begin(),lls.end(),[](const llp& l, const llp& r)->bool { return l.first<r.first; });
    }
    
}


class LonLatStoreImpl : public LonLatStore {
    
        
        
    
    public:
        LonLatStoreImpl() {}
        virtual ~LonLatStoreImpl() {}
        
        
                
        void add_populated_tile(int64 qt, std::shared_ptr<loctile> lls) {
            for (auto it=tiles.cbegin(); it!=tiles.cend(); ) {
                if (quadtree::common(qt, it->first) != it->first) {
                    it=tiles.erase(it);
                } else {
                    it++;
                }
            }

            
            if (!lls->empty()) {
                tiles[qt] = lls;
            }
        }
        
        virtual void add_tile(PrimitiveBlockPtr block) {
            if (!block) {
                Logger::Message() << "add_tile empty block??"; 
            }
            auto lls = std::make_shared<loctile>();
            populate_tile(*lls, block);
            add_populated_tile(block->Quadtree(), lls);
        }


            

        virtual std::vector<LonLat> get_lonlats(std::shared_ptr<Way> way) {
            return oqt::geometry::get_lonlats(tiles, way);           
        }
        
        virtual void finish() {
            tiles.clear();
            std::map<int64,std::shared_ptr<loctile>> o;
            tiles.swap(o);
        }
    private:
        std::map<int64,std::shared_ptr<loctile>> tiles;
};

std::shared_ptr<LonLatStore> make_lonlatstore() {
    return std::make_shared<LonLatStoreImpl>();
}


//PrimitiveBlockPtr add_waynodes(const std::map<int64,std::shared_ptr<loctile>>& tiles, PrimitiveBlockPtr in_bl) {
PrimitiveBlockPtr add_waynodes(std::shared_ptr<LonLatStore> lls, PrimitiveBlockPtr in_bl) {
    if (!in_bl) { 
        Logger::Message() << "add_waynodes empty block??";         
        return in_bl;
    }
    
    auto out_bl = std::make_shared<PrimitiveBlock>(in_bl->Index(), in_bl->size());
    out_bl->CopyMetadata(in_bl);
    
    
    for (auto& o : in_bl->Objects()) {
        if (o->Type()==ElementType::Node) {
            if (!o->Tags().empty()) {
                out_bl->add(o);
            }
        } else if (o->Type()==ElementType::Way) {
            auto w = std::dynamic_pointer_cast<Way>(o);
            auto wwn = std::make_shared<WayWithNodes>(w, lls->get_lonlats(w));
            out_bl->add(wwn);
        } else {
            out_bl->add(o);
        }
    }
    return out_bl;
}

block_callback make_addwaynodes_cb(block_callback cb) {
    auto lls = make_lonlatstore();
    return [lls,cb](PrimitiveBlockPtr bl) {
        if (!bl) {
            lls->finish();
            cb(nullptr);
            return;
        }
        lls->add_tile(bl);
        cb(add_waynodes(lls, bl));
    };
}

/*
struct loctile_pb {
    std::map<int64,std::shared_ptr<loctile>> tiles;
    PrimitiveBlockPtr pb;
};*/

block_callback make_waynodes_cb_split(block_callback cb) {
    Logger::Message() << "splitting add waynodes doesn't help";
    return make_addwaynodes_cb(cb);
    
    /*
    auto lls = std::make_shared<LonLatStoreImpl>();
    auto second=threaded_callback<loctile_pb>::make(
        [cb](std::shared_ptr<loctile_pb> bl) {
            if (!bl) {
                
                cb(nullptr);
                return;
            }
            
            cb(add_waynodes(bl->tiles, bl->pb));
        }
     );
    
    return [lls,second](PrimitiveBlockPtr bl) {
        if (!bl) {
            lls->finish();
            second(nullptr);
            return;
        }
        lls->add_tile(bl);
        
        auto ltp = std::make_shared<loctile_pb>();
        ltp->pb = bl;
        ltp->tiles=lls->get_tiles();
        
        second(ltp);
    };
    */
}

}}


