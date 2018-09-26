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

#ifndef ELEMENTS_RELATION_HPP
#define ELEMENTS_RELATION_HPP

#include "oqt/elements/element.hpp"
#include "oqt/pbfformat/idset.hpp"
#include "oqt/elements/member.hpp"
namespace oqt {
class Relation : public Element {
    public:
        Relation(changetype c, int64 i, int64 q, info inf, tagvector tags, memvector mems);
        virtual ~Relation() {}

        const memvector&  Members() const;
        
        virtual ElementPtr copy();
        
        bool filter_members(std::shared_ptr<idset> ids);
        friend bool fix_members(Relation& rel);
    private:
        memvector mems_;
};

bool fix_members(Relation& rel);
}
#endif
