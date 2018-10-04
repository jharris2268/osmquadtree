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


#include "oqt/elements/relation.hpp"


namespace oqt {



Relation::Relation(changetype c, int64 i, int64 q, ElementInfo inf, std::vector<Tag> tags, std::vector<Member> mems)
    : Element(ElementType::Relation,c,i,q,inf,tags), mems_(mems) {}
        
const std::vector<Member>&  Relation::Members() const { return mems_; }

ElementPtr Relation::copy() { return std::make_shared<Relation>(ChangeType(),Id(),Quadtree(),Info(),Tags(),Members()); }

bool Relation::filter_members(IdSetPtr ids) {
    if (mems_.empty()) { return true; }
    std::vector<Member> mems_new;
    mems_new.reserve(mems_.size());
    
    for (auto& m: mems_) {
        if (ids->contains(m.type, m.ref)) {
            mems_new.push_back(m);
        }
    }
    if (mems_.size()==mems_new.size()) {
        return false;
    }
    //mems_.swap(mems_new);
    mems_=mems_new;
    return true;
}
bool fix_members(Relation& rel) {
    if (rel.mems_.empty()) { return false; }
    bool r=false;
    for (auto& m: rel.mems_) {
        if ((!m.role.empty()) && (m.role.find(127)!=std::string::npos)) {
            auto s=fix_str(m.role);
            if (s!=m.role) {
                m.role=s;
                r=true;
            }
        }
    }
    return r;
}
}
