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

#ifndef GEOMETRY_MULTIPOLYGONS_HPP
#define GEOMETRY_MULTIPOLYGONS_HPP

#include "oqt/geometry/makegeometries.hpp"
#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"
#include "oqt/geometry/utils.hpp"
#include <map>
namespace oqt {
namespace geometry {


typedef std::tuple<std::shared_ptr<Relation>,std::string,std::vector<Ring>,std::vector<Ring>,std::vector<std::pair<bool,Ring>>> mperror;
//typedef std::vector<mperror> mperrorvec;
struct mperrorvec{
    mperrorvec() : count(0) {}
    
    std::vector<mperror> errors;
    size_t count;
};


/*
std::shared_ptr<BlockHandler> make_multipolygons(
    std::function<void(mperrorvec&)> errors_callback,
    const MultipolygonParams& params,
    int64 max_number_errors);
*/


std::pair<std::shared_ptr<ComplicatedPolygon>, std::optional<mperror>> process_multipolygon(
    const GeometryTagsParams& params, const bbox& box, std::shared_ptr<Relation> r, const std::vector<std::pair<bool,std::shared_ptr<WayWithNodes>>>& ways);

std::shared_ptr<BlockHandler> make_multipolygons(
    std::function<void(mperrorvec&)> errors_callback,
    const std::set<std::string>& feature_keys,  
    const std::set<std::string>& other_keys,
    const std::set<std::string>& drop_keys,
    bool all_other_keys,
    bool all_objs, 
    const bbox& box,
    bool boundary, bool multipolygon, int64 max_number_errors);

}}

#endif //MULTIPOLYGONS_HPP
