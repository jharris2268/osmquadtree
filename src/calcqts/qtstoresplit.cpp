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
    


class QtStoreSplitImpl : public QtStoreSplit {
    public:
        QtStoreSplitImpl(int64 splitat, bool use_arr) : splitat_(splitat), use_arr_(use_arr) {};
        QtStoreSplitImpl(int64 splitat, bool use_arr, std::map<int64,std::shared_ptr<QtStore>> tiles_) : splitat_(splitat), use_arr_(use_arr), tiles(tiles_) {};

        virtual ~QtStoreSplitImpl() {}

        virtual void expand(int64 ref, int64 qt) {
            get_tile(ref,true)->expand(ref,qt);
        }

        virtual int64 at(int64 ref) {
            std::shared_ptr<QtStore > tile = get_tile(ref,false);
            if (!tile) { return -1; }
            return tile->at(ref);
        };

        virtual bool contains(int64 ref) {
            std::shared_ptr<QtStore> tile = get_tile(ref,false);
            if (!tile) { return false; }
            return tile->contains(ref);
        };

        virtual size_t size() {
            size_t total=0;
            for(const auto& t : tiles) {
                if (t.second) {
                    total += t.second->size();
                }
            }
            return total;
        }

        virtual bool use_arr() { return use_arr_; }
        virtual int64 split_at() { return splitat_; }

        virtual size_t num_tiles() {
            return tiles.size();
        }

        virtual size_t last_tile() {
            if (tiles.size()==0) { return 0; }
            return tiles.rbegin()->first + 1;
        }

        virtual std::shared_ptr<QtStore> tile(size_t i) {
            return get_tile_key(i,false);
        }

        virtual int64 next(int64 ref) {
            if (ref<0) { return next(0); }
            
            if (ref >= ((int64) last_tile()*splitat_)) {
                return -1;
            }
            
            std::shared_ptr<QtStore> tile = get_tile(ref,false);
            if (tile) {
                int64 res = tile->next(ref);
                if (res!=-1) {
                    return res;
                }
            }
            
            size_t ni = ref/splitat_;
            ni++;
            int64 nref = ni*splitat_;
            if (contains(nref)) {
                return nref;
            }
            return next(nref);
            
        };
        virtual int64 first() { return next(0); };
        


        void add_tile(size_t i, std::shared_ptr<QtStore> qts) {
            if (tiles.count(i)) {
                throw std::domain_error("already have tile");
            }
            tiles[i]=qts;
        }
        
        void merge(std::shared_ptr<QtStoreSplit> other_in) {
            auto other = std::dynamic_pointer_cast<QtStoreSplitImpl>(other_in);
            if (!other) {
                throw std::domain_error("not a QtStoreSplitImpl??");
            }

            for (auto& bl : other->tiles) {
                if (tiles.count(bl.first)) {
                    throw std::domain_error("already have tile");
                }
                tiles[bl.first]=bl.second;
            }
           
        }
        int64 key() { return -1; }
        std::pair<int64,int64> ref_range() { return std::make_pair(-1,-1); }

    private:
        int64 splitat_;
        bool use_arr_;

        std::shared_ptr<QtStore> get_tile(int64 ref, bool create) {
            return get_tile_key(ref/splitat_, create);
        }
        std::shared_ptr<QtStore> get_tile_key(int64 k, bool create) {
            if (tiles.count(k)>0) {
                return tiles[k];
            }
            if (!create) { return std::shared_ptr<QtStore>(); }
            if (use_arr_) {
                //tiles[k] = make_qtstore_vector(k*splitat_, (k+1)*splitat_,k);
                tiles[k] = make_qtstore_48bit(k*splitat_, (k+1)*splitat_,k);
            } else {
                tiles[k] = make_qtstore_map();
            }
            return tiles[k];
        }

        std::map<int64, std::shared_ptr<QtStore>> tiles;
};


std::shared_ptr<QtStoreSplit> make_qtstore_split(int64 splitat, bool usearr) {
    return std::make_shared<QtStoreSplitImpl>(splitat,usearr);
}
std::shared_ptr<QtStoreSplit> combine_qtstores(std::vector<std::shared_ptr<QtStoreSplit>> src) {
    if (src.empty()) { throw std::domain_error("??"); }
    if (!src.front()) { throw std::domain_error("??"); }
    int64 splitat= src.front()->split_at();
    bool use_arr = src.front()->use_arr();

    std::map<int64,std::shared_ptr<QtStore>> qts;

    for (const auto& store : src) {
        if (!store) { throw std::domain_error("??"); }
        if ((store->split_at()!=splitat) || (store->use_arr() != use_arr)) {
            throw std::domain_error("??");
        }
        for (size_t i=0; i < store->num_tiles(); i++) {
            auto t = store->tile(i);
            if (t) {
                if (qts.count(i)>0) {
                    throw std::domain_error("already added tile");
                }
                qts[i] = t;
            }
        }
    }
    return std::make_shared<QtStoreSplitImpl>(splitat, use_arr,qts);
}

}
