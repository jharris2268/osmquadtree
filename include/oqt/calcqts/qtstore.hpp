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

#ifndef CALCQTS_QTSTORE_HPP
#define CALCQTS_QTSTORE_HPP

#include "oqt/common.hpp"

namespace oqt {
class QtStore {
    public:
        virtual void expand(int64 ref, int64 quadtree)=0;
        virtual int64 at(int64 ref)=0;
        virtual bool contains(int64 ref)=0;
        virtual int64 next(int64 ref)=0;
        virtual int64 first()=0;
        
        virtual size_t size()=0;
        
        virtual int64 key()=0;
        virtual std::pair<int64,int64> ref_range()=0;
        virtual ~QtStore() {}
};

std::shared_ptr<QtStore> make_qtstore_map();
std::shared_ptr<QtStore> make_qtstore_vector(int64 min, int64 max, int64 key);
std::shared_ptr<QtStore> make_qtstore_48bit(int64 min, int64 max, int64 key);

std::shared_ptr<QtStore> make_qtstore_vector_move(std::vector<int64>&& pts, int64 min, size_t count, int64 k);
}
#endif
