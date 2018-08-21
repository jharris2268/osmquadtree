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

#ifndef OBJSIDSET_HPP
#define OBJSIDSET_HPP

#include "oqt/readpbf/idset.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"

#include <set>
namespace oqt {

class objs_idset : public idset {
    public:
        objs_idset() {}
        virtual ~objs_idset() {}
        virtual bool contains(elementtype ty, int64 id) const {
            if (ty==Node) { return nodes.count(id); }
            if (ty==Way) { return ways.count(id); }
            if (ty==Relation) { return relations.count(id); }
            return false;
        }
        void add(elementtype t, int64 i) {
            if (t==Node) { nodes.insert(i); }
            else if (t==Way) { ways.insert(i); }
            else if (t==Relation) { relations.insert(i); }
        }
        void add_node(std::shared_ptr<node> nn) {
            nodes.insert(nn->Id());
        }

        void add_way(std::shared_ptr<way> ww) {
            ways.insert(ww->Id());
            for (auto& r : ww->Refs()) {
                nodes.insert(r);
            }

        }

        void add_relation(std::shared_ptr<relation> rr) {
            relations.insert(rr->Id());
            for (auto& m: rr->Members()) {
                if (m.type==Node) { nodes.insert(m.ref); }
                else if (m.type==Way) { ways.insert(m.ref); }
                else if (m.type==Relation) { relations.insert(m.ref);}                
            }
        }
        void add_all(std::shared_ptr<primitiveblock> pb) {
            for (auto& o : pb->objects) {
                if (o->Type()==0) {
                    auto n = std::dynamic_pointer_cast<node>(o);
                    if (!n) { throw std::domain_error("WTF"); }
                    add_node(n);
                } else if (o->Type()==1) {
                    auto w = std::dynamic_pointer_cast<way>(o);
                    if (!w) { throw std::domain_error("WTF"); }
                    add_way(w);
                } else if (o->Type()==2) {
                    auto r = std::dynamic_pointer_cast<relation>(o);
                    if (!r) { throw std::domain_error("WTF"); }
                    add_relation(r);
                }
            }
        }

        std::set<int64> nodes;
        std::set<int64> ways;
        std::set<int64> relations;
};
}


#endif
