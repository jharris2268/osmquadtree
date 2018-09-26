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

#include "oqt/geometry/addparenttags.hpp"
#include "oqt/utils/logger.hpp"
namespace oqt {
namespace geometry {

typedef std::map<std::string,std::string> temp_tags_map;
typedef std::map<int64,std::pair<ElementPtr,temp_tags_map>> pending_objs_map;
typedef std::map<int64,pending_objs_map> pending_blocks_map;


std::shared_ptr<primitiveblock> finish_tags(int64 qt, pending_objs_map& pending) {
    auto pb = std::make_shared<primitiveblock>(0,pending.size());
    pb->quadtree=qt;
    for (auto& po : pending) {
        if (!po.second.second.empty()) {

            for (auto& tt: po.second.second) {
                if (!tt.second.empty()) {
                    po.second.first->AddTag(tt.first,tt.second);
                }
            }
        }
        pb->objects.push_back(po.second.first);
    }
    pending.clear();
    return pb;

}

void check_way_tags(const parenttag_spec_map& spec, temp_tags_map& node_tags, const tagvector& way_tags) {

    std::map<std::string,std::string> wtgs;
    for (auto& t: way_tags) { wtgs[t.key]=t.val; }

    for (const auto& spp: spec) {
        const auto& sp = spp.second;
        auto nt_it = node_tags.find(sp.out_tag);
        if (nt_it!=node_tags.end()) {

            if (wtgs.count(sp.way_tag)>0) {
                std::string way_val = wtgs[sp.way_tag];
                auto w_prio_it = sp.priority.find(way_val);
                if (w_prio_it!=sp.priority.end()) {

                    if (nt_it->second=="") {
                        nt_it->second=way_val;
                    } else if (w_prio_it->second > sp.priority.find(nt_it->second)->second) {
                        nt_it->second=way_val;
                    }
                }
            }
        }
    }
}


temp_tags_map* find_node_tags(pending_blocks_map& cache, int64 i) {
    for (auto& cc : cache) {
        auto it=cc.second.find(i);
        if (it!=cc.second.end()) {
            return &(it->second.second);
        }
    }
    return nullptr;
}


class AddParentTags : public BlockHandler {
    public:
        AddParentTags(const parenttag_spec_map& spec_) : spec(spec_), ncheck((1ull)<<32,false) {}
        
        virtual ~AddParentTags() {}
        
        virtual primblock_vec finish() {
            primblock_vec out;
            for (auto& cc: cache) {
                auto fb=finish_tags(cc.first,cc.second);
                if (!fb->objects.empty()) {
                    out.push_back(fb);
                }
            }
            
            
            std::vector<bool> em;
            ncheck.swap(em);
            return out;
        }
        
        virtual primblock_vec process(std::shared_ptr<primitiveblock> bl) {
            
            primblock_vec out;
            int64 qt = bl->quadtree;
            for (auto it = cache.begin(); it!=cache.end(); ) {
                if (quadtree::common(it->first,qt)!=it->first) {
                    auto finbl = finish_tags(it->first, it->second);
                    if (!finbl->objects.empty()) {
                        out.push_back(finbl);
                    }
                    it=cache.erase(it);
                } else {
                    ++it;
                }

            }


            pending_objs_map nb;

            std::vector<ElementPtr> tempobjs;
            for (auto o : bl->objects) {
                if (o->Type()==ElementType::Node) {
                    bool hast=false;
                    for (const auto& tt: o->Tags()) {
                        if (spec.count(tt.key)>0) {
                            auto it=spec.find(tt.key);


                            nb[o->Id()].second[it->second.out_tag]="";
                            hast=true;

                        }
                    }
                    if (hast) {
                        size_t oi=o->Id();
                        if (oi>ncheck.size()) {
                            ncheck.resize(oi+1000000);
                        }
                        ncheck[oi]=true;
                        
                        nb[o->Id()].first=o;
                    } else {
                        tempobjs.push_back(o);
                    }
                } else {
                    tempobjs.push_back(o);
                }
            }
            if (!nb.empty()) {
                cache[bl->quadtree]=nb;
            }

            for (auto o : tempobjs) {

                if (o->Type()==ElementType::WayWithNodes) {
                    auto w = std::dynamic_pointer_cast<way_withnodes>(o);
                    
                    for (auto i : w->Refs()) {
                        size_t oi=i;
                        if ((oi<ncheck.size()) && (ncheck[oi])) {
                            auto it=find_node_tags(cache,i);
                            if (!it) {
                                logger_message() << "can't find " << i << " from way " << w->Id();
                            } else {
                                check_way_tags(spec, *it, w->Tags());
                            }
                        }
                    }
                }
            }

            bl->objects.swap(tempobjs);
            if (!bl->objects.empty()) {
                out.push_back(bl);
            }
            return out;
            
        }
        
               
                
    private:
        parenttag_spec_map spec;
        std::vector<bool> ncheck;
        pending_blocks_map cache;
        
        

};

std::shared_ptr<BlockHandler> make_addparenttags(const parenttag_spec_map& spec) {
    return std::make_shared<AddParentTags>(spec);
}


}

}








