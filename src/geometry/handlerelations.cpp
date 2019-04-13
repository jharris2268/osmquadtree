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

#include "oqt/geometry/handlerelations.hpp"
#include "oqt/geometry/utils.hpp"
#include "oqt/geometry/makegeometries.hpp"
#include "oqt/utils/logger.hpp"
#include <sstream>
#include <map>
namespace oqt {
namespace geometry {

typedef std::vector<std::pair<size_t, std::string>> PendingVals;
typedef std::map<int64, PendingVals> PendingMap;



bool passes_filter(const RelationTagSpec& spec, const std::vector<Tag>& tags) {
    
    size_t passes_source_filter=0;
    bool has_source_key=false;
    for (const auto& tg : tags) {
        if (tg.key==spec.source_key) { has_source_key=true; }
        for (const auto& f: spec.source_filter) {
            if ((f.key==tg.key) && ((f.val=="*") || (f.val==tg.val))) {
                passes_source_filter++;
            }
        }
        
        if (has_source_key && (passes_source_filter>=spec.source_filter.size())) {
            return true;
        }
    }
    return false;
}
    


void handle_relation(const std::vector<RelationTagSpec>& spec, PendingMap& pending, std::shared_ptr<Relation> rel) {
    
    for (size_t i=0; i < spec.size(); i++) {
        
        if (passes_filter(spec.at(i), rel->Tags())) {
            auto val = get_tag(rel, spec.at(i).source_key);
            for (const auto& m: rel->Members()) {
                if (m.type==ElementType::Way) {
                    pending[m.ref].push_back(std::make_pair(i,val));
                }
            }
        }
        
    }
    
}

std::string find_min(size_t i, const PendingVals& vals) {
    
    int64 min_val=0;
    bool found=false;
    for (const auto& pv: vals) {
        if (pv.first==i) {
            try {
                int64 val = std::stoll(pv.second);
                if ((!found) || val < min_val) {
                    min_val=val;
                    found=true;
                }
            } catch (...) {}
        }
    }
    if (found) {
        return std::to_string(min_val);
    }
    return "";
}

std::string find_max(size_t i, const PendingVals& vals) {
    
    int64 max_val=0;
    bool found=false;
    for (const auto& pv: vals) {
        if (pv.first==i) {
            try {
                int64 val = std::stoll(pv.second);
                if ((!found) || val > max_val) {
                    max_val=val;
                    found=true;
                }
            } catch (...) {}
        }
    }
    if (found) {
        return std::to_string(max_val);
    }
    return "";
}

std::string find_list(size_t i, const PendingVals& vals) {
    
    std::set<std::string> ss;
    for (const auto& pv: vals) {
        if (pv.first==i) {
            ss.insert(pv.second);
        }
    }
    if (ss.empty()) {
        return "";
    }
    
    bool first=true;
    std::string result;
    for (const auto& v: ss) {
        if (!first) { 
            result += "; ";
        }
        result+=v;
        first=false;
    }
    
    return result;
}


bool finish_way(const std::vector<RelationTagSpec>& spec_vec, const PendingVals& pendingvals, std::shared_ptr<Element> ele) {
    
    
    
        
    bool found=false;
    for (size_t i=0; i < spec_vec.size(); i++) {
        
        const RelationTagSpec& spec = spec_vec.at(i);
        
        
        std::string result;
        if (spec.type == RelationTagSpec::Type::Min) {
            result = find_min(i,pendingvals);
        } else if (spec.type == RelationTagSpec::Type::Max) {
            result = find_max(i,pendingvals);
        } else if (spec.type == RelationTagSpec::Type::List) {
            result = find_list(i,pendingvals);
        }
        
        if (!result.empty()) {
            found=true;
            ele->AddTag(spec.target_key, result);
        }
        
        
        
    }
    
    return found;
}
    
    
class HandleRelationTags : public BlockHandler {
    
    public:
        HandleRelationTags(const std::vector<RelationTagSpec>& spec_) : spec(spec_) {}
        
        virtual ~HandleRelationTags() {}
        
        virtual primblock_vec process(PrimitiveBlockPtr bl) {
            
            
            if (!bl) { return {}; }
            
            
            for (auto ele: bl->Objects()) {
                if (ele->Type() == ElementType::Relation) {
                    auto rel = std::dynamic_pointer_cast<Relation>(ele);
                    
                    handle_relation(spec, pending, rel);
                }
            }
            
            for (auto ele: bl->Objects()) {
                if (ele->Type() == ElementType::WayWithNodes) {
                    auto it = pending.find(ele->Id());
                    if (it!=pending.end()) {
                        finish_way(spec, it->second, ele);
                        pending.erase(it);
                    }
                    
                }
                
            }
            return {bl};
        }
        
