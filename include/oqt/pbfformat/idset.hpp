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

}

#endif //PBFFORMAT_IDSET_HPP
