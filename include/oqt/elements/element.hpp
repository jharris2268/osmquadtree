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

#ifndef ELEMENTS_ELEMENT_HPP
#define ELEMENTS_ELEMENT_HPP

#include "oqt/common.hpp"

#include "oqt/elements/rawelement.hpp"
#include "oqt/elements/info.hpp"
#include "oqt/elements/tag.hpp"
#include "oqt/elements/packedobj.hpp"

#include <list>
namespace oqt {
//struct PbfTag;

class element : public rawelement {
    public:
        element(elementtype t, changetype c, int64 i, int64 q, info inf, std::vector<tag> tags)
            : type_(t),changetype_(c), id_(i), quadtree_(q), info_(inf), tags_(tags) {}

        uint64 InternalId() const {
            uint64 r = type_;
            r<<=61;
            r += (uint64) id_;
            return r;
        }
        elementtype Type() const { return type_; }
        changetype ChangeType() const { return changetype_; }
        void SetChangeType(changetype ct) { changetype_=ct; }
        int64 Id() const { return id_; }
        info Info() const  { return info_; }
        const tagvector& Tags() const { return tags_; }
        int64 Quadtree() const { return quadtree_; }
        void SetQuadtree(int64 qt) { quadtree_=qt; }

        void AddTag(const std::string& k, const std::string& v, bool replaceifpresent=true) {
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

        bool RemoveTag(const std::string& k) {
            if (tags_.empty()) { return false; }
            for (auto it=tags_.begin(); it < tags_.end(); ++it) {
                if (it->key==k) {
                    tags_.erase(it);
                    return true;
                }
            }
            return false;
        }
        
        void SetTags(const tagvector& new_tags) {
            tags_ = new_tags;
        }

        virtual ~element() {}
        virtual std::shared_ptr<element> copy()=0;

        virtual packedobj_ptr pack();
        virtual std::list<PbfTag> pack_extras() const=0;
        
        friend bool fix_tags(element& ele);

    private:
        elementtype type_;
        changetype changetype_;
        int64 id_, quadtree_ ;
        info info_;
        tagvector tags_;
};

typedef std::shared_ptr<element> element_ptr;

element_ptr unpack_packedobj(packedobj_ptr p);
std::shared_ptr<element> unpack_packedobj(int64 ty, int64 id, changetype ct, int64 qt, const std::string& d);

inline bool element_cmp(const element_ptr& l, const element_ptr& r) {
    return l->InternalId()<r->InternalId();
}


class PbfTag;
std::tuple<info,tagvector,std::list<PbfTag>> unpack_packedobj_common(const std::string& data);

bool fix_tags(element& ele);
}
#endif
