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

#include "oqt/sorting/qttreegroups.hpp"
#include <numeric>
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/elements/minimalblock.hpp"
#include "oqt/pbfformat/readfile.hpp"
#include "oqt/elements/quadtree.hpp"
#include "oqt/pbfformat/writeblock.hpp"
#include <iomanip>
#include <algorithm>
#include <set>
namespace oqt {

std::pair<size_t,int64> clip_within_copy(std::shared_ptr<QtTree> tree, std::shared_ptr<QtTree> result, int64 min, int64 max, int64 absmin) {
    size_t cc=0;
    int64 sz=0;

    int64 qq=0;
    size_t i=0;
    while (i < tree->size() ) {
        const QtTree::Item& t = tree->at(i);

        if (t.qt < qq) {
            throw std::domain_error("out of order");
        }
        qq=t.qt;
        int64 t_total = t.total;
        const QtTree::Item& result_tile = result->at(result->find(qq));
        if (result_tile.qt == t.qt) {
            t_total -= result_tile.total;
        }
        
        

        if (t_total >= min) {
            bool alls = true;
            for (size_t ji=0; ji<4; ji++) {
                size_t j = t.children[ji];
                if (j>0) {
                    const QtTree::Item& ct = tree->at(j);
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
    
    

std::pair<size_t,int64> clip_within(std::shared_ptr<QtTree> tree, std::set<size_t>& outs, int64 min, int64 max, int64 absmin) {

    size_t cc=0;
    int64 sz=0;

    int64 qq=0;
    size_t i=0;
    while (i < tree->size() ) {
        const QtTree::Item& t = tree->at(i);


        
        if (t.qt < qq) {
            throw std::domain_error("out of order");
        }
        qq=t.qt;

        if (t.total >= min) {
            bool alls = true;
            for (size_t j : t.children)  {
                if (j>0) {
                    const QtTree::Item& ct = tree->at(j);
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


void tree_rollup(std::shared_ptr<QtTree> tree, int64 minsize) {
    int p=0;
    int64 v=0;
    for (int j=0; j < 18; j++) {
        int k = 17-j;
        for (size_t i=0; i<tree->size(); i=tree->next(i) ) {
            const QtTree::Item& t = tree->at(i);
            if ((t.qt&31) == k) {
                for (size_t ci=0; ci<4; ci++) {
                    if (t.children[ci]!=0) {
                        const QtTree::Item& c = tree->at(t.children[ci]);
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
    Logger::Message() << "rollup: minsize=" << minsize << "; removed " << p << " weight=" << v;
}


std::shared_ptr<QtTree> tree_round_copy(std::shared_ptr<QtTree> tree, int64 maxlevel) {
    auto result = make_tree_empty();
        
    size_t i =0;
    size_t max=tree->size();
    while (i<max) {
        const QtTree::Item& t = tree->at(i);
        
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

std::shared_ptr<QtTree> find_groups_copy(std::shared_ptr<QtTree> tree, int64 target, int64 minsize) {



    int64 total = tree->at(0).total;
    double total_fac = 100.0 / (double) total;

    auto result = make_tree_empty();
    
    int64 min=target-50;
    int64 max=target+50;
    while ((tree->at(0).total > result->at(0).total)) {

        while (true) {
            const QtTree::Item& t0 = tree->at(0);

            const QtTree::Item& r0 = result->at(0);
            

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
            Logger::Progress(100-rem*total_fac) << "min=" << min << ", max=" << max << " found=" << result->size() << " remaining " << rem
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
        QtTree::Item& t = result->at(i);
        if (t.qt <= qt) {
            Logger::Message() << "???" << idx << " " << i << " " << quadtree::string(qt) << "<=" << quadtree::string(t.qt) << "[ " << t.weight << ", " << t.total << "]";
        }
        if (t.weight != 0) {
            t.idx=idx;
            idx++;
        }
        i = result->next(i,0);
    }
    Logger::Message() << "found " << idx << " groups";
    return result;
    
}
}
