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

#include <map>
#include <unordered_map>
#include <algorithm>

namespace oqt {
namespace geometry {



class LonLatStoreImpl : public LonLatStore {
    //typedef std::unordered_map<int64,lonlat> loctile;
    typedef std::pair<int64,lonlat> llp;
    typedef std::vector<llp> loctile;
    std::map<int64,loctile> tiles;
    lonlat get_ll(int64 n) {
        for (auto& tile : tiles) {
            if (tile.second.empty()) {
                continue;
            }
            auto it = std::lower_bound(tile.second.begin(),tile.second.end(),std::make_pair(n,lonlat{0,0}),[](const llp& l, const llp& r)->bool { return l.first<r.first; });
            if (it < tile.second.end()) {
                if (it->first==n) {
                    return it->second;
                }
            }
        }
        throw std::range_error("ref not present");
        return lonlat{0,0};
    }


    public:
        LonLatStoreImpl() {}
        virtual ~LonLatStoreImpl() {}
        virtual void add_tile(PrimitiveBlockPtr block) {

            /*
            std::vector<int64> dels;
            for (auto& cc : tiles) {
                if (quadtree::common(block->quadtree, cc.first) != cc.first) {
                    dels.push_back(cc.first);
                }
            }
            for (auto c: dels) {
                tiles.erase(c);
            }*/
            for (auto it=tiles.cbegin(); it!=tiles.cend(); ) {
                if (quadtree::common(block->Quadtree(), it->first) != it->first) {
                    it=tiles.erase(it);
                } else {
                    it++;
                }
            }


            loctile lls; lls.reserve(block->size());
            for (auto& o : block->Objects()) {
                if (o->Type()==ElementType::Node) {
                    auto n = std::dynamic_pointer_cast<Node>(o);
                    //lls.insert(std::make_pair(o->Id(), lonlat{n->Lon(),n->Lat()}));
                    lls.push_back(std::make_pair(o->Id(), lonlat{n->Lon(),n->Lat()}));
                }
            }
            if (!lls.empty()) {
                tiles[block->Quadtree()] = lls;
            }
        }

        virtual lonlatvec get_lonlats(std::shared_ptr<Way> way) {
            lonlatvec res;
            res.reserve(way->Refs().size());
            for (auto& r : way->Refs()) {
                res.push_back(get_ll(r));
            }
            return res;
        }
        virtual void finish() {
            tiles.clear();
            std::map<int64,loctile> o;
            tiles.swap(o);
        }
};

std::shared_ptr<LonLatStore> make_lonlatstore() {
    return std::make_shared<LonLatStoreImpl>();
}


PrimitiveBlockPtr add_waynodes(std::shared_ptr<LonLatStore> lls, PrimitiveBlockPtr in_bl) {
    lls->add_tile(in_bl);
    auto out_bl = std::make_shared<PrimitiveBlock>(in_bl->Index(), in_bl->size());
    out_bl->SetQuadtree(in_bl->Quadtree());
    out_bl->SetStartDate(in_bl->StartDate());
    out_bl->SetEndDate(in_bl->EndDate());

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
        cb(add_waynodes(lls, bl));
    };
}

/*
void add_waynodes_process(
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> in,
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> out,
    std::shared_ptr<lonlatstore> lls) {

    size_t in_index=0;
    size_t out_index=0;



    for (
        auto in_bl = in[in_index%in.size()]->wait_and_pop(); in_bl;
             in_bl = in[in_index%in.size()]->wait_and_pop()) {

        auto out_bl = add_waynodes(lls, in_bl);
        out[out_index % out.size()]->wait_and_push(out_bl);
        in_index++;
        out_index++;
    }

    for (auto o:out) {
        o->wait_and_finish();
    }

}
*/

xy forward_transform(int64 ln, int64 lt) {
    double x = toFloat(ln)*earth_width / 180; 
    double y = merc(toFloat(lt), earth_width);
    return xy{round(x*100)/100, round(y*100)/100};
}

lonlat inverse_transform(double x, double y) {
    return lonlat{toInt(x*180 /earth_width), toInt(unMerc(y, earth_width))};
}

}}


