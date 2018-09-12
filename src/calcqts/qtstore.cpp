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

#include "oqt/calcqts/qtstore.hpp"
#include "oqt/calcqts/qtstoresplit.hpp"
#include "oqt/elements/quadtree.hpp"
#include <algorithm>
//#include "oqt/utils.hpp"
#include <cmath>
#include <map>
namespace oqt {
class qtstore_map : public qtstore {
    std::map<int64,int64> m;

    public:
        qtstore_map() {}
        qtstore_map(std::map<int64,int64>&& m_) : m(m_) {}

        void expand(int64 ref, int64 qt) {

            if (m.count(ref)==0) {
                if (qt<0) { return; }
                m[ref]=qt;
            } else {
                if (qt<0) {
                    m.erase(ref);
                } else {
                    m[ref]=quadtree::common(m[ref],qt);
                }
            }
        }

        int64 at(int64 ref) {
            if (!contains(ref)) {
                return -1;
            }
            return m[ref];
        }

        bool contains(int64 ref) {
            return m.count(ref)>0;
        }

        int64 first() {
            return m.begin()->first;
        }
        
        std::pair<int64,int64> ref_range() { return std::make_pair(-1,-1); }
        

        int64 next(int64 ref) {
            auto it = m.lower_bound(ref);
            if (it==m.end()) { return -1; }
            if (it->first > ref) {
                return it->first;
            }
            ++it;
            if (it==m.end()) { return -1; }
            return it->first;
        }

        size_t size() { return m.size(); }
        
        int64 key() { return -1; }
};



class qtstore_arr : public qtstore  {
    std::vector<int64> pts;
    int64 min;
    size_t count;
    int64 key_;
    public:
        qtstore_arr(int64 min_way, int64 max_way, int64 k) : pts(max_way-min_way,-1), min(min_way),count(0),key_(k) {};
        qtstore_arr(std::vector<int64>&& pts_, int64 min_, size_t count_, int64 k) : pts(std::move(pts_)),min(min_),count(count_),key_(k) {}

        void expand(int64 ref, int64 qt) {
            if (ref < min) {
                throw std::domain_error("too small");
            }
            size_t rm = ref-min;

            if (rm >= pts.size()) {
                throw std::domain_error("too big");
            }

            if (pts[rm]==-1) {
                if (qt<0) { return; }
                pts[rm]=qt;
                count++;
            } else {
                if (qt<0) {
                    pts[rm]=-1;
                    count--;
                } else {
                    pts[rm]=quadtree::common(pts[rm], qt);
                }
            }

        }

        int64 at(int64 ref) {
            if (!contains(ref)) {
                return -1;
            }
            size_t rm=ref-min;
            return pts[rm];
        }

        bool contains(int64 ref) {

            int64 rm=ref-min;

            return ((rm >= 0) && (rm < (int64)pts.size()) && (pts.at(rm)!=-1));
        }



        int64 first() {
            return next(min-1);
        }
        
        std::pair<int64,int64> ref_range() {
            return std::make_pair(min, min+pts.size());
        }

        int64 next(int64 ref) {
            int64 rm=ref-min;

            auto it = std::find_if(pts.begin()+rm+1, pts.end(), [](const int64& v){return v!=-1;} );
            if (it==pts.end()) {
                return -1;
            }

            return (int64) (it-pts.begin()) + min;
        }

        size_t size() {
            return count;
        }
        
        int64 key() { return key_; }


};


std::shared_ptr<qtstore> make_qtstore_arr_alt(std::vector<int64>&& pts, int64 min, size_t count, int64 k) {
    return std::make_shared<qtstore_arr>(std::move(pts),min,count,k);
}

std::shared_ptr<qtstore> make_qtstore_map() {
    return std::make_shared<qtstore_map>();
}
std::shared_ptr<qtstore> make_qtstore_arr(int64 min, int64 max, int64 key) {
    return std::make_shared<qtstore_arr>(min,max,key);
}


}
