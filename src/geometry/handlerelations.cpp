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

}}



