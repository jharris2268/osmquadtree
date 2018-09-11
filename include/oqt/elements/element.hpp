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

#include <list>
namespace oqt {


class element : public rawelement {
    public:
        element(elementtype t, changetype c, int64 i, int64 q, info inf, std::vector<tag> tags);
        uint64 InternalId() const;
        elementtype Type() const;
        changetype ChangeType() const;
        void SetChangeType(changetype ct);
        int64 Id() const;
        info Info() const;
        const tagvector& Tags() const;
        int64 Quadtree() const;
        void SetQuadtree(int64 qt);

        void AddTag(const std::string& k, const std::string& v, bool replaceifpresent=true);

        bool RemoveTag(const std::string& k);
        
        void SetTags(const tagvector& new_tags);

        virtual ~element() {}
        virtual std::shared_ptr<element> copy()=0;

        
        friend bool fix_tags(element& ele);

    private:
        elementtype type_;
        changetype changetype_;
        int64 id_, quadtree_ ;
        info info_;
        tagvector tags_;
};

typedef std::shared_ptr<element> element_ptr;


inline bool element_cmp(const element_ptr& l, const element_ptr& r) {
    return l->InternalId()<r->InternalId();
}

bool fix_tags(element& ele);

}
#endif
