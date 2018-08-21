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

#include "oqt/quadtreegroups.hpp"
#include <numeric>

#include <iomanip>
#include <algorithm>
#include <set>
namespace oqt {
std::string item_string(size_t i, const qttree_item& t) {
    std::stringstream ss;
    ss  << std::right << std::setw(12) << i
        << std::setw(22) << t.qt
        << " " << std::setw(18) << std::left << quadtree::string(t.qt)
        << std::fixed << std::setprecision(0)
        << std::right << std::setw(12) << t.weight
        << " " << std::right << std::setw(12) <<  t.total;
    return ss.str();
}

struct qttree_tile {
    std::vector<qttree_item> vals;
    size_t len;

    qttree_tile() : vals(1<<16),len(0) {}
    qttree_item& at(size_t i) {
        if (i>=len) {
            throw std::domain_error("out of range");
        }
        return std::ref(vals[i]);
    }

    size_t push_back(int64 qt, size_t parent) {
        if (len==vals.size()) {
            throw std::domain_error("out of range");
        }
        vals[len].qt=qt;
        vals[len].parent=parent;

        return len++;
    }

};


class qttree_impl : public qttree {
    std::deque<qttree_tile> tiles;
    uint32_t idx;

    public:

        qttree_impl() {
            idx=1;
            push_back(0,0);
        }

        qttree_item& at(size_t i) {
            size_t t = i>>16;
            if (t>=tiles.size()) {
                throw std::domain_error("out of range");
            }
            return std::ref(tiles[t].at(i&65535));
        }

        size_t size() {
            if (tiles.empty()) {
                return 0;
            }
            return (tiles.size()-1)*65536 + tiles.back().len;
        }

        size_t push_back(int64 qt, size_t parent) {
            if (tiles.empty() || (tiles.back().len==65536)) {
                tiles.push_back(qttree_tile());
            }
            size_t r=size();

            tiles.back().push_back(qt,parent);

            return r;
        }

        size_t find(int64 qt) {
            return find_int(qt,0);
        }

        size_t find_int(int64 qt, size_t curr) {
            qttree_item& t = at(curr);
            if (t.qt==qt) {
                return curr;
            }

            size_t c = (qt>>(61-2*(t.qt&31)))&3;
            if (t.children[c]==0) {
                return curr;
            }
            return find_int(qt, t.children[c]);
        }

        size_t add(int64 qt, int64 val) {
            return add_int(qt,val, 0);
        }

        size_t add_int(int64 qt, int64 val, size_t curr) {
            qttree_item& t = at(curr);
            t.total+=val;
            if (t.qt==qt) {
                if (t.idx==0) {
                    t.idx=idx++;
                }
                t.weight+=val;
                return curr;
            }

            size_t c = (qt>>(61-2*(t.qt&31)))&3;
            if (t.children[c]==0) {
                int64 qtr=quadtree::round(qt,(t.qt&31)+1);
                t.children[c]=push_back(qtr,curr);
            }
            return add_int(qt, val, t.children[c]);
        }

        size_t next(size_t curr, size_t c=0) {
            const qttree_item& t=at(curr);
            for ( ; c<4; c++) {
                if (t.children[c]!=0) {
                    return t.children[c];
                }
            }

            if (t.parent==curr) { return size(); }
            size_t pc = (t.qt>>(63-2*(t.qt&31)))&3;
            return next(t.parent,pc+1);
        }

        void rollup(size_t curr) {
            qttree_item& t=at(curr);
            t.weight=t.total;
            for (size_t i=0; i < 4; i++) {
                t.children[i]=0;
            }
        }
        
        void rollup_child(size_t curr, size_t ci) {
            qttree_item& t=at(curr);
            if (t.children[ci]==0) {
                logger_message() << "rollup empty child??" << curr << " " << t.qt << " " << t.weight << " " << t.total << " " << ci;;
                return;
            }
            qttree_item& ct=at(t.children[ci]);
            t.weight += ct.total;
            t.children[ci] = 0;
        }
            
            
            

