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

#ifndef MAKEGEOMETRIES_HPP
#define MAKEGEOMETRIES_HPP

#include "oqt/geometry/utils.hpp"
#include "oqt/geometry/findminzoom.hpp"
#include <map>
namespace oqt {
namespace geometry {


struct style_info {
    bool IsFeature;
    bool IsArea;
    bool IsWay;
    bool IsNode;
    bool OnlyArea;
    bool IsOtherTags;
    std::set<std::string> ValueList;
    style_info(bool f, bool a, bool w, bool n, bool o) : IsFeature(f),IsArea(a),IsWay(w),IsNode(n),OnlyArea(false),IsOtherTags(o) {}
};

typedef std::map<std::string,style_info> style_info_map;

std::pair<tagvector,int64> filter_node_tags(const style_info_map& style, const tagvector& tags, const std::string& extra_tags_key);
std::tuple<tagvector, bool, int64, int64> filter_way_tags(const style_info_map& style, const tagvector& tags, bool is_ring, bool is_bp, const std::string& extra_tags_key);

PrimitiveBlockPtr make_geometries(const style_info_map& style, const bbox& box, PrimitiveBlockPtr in);



size_t recalculate_quadtree(PrimitiveBlockPtr block, uint64 maxdepth, double buf);
void calculate_minzoom(PrimitiveBlockPtr block, std::shared_ptr<FindMinZoom> minzoom);

std::shared_ptr<BlockHandler> make_geometryprocess(const style_info_map& style, const bbox& box, bool recalc, std::shared_ptr<FindMinZoom> fmz);

}}

#endif //MAKEGEOMETRIES_HPP
