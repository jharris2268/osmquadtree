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

#include "oqt/elements/combineblocks.hpp"
#include <map>
namespace oqt {
bool check_changetype(changetype ct) {
    switch (ct) {
        case changetype::Normal: return true;
        case changetype::Delete: return false;
        case changetype::Remove: return false;
        case changetype::Unchanged: return true;
        case changetype::Modify: return true;
        case changetype::Create: return true;
        
    }
    return true;
}

int obj_cmp(ElementPtr l, ElementPtr r) {
    if (l->Type()==r->Type()) {
        if (l->Id()==r->Id()) {
            return 0;
        }
        /*    if (l->Info().version==r->Info().version) {
                if (l->ChangeType()==r->ChangeType()) { return 0; }
                return (l->ChangeType() < r->ChangeType()) ? -1 : 1;
            }
            return (l->Info().version < r->Info().version) ? -1 : 1;
        }*/
        return (l->Id() < r->Id()) ? -1 : 1;
    }
    return (l->Type() < r->Type()) ? -1 : 1;
}


PrimitiveBlockPtr combine_primitiveblock_two(
    size_t idx,
    PrimitiveBlockPtr left,
    PrimitiveBlockPtr right,
    bool apply_change) {

    if (!left || !right) { throw std::domain_error("??"); }
    if (left->Quadtree()!=right->Quadtree()) { throw std::domain_error("?? different quadtree"); }

    size_t left_size = left->size();
    size_t right_size = right->size();
    
    auto res = std::make_shared<PrimitiveBlock>(idx,left_size+right_size);
    res->SetQuadtree(left->Quadtree());
    res->SetStartDate(left->StartDate());
    res->SetEndDate(right->EndDate());

    size_t left_idx = 0;
    size_t right_idx= 0;

    for ( ; left_idx<left_size || right_idx < right_size; ) {
        ElementPtr left_object, right_object;
        if (left_idx<left_size) { left_object = left->at(left_idx); }
        if (right_idx<right_size) { right_object = right->at(right_idx); }

        if (!left_object) {
            if (!apply_change || check_changetype(right_object->ChangeType())) {
                res->add(right_object);
            }
            right_idx++;
        } else if (!right_object) {
            if (!apply_change || check_changetype(left_object->ChangeType())) {
                res->add(left_object);
            }
            left_idx++;
        } else {
            auto c = obj_cmp(left_object,right_object);
            if (c<0) {
                if (!apply_change || check_changetype(left_object->ChangeType())) {
                    res->add(left_object);
                }
                left_idx++;
            } else if (c==1) {
                if (!apply_change || check_changetype(right_object->ChangeType())) {
                    res->add(right_object);
                }
                right_idx++;
            } else {
                if (!apply_change || check_changetype(right_object->ChangeType())) {
                    res->add(right_object);
                }
                left_idx++;
                right_idx++;
            }
        }
    }
    if (apply_change) {
        for (auto e:res->Objects()) {
            e->SetChangeType(changetype::Normal);
        }
    }
    return res;
}




PrimitiveBlockPtr combine_primitiveblock_many(
    PrimitiveBlockPtr main,
    const std::vector<PrimitiveBlockPtr>& changes) {

    if (changes.size()==0) {
        return main;
    }
    PrimitiveBlockPtr merged_changes;
    merged_changes=changes[0];
    if (changes.size()>1) {
        for (size_t i=1; i < changes.size(); i++) {
            merged_changes = combine_primitiveblock_two(main->Index(), merged_changes, changes[i], false);
        }
    }
    return combine_primitiveblock_two(main->Index(), main, merged_changes, true);
}


template <class T>
int minimalobj_cmp(const T& l, const T& r) {
    if (l.id == r.id) { return 0; }
    if (l.id < r.id) { return -1; }
    return 1;
}

template <class T>
void combine_minimalblock_objs(std::vector<T>& res, const std::vector<T>& left, const std::vector<T>& right, bool apply_change) {
    
    size_t left_idx = 0;
    size_t right_idx= 0;

    for ( ; left_idx<left.size() || right_idx < right.size(); ) {

        if (left_idx==left.size()) {
            if (!apply_change || check_changetype((changetype)right[right_idx].changetype)) {
                res.push_back(right[right_idx]);
            }
            right_idx++;
        } else if (right_idx==right.size()) {
            if (!apply_change || check_changetype((changetype)left[left_idx].changetype)) {
                res.push_back(left[left_idx]);
            }
            left_idx++;
        } else {
            auto c = minimalobj_cmp(left[left_idx],right[right_idx]);
            if (c<0) {
                if (!apply_change || check_changetype((changetype)left[left_idx].changetype)) {
                    res.push_back(left[left_idx]);
                }
                left_idx++;
            } else if (c==1) {
                if (!apply_change || check_changetype((changetype)right[right_idx].changetype)) {
                    res.push_back(right[right_idx]);
                }
                right_idx++;
            } else {
                if (!apply_change || check_changetype((changetype)right[right_idx].changetype)) {
                    res.push_back(right[right_idx]);
                }
                left_idx++;
                right_idx++;
            }
        }
    }
    if (apply_change) {
        for (auto& o: res) {
            o.changetype=0;
        }
    }
    
    
}

minimal::BlockPtr combine_minimalblock_two(
    size_t idx,
    minimal::BlockPtr left,
    minimal::BlockPtr right,
    bool apply_change) {
        
    auto res = std::make_shared<minimal::Block>();
    res->index = idx;
    res->quadtree=left->quadtree;
    
    combine_minimalblock_objs(res->nodes, left->nodes, right->nodes,apply_change);
    combine_minimalblock_objs(res->ways, left->ways, right->ways,apply_change);
    combine_minimalblock_objs(res->relations, left->relations, right->relations,apply_change);
    combine_minimalblock_objs(res->geometries, left->geometries, right->geometries,apply_change);
    
    return res;
        
}

minimal::BlockPtr combine_minimalblock_many(
    minimal::BlockPtr main,
    const std::vector<minimal::BlockPtr>& changes) {
    if (changes.size()==0) {
        return main;
    }
    minimal::BlockPtr merged_changes;
    merged_changes=changes[0];
    if (changes.size()>1) {
        for (size_t i=1; i < changes.size(); i++) {
            merged_changes = combine_minimalblock_two(main->index, merged_changes, changes[i], false);
        }
        //merged_changes = combine_packedblock_vector(changes);
    }
    return combine_minimalblock_two(main->index, main, merged_changes, true);
}
}
