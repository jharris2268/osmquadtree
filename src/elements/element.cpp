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


#include "oqt/elements/element.hpp"
#include <locale>
#include <codecvt>
#include <algorithm>
namespace oqt {


element::element(elementtype t, changetype c, int64 i, int64 q, info inf, std::vector<tag> tags)
    : type_(t),changetype_(c), id_(i), quadtree_(q), info_(inf), tags_(tags) {}

uint64 element::InternalId() const {
    uint64 r = type_;
    r<<=61;
    r += (uint64) id_;
    return r;
}
elementtype element::Type() const { return type_; }
changetype element::ChangeType() const { return changetype_; }
void element::SetChangeType(changetype ct) { changetype_=ct; }
int64 element::Id() const { return id_; }
info element::Info() const  { return info_; }
const tagvector& element::Tags() const { return tags_; }
int64 element::Quadtree() const { return quadtree_; }
void element::SetQuadtree(int64 qt) { quadtree_=qt; }

void element::AddTag(const std::string& k, const std::string& v, bool replaceifpresent) {
    if (replaceifpresent) {
        for (auto& t: tags_) {
            if (t.key==k) {
                t.val=v;
                return;
            }
        }
    }
    tags_.push_back(tag{k,v});
}

bool element::RemoveTag(const std::string& k) {
    if (tags_.empty()) { return false; }
    for (auto it=tags_.begin(); it < tags_.end(); ++it) {
        if (it->key==k) {
            tags_.erase(it);
            return true;
        }
    }
    return false;
}

void element::SetTags(const tagvector& new_tags) {
    tags_ = new_tags;
}

        
bool fix_tags(element& ele) {
    if (ele.tags_.empty()) { return false; }
    std::sort(ele.tags_.begin(),ele.tags_.end(),[](const tag&l, const tag& r) { return l.key<r.key; });
    bool r=false;
    for (auto& t: ele.tags_) {
        if (t.key.find(127)!=std::string::npos) {
            auto s=fix_str(t.key);
            if (s!=t.key) {
                t.key=s;
                r=true;
            }
        }
        if (t.val.find(127)!=std::string::npos) {
            auto s=fix_str(t.val);
            if (s!=t.val) {
                t.val=s;
                r=true;
            }
        }
    }
    return r;
}

}

