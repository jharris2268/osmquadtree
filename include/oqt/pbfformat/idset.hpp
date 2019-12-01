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

#ifndef PBFFORMAT_IDSET_HPP
#define PBFFORMAT_IDSET_HPP


#include "oqt/elements/baseelement.hpp"


namespace oqt {
class IdSet {
    public:
        virtual bool contains(ElementType ty, int64 id) const=0;
        virtual ~IdSet() {}
};

typedef std::shared_ptr<IdSet> IdSetPtr;

class IdSetRange : public IdSet {
    public:
        IdSetRange(ElementType ty_, int64 min_id_, int64 max_id_) : 
            ty(ty_), min_id(min_id_), max_id(max_id_) {}
        
        virtual ~IdSetRange() {}
        
        virtual bool contains(ElementType t, int64 id) const {
            if (t != ty) { return false; }
            if (id < min_id) { return false; }
            if ((max_id > 0) && (id >= max_id)) { return false; }
            return true;
        }
        
        ElementType ty;
        int64 min_id;
        int64 max_id;
};
         
}

#endif //PBFFORMAT_IDSET_HPP
