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


#include "oqt/pbfformat/objsidset.hpp"


namespace oqt {


bool objs_idset::contains(ElementType ty, int64 id) const {
    if (ty==ElementType::Node) { return nodes_.count(id); }
    if (ty==ElementType::Way) { return ways_.count(id); }
    if (ty==ElementType::Relation) { return relations_.count(id); }
    return false;
}
void objs_idset::add(ElementType t, int64 i) {
    if (t==ElementType::Node) { nodes_.insert(i); }
    else if (t==ElementType::Way) { ways_.insert(i); }
    else if (t==ElementType::Relation) { relations_.insert(i); }
}
void objs_idset::add_node(std::shared_ptr<Node> nn) {
    nodes_.insert(nn->Id());
}

void objs_idset::add_way(std::shared_ptr<Way> ww) {
    ways_.insert(ww->Id());
    for (auto& r : ww->Refs()) {
        nodes_.insert(r);
    }

}

void objs_idset::add_relation(std::shared_ptr<Relation> rr) {
    relations_.insert(rr->Id());
    for (auto& m: rr->Members()) {
        if (m.type==ElementType::Node) { nodes_.insert(m.ref); }
        else if (m.type==ElementType::Way) { ways_.insert(m.ref); }
        else if (m.type==ElementType::Relation) { relations_.insert(m.ref);}                
    }
}
void objs_idset::add_all(std::shared_ptr<primitiveblock> pb) {
    for (auto& o : pb->objects) {
        if (o->Type()==ElementType::Node) {
            auto n = std::dynamic_pointer_cast<Node>(o);
            if (!n) { throw std::domain_error("WTF"); }
            add_node(n);
        } else if (o->Type()==ElementType::Way) {
            auto w = std::dynamic_pointer_cast<Way>(o);
            if (!w) { throw std::domain_error("WTF"); }
            add_way(w);
        } else if (o->Type()==ElementType::Relation) {
            auto r = std::dynamic_pointer_cast<Relation>(o);
            if (!r) { throw std::domain_error("WTF"); }
            add_relation(r);
        }
    }
}

}