        size_t clip(size_t curr) {
            const qttree_item t=at(curr);
            size_t p=curr;
            int64 tt=t.total;

            size_t tp=t.parent;

            while (tp != p) {
                qttree_item& s = at(tp);
                p=tp;
                tp=s.parent;
                s.total -= tt;
            }

            qttree_item& s2 = at(t.parent);
            size_t pc = (t.qt>>(63-2*(t.qt&31)))&3;
            s2.children[pc]=0;
            return next(t.parent,pc+1);
        }

        qttree_item find_tile(int64 qt) {
            qttree_item t=at(find(qt));
            while ((t.weight==0) && (t.qt!=0)) {
                t=at(t.parent);
            }
            return t;
        }

};



void collect_block(std::shared_ptr<count_map> res, std::shared_ptr<minimalblock> block) {

    for (const auto&  nn : block->nodes) {
        (*res)[nn.quadtree]+=1;
    }

    for (const auto&  ww : block->ways) {
        (*res)[ww.quadtree]+=1;
    }

    for (const auto&  rr : block->relations) {
        (*res)[rr.quadtree]+=1;
    }
    for (const auto& gg: block->geometries) {
        (*res)[gg.quadtree]+=1;
    }

}

std::shared_ptr<qttree> make_tree_empty() {
    return std::make_shared<qttree_impl>();
}

class AddCountMapTree : public CallFinish<count_map,void> {
    public:
        AddCountMapTree(std::shared_ptr<qttree> tree_, size_t maxlevel_) : 
            tree(tree_), maxlevel(maxlevel_), tb(0) {}
        
