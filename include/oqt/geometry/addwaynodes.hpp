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

#ifndef ADDWAYNODES_HPP
#define ADDWAYNODES_HPP

#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/geometry.hpp"

#include "oqt/elements/block.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/geometry/utils.hpp"
namespace oqt {
namespace geometry {





class lonlatstore {
    public:
        virtual void add_tile(PrimitiveBlockPtr block)=0;
        virtual lonlatvec get_lonlats(std::shared_ptr<Way> way)=0;
        virtual void finish()=0;
        virtual ~lonlatstore() {}
};

std::shared_ptr<lonlatstore> make_lonlatstore();

PrimitiveBlockPtr add_waynodes(std::shared_ptr<lonlatstore> lls, PrimitiveBlockPtr bl);
/*
void add_waynodes_process(
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> in,
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> out,
    std::shared_ptr<lonlatstore> lls);
*/


typedef std::function<void(PrimitiveBlockPtr)> block_callback;
block_callback make_addwaynodes_cb(block_callback cb);
}}

#endif //ADDWAYNODES_HPP
