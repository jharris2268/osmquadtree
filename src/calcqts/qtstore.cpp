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

#include "oqt/utils/logger.hpp"

#include <algorithm>
//#include "oqt/utils.hpp"
#include <cmath>
#include <map>
namespace oqt {
class QtStoreMap : public QtStore {
    

    public:
        QtStoreMap() {}
        QtStoreMap(std::map<int64,int64>&& m_) : m(m_) {}

        virtual ~QtStoreMap() {}

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
            if (m.empty()) { return -1; }
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
    private:
        std::map<int64,int64> m;
};



class QtStoreVector : public QtStore  {
    
    public:
        QtStoreVector(int64 min_way, int64 max_way, int64 k) : pts(max_way-min_way,-1), min(min_way),count(0),key_(k) {};
        QtStoreVector(std::vector<int64>&& pts_, int64 min_, size_t count_, int64 k) : pts(std::move(pts_)),min(min_),count(count_),key_(k) {}

        ~QtStoreVector() {}

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
    
    private:
        std::vector<int64> pts;
        int64 min;
        size_t count;
        int64 key_;

};





class QtStore48bit : public QtStore  {
    
           
        
        
    public:
        QtStore48bit(int64 min_way, int64 max_way, int64 k) : min(min_way), max(max_way), count(0),key_(k) {
            
            size_t sz = (max_way-min_way) / 8;
            if (( (max_way-min_way) % 8)!=0) { sz+=1; }
            tiles.resize(sz);            
            if (key_==0) {
                Logger::Message() << "create QtStore48bit("
                    << min_way << ", " << max_way
                    << ", " << key_ << "): sz=" << sz << " {" << tiles.size() << "}"
                    << ", sizeof(Tile)=" << sizeof(Tile)
                    << " mem usage " << sizeof(Tile)*sz << " compared to "
                    << (max_way-min_way)*8;
            }
        };
        
        ~QtStore48bit() {}

        void expand(int64 ref, int64 qt) {
            if ((ref < min) || (ref >= max)) {
                Logger::Message() << "called expand(" << ref << ", " << qt << ") [range " << min << ", " << max << "]"; 
                throw std::range_error("??");
            }
            
            try {
                int64 curr = at(ref);


                if (curr==-1) {
                    if (qt<0) { return; }
                    set(ref, qt);
                    count++;
                } else {
                    if (qt<0) {
                        set(ref, -1);
                        count--;
                    } else {
                        set(ref, quadtree::common(curr, qt));
                    }
                }
            } catch (std::exception& ex) {
                size_t i=ref-min;
                
                Logger::Message() << "called expand(" << ref << ", " << qt << ") "
                        << "[range " << min << ", " << max << "]"
                        << " i=" << i << ", i/8=" << (i/8) << ", i%8=" << (i%8)
                        << " fails " << ex.what();
                throw ex;
            }

        }

        int64 at(int64 ref) {
            
            
            
            if (ref < min) {
                throw std::range_error("under range "+std::to_string(ref)+"<"+std::to_string(min));
            }
            if (ref >= max) {
                throw std::range_error("over range "+std::to_string(ref)+">="+std::to_string(max));
            }
            
            return get(ref);
        }

        bool contains(int64 ref) {

            return (at(ref)!=-1);
        }



        int64 first() {
            return next(min-1);
        }
        
        std::pair<int64,int64> ref_range() {
            return std::make_pair(min, max);
        }

        int64 next(int64 ref) {
            
            for (int64 test = ref+1; test < max; ++test) {
                if (get(test) != -1) {
                    return test;
                }
            }
            return -1;
        }

        size_t size() {
            return count;
        }
        
        int64 key() { return key_; }
    
    private:
        uint64_t orig_to_packed(int64 qt) {
            if (qt==-1) { return 0; }
            if (qt < -1) { throw std::domain_error("null qt"); }
            if ((qt&31) >= 20) { throw std::domain_error("too deep"); }
            
            return (((qt>>23) << 5) | (qt&31)) + 1;
        }
        
        int64 packed_to_orig(uint64_t v) {
            if (v==0) { return -1; }
            v -= 1;
            
            return ((v>>5) << 23) | (v&31);
        }
        
        
        int64 get(int64 ref) {
            size_t i  = (ref-min);
            
            return packed_to_orig(tiles.at(i/8).get(i%8));
        }
        
        void set(int64 ref, int64 q) {
            size_t i = ref-min;
            return tiles.at(i/8).set(i%8, orig_to_packed(q));
        }
    
    
        struct Tile {
            
            uint32_t upper[8];
            uint16_t lower[8];
            
            
            
            Tile() : upper{0,0,0,0,0,0,0,0}, lower{0,0,0,0,0,0,0,0} {}
            uint64_t get(size_t i) {
                if (i>=8) {
                    throw std::range_error("get("+std::to_string(i)+")");
                }
                uint64_t a = upper[i];
                a<<=16;
                a |= lower[i];
                return a;
                
                
            }
            void set(size_t i, uint64_t v) {
                if (i>=8) {
                    throw std::range_error("set("+std::to_string(i)+", "+std::to_string(v)+")");
                }
                
                upper[i] = (v>>16);
                lower[i] = (v&0xffff);
                
            }
        };
    
    
        std::vector<Tile> tiles;
        int64 min;
        int64 max;
        size_t count;
        int64 key_;

};

std::shared_ptr<QtStore> make_qtstore_vector_move(std::vector<int64>&& pts, int64 min, size_t count, int64 k) {
    return std::make_shared<QtStoreVector>(std::move(pts),min,count,k);
}

std::shared_ptr<QtStore> make_qtstore_map() {
    return std::make_shared<QtStoreMap>();
}
std::shared_ptr<QtStore> make_qtstore_vector(int64 min, int64 max, int64 key) {
    return std::make_shared<QtStoreVector>(min,max,key);
}
std::shared_ptr<QtStore> make_qtstore_48bit(int64 min, int64 max, int64 key) {
    return std::make_shared<QtStore48bit>(min,max,key);
}

}
