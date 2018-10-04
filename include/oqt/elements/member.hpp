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

#ifndef ELEMENTS_MEMBER_HPP
#define ELEMENTS_MEMBER_HPP


#include "oqt/common.hpp"
#include "oqt/elements/baseelement.hpp"
#include <string>
#include <vector>
namespace oqt {
struct Member {
    Member() : type(ElementType::Node),ref(0),role("") {}
    Member(ElementType t, int64 r, std::string rl) : type(t),ref(r),role(rl) {}
    ElementType type;
    int64 ref;
    std::string role;
};

}
#endif
