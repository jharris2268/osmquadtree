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


Element::Element(ElementType t, changetype c, int64 i, int64 q, ElementInfo inf, std::vector<Tag> tags)
    : type_(t),changetype_(c), id_(i), quadtree_(q), info_(inf), tags_(tags) {}


int64 internal_id(ElementType et, int64 id) {
    uint64 r = (uint64) et;
    r<<=61;
    r += (uint64) id;
    return r;
}

uint64 Element::InternalId() const {
    return internal_id(type_,id_);
}
ElementType Element::Type() const { return type_; }
changetype Element::ChangeType() const { return changetype_; }
void Element::SetChangeType(changetype ct) { changetype_=ct; }
int64 Element::Id() const { return id_; }
const ElementInfo& Element::Info() const  { return info_; }
const std::vector<Tag>& Element::Tags() const { return tags_; }
int64 Element::Quadtree() const { return quadtree_; }
void Element::SetQuadtree(int64 qt) { quadtree_=qt; }



void Element::AddTag(const std::string& k, const std::string& v, bool replaceifpresent) {
//void add_tag(std::vector<Tag>& tags_, const std::string& k, const std::string& v, bool replaceifpresent) {
    if (replaceifpresent) {
        for (auto& t: tags_) {
            if (t.key==k) {
                t.val=v;
                return;
            }
        }
    }
    tags_.push_back(Tag{k,v});
}

bool Element::RemoveTag(const std::string& k) {
//bool remove_tag(std::vector<Tag>& tags_, const std::string& k) {
    if (tags_.empty()) { return false; }
    for (auto it=tags_.begin(); it < tags_.end(); ++it) {
        if (it->key==k) {
            tags_.erase(it);
            return true;
        }
    }
    return false;
}

void Element::SetTags(const std::vector<Tag>& new_tags) {
    tags_ = new_tags;
}




    
bool fix_tags(Element& ele) {
    
    //auto& tags_ = ele.EditTags();
    
    if (ele.tags_.empty()) { return false; }
    std::sort(ele.tags_.begin(),ele.tags_.end(),[](const Tag&l, const Tag& r) { return l.key<r.key; });
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

std::string fix_str(const std::string& s) {
    
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string t = converter.from_bytes(s);
    std::u32string u;
    u.reserve(t.size());    
    for (const auto& c: t) {
        if (c!=127) {
            u.push_back(c);
        }
    }
    return converter.to_bytes(u);
}
}

