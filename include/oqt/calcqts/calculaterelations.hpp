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

#ifndef CALCQTS_CALCULATERELATIONS_HPP
#define CALCQTS_CALCULATERELATIONS_HPP

#include "oqt/common.hpp"
#include "oqt/elements/minimalblock.hpp"
#include "oqt/calcqts/qtstore.hpp"
#include "oqt/calcqts/waynodes.hpp"
namespace oqt {
class calculate_relations {
    public:
        
        virtual void add_nodes(std::shared_ptr<minimalblock>)=0;
        virtual void add_ways(std::shared_ptr<qtstore>)=0;
        
        virtual void finish_alt(std::function<void(int64,int64)>)=0;

        virtual std::string str()=0;
        virtual void add_relations_data(std::shared_ptr<minimalblock> data)=0;
        virtual ~calculate_relations() {}
};

std::shared_ptr<calculate_relations> make_calculate_relations();
}

#endif
