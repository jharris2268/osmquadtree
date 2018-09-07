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
#include <map>
#include <unordered_map>
#include <algorithm>

namespace oqt {
namespace geometry {


std::list<PbfTag> way_withnodes::pack_extras() const {
    

    std::list<PbfTag> extras;
    extras.push_back(PbfTag{8,0,writePackedDelta(refs)}); //refs
    extras.push_back(PbfTag{12,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lon; })}); //lons
    extras.push_back(PbfTag{13,0,writePackedDeltaFunc<lonlat>(lonlats,[](const lonlat& l)->int64 { return l.lat; })}); //lats
    return extras;
}


class lonlatstore_impl : public lonlatstore {
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
        lonlatstore_impl() {}

        virtual void add_tile(std::shared_ptr<primitiveblock> block) {

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
                if (quadtree::common(block->quadtree, it->first) != it->first) {
                    it=tiles.erase(it);
                } else {
                    it++;
                }
            }


            loctile lls; lls.reserve(block->objects.size());
            for (auto& o : block->objects) {
                if (o->Type()==0) {
                    auto n = std::dynamic_pointer_cast<node>(o);
                    //lls.insert(std::make_pair(o->Id(), lonlat{n->Lon(),n->Lat()}));
                    lls.push_back(std::make_pair(o->Id(), lonlat{n->Lon(),n->Lat()}));
                }
            }
            if (!lls.empty()) {
                tiles[block->quadtree] = lls;
            }
        }

        virtual lonlatvec get_lonlats(std::shared_ptr<way> way) {
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

std::shared_ptr<lonlatstore> make_lonlatstore() {
    return std::make_shared<lonlatstore_impl>();
}


std::shared_ptr<primitiveblock> add_waynodes(std::shared_ptr<lonlatstore> lls, std::shared_ptr<primitiveblock> in_bl) {
    lls->add_tile(in_bl);
    auto out_bl = std::make_shared<primitiveblock>(in_bl->index, in_bl->objects.size());
    out_bl->quadtree = in_bl->quadtree;
    out_bl->startdate = in_bl->startdate;
    out_bl->enddate = in_bl->enddate;

    for (auto& o : in_bl->objects) {
        if (o->Type()==0) {
            if (!o->Tags().empty()) {
                out_bl->objects.push_back(o);
            }
        } else if (o->Type()==1) {
            auto w = std::dynamic_pointer_cast<way>(o);
            auto wwn = std::make_shared<way_withnodes>(w, lls->get_lonlats(w));
            out_bl->objects.push_back(wwn);
        } else {
            out_bl->objects.push_back(o);
        }
    }
    return out_bl;
}

block_callback make_addwaynodes_cb(block_callback cb) {
    auto lls = make_lonlatstore();
    return [lls,cb](primitiveblock_ptr bl) {
        if (!bl) {
            lls->finish();
            cb(nullptr);
            return;
        }
        cb(add_waynodes(lls, bl));
    };
}


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


xy forward_transform(int64 ln, int64 lt) {
    double x = toFloat(ln)*earth_width / 180; 
    double y = merc(toFloat(lt), earth_width);
    return xy{round(x*100)/100, round(y*100)/100};
}

lonlat inverse_transform(double x, double y) {
    return lonlat{toInt(x*180 /earth_width), toInt(unMerc(y, earth_width))};
}

}}


