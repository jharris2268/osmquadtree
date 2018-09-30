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

#ifndef CALCQTS_QTSTORESPLIT_HPP
#define CALCQTS_QTSTORESPLIT_HPP

#include "oqt/calcqts/qtstore.hpp"
namespace oqt {
class QtStoreSplit : public QtStore {
    public:
        virtual int64 split_at()=0;
        virtual size_t num_tiles()=0;
        virtual size_t last_tile()=0;
        virtual bool use_arr()=0;
        virtual std::shared_ptr<QtStore> tile(size_t i)=0;

        virtual void add_tile(size_t i, std::shared_ptr<QtStore> t)=0;
        virtual void merge(std::shared_ptr<QtStoreSplit>)=0;
        virtual ~QtStoreSplit() {}

};
std::shared_ptr<QtStoreSplit> make_qtstore_split(int64 splitat, bool usearr);
}



#endif