    private:
    
        std::vector<RelationTagSpec> spec;
        PendingMap pending;
};
            
std::shared_ptr<BlockHandler> make_handlerelations(const std::vector<RelationTagSpec>& spec) {
    return std::make_shared<HandleRelationTags>(spec);
}
/*

class HandleRelationTags : public BlockHandler {
    public:
        HandleRelationTags(bool boundary_polys_, bool add_admin_levels_, const std::set<std::string>& route_types_)
            : boundary_polys(boundary_polys_), add_admin_levels(add_admin_levels_), route_types(route_types_) {}


        virtual ~HandleRelationTags() {
            Logger::Message() << "~HandleRelationTags, route_refs.size()=" << route_refs.size() << ", admin_levels.size()=" << admin_levels.size();
        }


        virtual primblock_vec process(primblock_ptr bl) {

            std::vector<ElementPtr> tempobjs;
            for (auto o : bl->Objects()) {
                if (o->Type()==ElementType::Relation) {
                    auto r = std::dynamic_pointer_cast<Relation>(o);
                    auto type = get_tag(r, "type");
                    if ((type == "route") && (!route_types.empty())) {
                        auto rt = get_tag(r, "route");
                        if (route_types.count(rt)) {
                            auto rf = get_tag(r, "ref");
                            
                            if (!rf.empty()) {
                                for (const auto& m : r->Members()) {
                                    if (m.type==ElementType::Way) {
                                        route_refs[m.ref].insert(std::make_pair(rt,rf));
                                    }
                                }
                            }
                        }
                    } else if ((type=="boundary") && (get_tag(r, "boundary")=="administrative")) {
                        if (add_admin_levels) {
                            int lev=-1;
                            try {
                                lev = atoi(get_tag(r, "admin_level").c_str());
                            } catch (...) {
                                //pass
                            }
                            if (lev>=0) {
                                
                                for (const auto& m : r->Members()) {
                                    if (m.type==ElementType::Way) {
                                        auto it = admin_levels.find(m.ref);
                                        if (it==admin_levels.end()) {
                                            admin_levels[m.ref] = std::make_pair(lev,lev);
                                        } else {
                                            if (lev < it->second.first) { it->second.first=lev; }
                                            if (lev > it->second.second) { it->second.second=lev; }
                                        }
                                    }
                                }
                            }
                        }
                        if (boundary_polys) {
                            tempobjs.push_back(o);
                        }
                    } else if (type=="multipolygon") {
                        tempobjs.push_back(o);
                    }
                } else {
                    tempobjs.push_back(o);
                }

            }
            bl->Objects().swap(tempobjs);

            for (auto o : bl->Objects()) {
                if (o->Type()==ElementType::WayWithNodes) {

                    auto rf_it = route_refs.find(o->Id());
                    if (rf_it!=route_refs.end()) {
                        std::map<std::string,std::set<std::string>> tgs;
                        for (auto& rr : rf_it->second) {
                            
                            tgs[rr.first].insert(rr.second);
                            
                        }
                        for (auto& t : tgs) {
                            std::string val;
                            for (const auto& v: t.second) {
                                if (!val.empty()) { val += ", "; }
                                val += v;
                            }
                            
                            o->AddTag(t.first+"_routes", val);
                        }

                        route_refs.erase(rf_it);
                    }
                    auto al_it = admin_levels.find(o->Id());
                    if (al_it!=admin_levels.end()) {
                        
                        o->AddTag("min_admin_level", std::to_string(al_it->second.first));
                        o->AddTag("max_admin_level", std::to_string(al_it->second.second));
                        admin_levels.erase(al_it);
                    }
                }
            }
            return primblock_vec(1,bl);
        }


    private:
    
        
    
    
        bool boundary_polys;
        bool add_admin_levels;
        std::set<std::string> route_types;

        std::map<int64, std::multimap<std::string,std::string>> route_refs;
        std::map<int64, std::pair<int,int>> admin_levels;
};






std::shared_ptr<BlockHandler> make_handlerelations(bool boundary_polys, bool add_admin_levels, const std::set<std::string>& route_refs) {
    return std::make_shared<HandleRelationTags>(boundary_polys,add_admin_levels,route_refs);
}
*/
}}



