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

#include "oqt/calcqts/calculaterelations.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/operatingsystem.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
#include <map>
#include <algorithm>

namespace oqt {
    
    

class CalculateRelationsImpl : public CalculateRelations {
    typedef int32_t relation_id_type;
    typedef std::pair<relation_id_type,relation_id_type> id_pair;
    typedef std::vector<id_pair> id_pair_vec;

    typedef std::multimap<relation_id_type,relation_id_type> id_pair_map;

    
    public:
        CalculateRelationsImpl() : rel_qts(make_qtstore_map()),nodes_sorted(false), node_rshift(30) {
            node_mask = (1ull<<node_rshift) - 1;
            Logger::Message() << "calculate_relations_impl: node_rshift = " << node_rshift << ", node_mask = " << node_mask;
            
        }

        
        virtual void add_relations_data(minimal::BlockPtr data) {
            if (!data) { return; }
            if (data->relations.empty()) { return; }
            
            
            for (const auto& r: data->relations) {
                if (r.refs_data.empty()) {
                    empty_rels.push_back(r.id);
                }
                auto rfs = read_packed_delta(r.refs_data);
                auto tys = read_packed_int(r.tys_data);
                for (size_t i=0; i < tys.size(); i++) {
                    int64 rf=rfs[i];
                    if (tys[i]==0) {
                        
                        int64 s = rf>>node_rshift;
                    
                        id_pair_vec& objs = rel_nodes[s];
                        if (objs.size()==objs.capacity()) {
                            objs.reserve(objs.capacity()+(1<<(node_rshift-12)));
                        }
                        objs.push_back(std::make_pair(rf&node_mask, r.id));
                    } else {
                        id_pair_vec& objs = ((tys[i]==1) ? rel_ways : rel_rels);


                        if (objs.size()==objs.capacity()) {
                            objs.reserve(objs.capacity()+(1<<18));
                        }
                        objs.push_back(std::make_pair(rf, r.id));
                    }
                }
                
            }
        }



        
        virtual void add_nodes(minimal::BlockPtr mb) {
            auto cmpf = [](const id_pair& l, const id_pair& r) { return l.first < r.first; };
            if (!nodes_sorted) {
                Logger::Message() << "sorting nodes [RSS = " << std::fixed << std::setprecision(1) << getmemval(getpid())/1024.0 << "]";
                if (rel_nodes.empty()) {
                    Logger::Message() << "no nodes...";
                } else {
                    int64 mx = rel_nodes.rbegin()->first;
                    
                    node_check.resize((mx+1) << node_rshift);
                    for (auto& rn : rel_nodes) {
                        std::sort(rn.second.begin(), rn.second.end(), cmpf);
                        int64 k = rn.first << node_rshift;
                        for (const auto& n: rn.second) {
                            node_check[k+n.first]=true;
                        }
                        
                    }
                    
                    Logger::Message() << "node_check.size() = " << node_check.size() << " [RSS = " << std::fixed << std::setprecision(1) << getmemval(getpid())/1024.0 << "]";
                }
                nodes_sorted=true;
            }
            if (!mb) { return; }
            if (mb->nodes.size()==0) { return; }
            
            
            for (const auto& n : mb->nodes) {
                int64 rf = n.id;
                
                if ((((size_t) rf)>=node_check.size()) || (!node_check[rf])) {
                    continue;
                }
                
                int64 qt=n.quadtree;
                int64 rf0 = rf>>node_rshift;
                relation_id_type rf1 = rf&node_mask;
                if (!rel_nodes.count(rf0)) {
                    
                    continue;
                }
                const id_pair_vec& tile = rel_nodes[rf0];
                
                id_pair_vec::const_iterator it,itEnd;
                std::tie(it,itEnd) = std::equal_range(tile.begin(), tile.end(), std::make_pair(rf1,0), cmpf);
                
                if ((it<tile.end()) && (it<itEnd)) {
                    
                        
                    
                    for ( ; it<itEnd; ++it) {
                        if (it->first != rf1) {
                            Logger::Message() << "??? rel_node " << (int64) it->first + (rf>>node_rshift) << " " << it->second << " matched by " << rf;
                            continue;
                        }
                        rel_qts->expand(it->second, qt);
                        
                        
                        
                    }
                }
            }
        }
        

        virtual void add_ways(std::shared_ptr<QtStore> qts) {
            for (const auto& wq : rel_ways) {
                int64 q=qts->at(wq.first);
                if (q>=0) {
                    rel_qts->expand(wq.second, q);
                }
               
                
                
            }
            id_pair_vec e;
            rel_ways.swap(e);
        }
        
        void finish_alt(std::function<void(int64,int64)> func) {
            for (auto r : empty_rels) {
                rel_qts->expand(r,0);
            }

            for (size_t i=0; i < 5; i++) {
                for (auto rr : rel_rels) {
                    if (rel_qts->contains(rr.first)) {
                        rel_qts->expand(rr.second, rel_qts->at(rr.first));
                    }
                }
            }
            for (auto rr : rel_rels) {
                if (!rel_qts->contains(rr.second)) {
                    Logger::Message() << "no child rels ??" << rr.first << " " << rr.second;
                    rel_qts->expand(rr.second, 0);
                }
            }
            
            int64 rf = rel_qts->first();
            for ( ; rf > 0; rf=rel_qts->next(rf)) {
                func(rf, rel_qts->at(rf));
            }
        }
        
        
        

        virtual std::string str() {
            size_t rnn=0,rnc=0;
            for (auto& tl : rel_nodes) {
                
                rnn += tl.second.size();
                rnc += tl.second.capacity();
          
            }
            

            std::stringstream ss;
            ss  << "have: " << rel_qts->size() << " rel qts\n"
                << "      " << rnn << " rel_nodes [cap=" << rnc << "]\n"
                << "      " << rel_ways.size() << " rel_ways [cap=" << rel_ways.capacity() << "]\n"
                << "      " << rel_rels.size() << " rel_rels \n"//[cap=" << rel_rels.capacity() << "]\n"
                << "      " << empty_rels.size() << " empty_rels";
            return ss.str();
        }
    private:
        std::shared_ptr<QtStore> rel_qts;

        std::map<int64,id_pair_vec> rel_nodes;
        bool nodes_sorted;
        
        id_pair_vec rel_ways;
        id_pair_vec rel_rels;
        std::vector<relation_id_type> empty_rels;
        bool objs_sorted;
        
        size_t node_rshift;
        size_t node_mask;
        
        std::vector<bool> node_check;

};

std::shared_ptr<CalculateRelations> make_calculate_relations() {
    return std::make_shared<CalculateRelationsImpl>();
}

 
    
}

