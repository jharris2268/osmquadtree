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



#include "oqt/elements/way.hpp"

namespace oqt {


Way::Way(changetype c, int64 i, int64 q, info inf, tagvector tags, refvector refs)
    : Element(ElementType::Way,c,i,q,inf,tags), refs_(refs) {}


const refvector& Way::Refs() const { return refs_; }

        

/*
std::list<PbfTag> way::pack_extras() const {
    return {PbfTag{8,0,writePackedDelta(refs_)}};
}
  */      
ElementPtr Way::copy() { return std::make_shared<Way>(ChangeType(),Id(),Quadtree(),Info(),Tags(),Refs()); }
    
}


