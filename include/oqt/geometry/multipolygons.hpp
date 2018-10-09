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

#ifndef MULTIPOLYGONS_HPP
#define MULTIPOLYGONS_HPP

#include "oqt/geometry/makegeometries.hpp"
#include "oqt/geometry/elements/ring.hpp"

#include "oqt/geometry/utils.hpp"
#include <map>
namespace oqt {
namespace geometry {


typedef std::tuple<std::shared_ptr<Relation>,std::string,std::vector<Ring>,std::vector<Ring>,std::vector<std::pair<bool,Ring>>> mperror;
typedef std::vector<mperror> mperrorvec;
std::shared_ptr<BlockHandler> make_multipolygons(
    std::shared_ptr<mperrorvec> multipolygon_errors,
    const style_info_map& style, const bbox& box,
    bool boundary, bool multipolygon);

}}

#endif //MULTIPOLYGONS_HPP