        virtual void call(std::shared_ptr<count_map> fb) {
            tb+=std::accumulate(fb->begin(),fb->end(),0,
            [](int64 l, const std::pair<int64,int64>& r){return l+r.second;});
            logger_progress(50) << "add " << fb->size() << " qts ["
                    << std::setw(12) << tree->size() << " "
                    << std::fixed << std::setw(15) << std::setprecision(0) << tb
                    << "]";
            for (auto it : *fb) {
                if (it.first>=0) {
                    tree->add(quadtree::round(it.first,maxlevel),it.second);
                }
                
            }
        }
        virtual void finish() {
            logger_progress(100) << "finished totals: [" << tree->size() << " " << tb << "]        ";
        }
    private:
        std::shared_ptr<qttree> tree;
        size_t maxlevel;
        int64 tb;
};


std::shared_ptr<CallFinish<count_map,void>> make_addcountmaptree(std::shared_ptr<qttree> tree, size_t maxlevel) {
    return std::make_shared<AddCountMapTree>(tree, maxlevel);
}
    


std::pair<size_t,int64> clip_within_copy(std::shared_ptr<qttree> tree, std::shared_ptr<qttree> result, int64 min, int64 max, int64 absmin) {
    size_t cc=0;
    int64 sz=0;

    int64 qq=0;
    size_t i=0;
    while (i < tree->size() ) {
        const qttree_item& t = tree->at(i);

        if (t.qt < qq) {
            throw std::domain_error("out of order");
        }
        qq=t.qt;
        int64 t_total = t.total;
        const qttree_item& result_tile = result->at(result->find(qq));
        if (result_tile.qt == t.qt) {
            t_total -= result_tile.total;
        }
        
        

        if (t_total >= min) {
            bool alls = true;
            for (size_t ji=0; ji<4; ji++) {
                size_t j = t.children[ji];
                if (j>0) {
                    const qttree_item& ct = tree->at(j);
                    int64 ct_total = ct.total;
                    if ((result_tile.qt == t.qt)&&(result_tile.children[ji]>0)) {
                        
                        ct_total -= result->at(result_tile.children[ji]).total;
                    }                    
                    
                    if (ct_total > absmin) {
                        alls=false;
                        break;
                    }
                }
            }
            

            if ((t.weight!=0) && ((t_total==t.weight) || (t_total <= max) || alls)) {
                
                cc++;
                sz+=t_total;
                
                result->add(qq, t_total);

                i = tree->next(i,4);
            } else {
                i = tree->next(i,0);
            }
        } else {
            i = tree->next(i,4);
        }
        
    }

    return std::make_pair(cc,sz);
}
    
    

std::pair<size_t,int64> clip_within(std::shared_ptr<qttree> tree, std::set<size_t>& outs, int64 min, int64 max, int64 absmin) {

    size_t cc=0;
    int64 sz=0;

    int64 qq=0;
    size_t i=0;
    while (i < tree->size() ) {
        const qttree_item& t = tree->at(i);


        
        if (t.qt < qq) {
            throw std::domain_error("out of order");
        }
        qq=t.qt;

        if (t.total >= min) {
            bool alls = true;
            for (size_t j : t.children)  {
                if (j>0) {
                    const qttree_item& ct = tree->at(j);
                    if (ct.total > absmin) {
                        alls=false;
                        break;
                    }
                }
            }
            
            if ((t.weight!=0) && ((t.total==t.weight) || (t.total <= max) || alls)) {
                
                cc++;
                sz+=t.total;
                outs.insert(i);

                i = tree->clip(i);
            } else {
                i = tree->next(i,0);
            }
        } else {
            
            i = tree->next(i,4);
        }
        
    }

    return std::make_pair(cc,sz);
}


void tree_rollup(std::shared_ptr<qttree> tree, int64 minsize) {
    int p=0;
    int64 v=0;
    for (int j=0; j < 18; j++) {
        int k = 17-j;
        for (size_t i=0; i<tree->size(); i=tree->next(i) ) {
            const qttree_item& t = tree->at(i);
            if ((t.qt&31) == k) {
                for (size_t ci=0; ci<4; ci++) {
                    if (t.children[ci]!=0) {
                        const qttree_item& c = tree->at(t.children[ci]);
                        if (c.total<minsize) {
                            p++;
                            v+=c.total;
                            tree->rollup_child(i,ci);
                        }
                    }
                }
            }
            
        }
    }
    logger_message() << "rollup: minsize=" << minsize << "; removed " << p << " weight=" << v;
}


std::shared_ptr<qttree> tree_round_copy(std::shared_ptr<qttree> tree, int64 maxlevel) {
    auto result = make_tree_empty();
        
    size_t i =0;
    size_t max=tree->size();
    while (i<max) {
        const qttree_item& t = tree->at(i);
        
        if (t.total==0) {
            i = tree->next(i,4);
        } else if ((t.qt&31)==maxlevel) {
            result->add(t.qt, t.total);
            i = tree->next(i,4);
        } else {
            if (t.weight>0) {
                result->add(t.qt, t.weight);
            }
            i = tree->next(i);
        }
    }
    
    return result;
        
}

std::shared_ptr<qttree> find_groups_copy(std::shared_ptr<qttree> tree, int64 target, int64 minsize) {



    int64 total = tree->at(0).total;
    double total_fac = 100.0 / (double) total;

    auto result = make_tree_empty();
    
    int64 min=target-50;
    int64 max=target+50;
    while ((tree->at(0).total > result->at(0).total)) {

        while (true) {
            const qttree_item& t0 = tree->at(0);

            const qttree_item& r0 = result->at(0);
            

            if (t0.total == r0.total) {
                break;
            }
            if (((t0.total-r0.total) < max) || ( (t0.total-r0.total)==t0.weight)) {
                result->add(0, (t0.total-r0.total));
                break;
            }

            size_t cc; int64 sz;
            std::tie(cc,sz) = clip_within_copy(tree,result,min,max,minsize);
            if (cc==0) {
                break;
            }
        }
        if (result->at(0).weight==0) {
            int64 rem = tree->at(0).total-result->at(0).total;
            logger_progress(100-rem*total_fac) << "min=" << min << ", max=" << max << " found=" << result->size() << " remaining " << rem
                      << "[" << std::setw(5) << std::fixed << std::setprecision(1) << rem*total_fac << "%]]";
        }

        min -= 50;
        max += 50;
        if (min<minsize) { min=minsize; }
        if (max > 50*target) {
            break;
        }
    }
    
    size_t idx=1;
    size_t i=0;
    int64 qt=-1;
    while (i < result->size() ) {
        qttree_item& t = result->at(i);
        if (t.qt <= qt) {
            logger_message() << "???" << idx << " " << i << " " << quadtree::string(qt) << "<=" << quadtree::string(t.qt) << "[ " << t.weight << ", " << t.total << "]";
        }
        if (t.weight != 0) {
            t.idx=idx;
            idx++;
        }
        i = result->next(i,0);
    }
    logger_message() << "found " << idx << " groups";
    return result;
    
}
}
