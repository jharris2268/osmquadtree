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



/*struct StyleInfo {
    bool IsFeature;
    bool IsArea;
    bool IsWay;
    bool IsNode;
    bool OnlyArea;
    bool IsOtherTags;
    std::set<std::string> ValueList;
    StyleInfo(bool f, bool a, bool w, bool n, bool o) : IsFeature(f),IsArea(a),IsWay(w),IsNode(n),OnlyArea(false),IsOtherTags(o) {}
};

typedef std::map<std::string,StyleInfo> style_info_map;


std::pair<tagvector,int64> filter_node_tags(const style_info_map& style, const tagvector& tags, const std::string& extra_tags_key);
std::tuple<tagvector, bool, int64, int64> filter_way_tags(const style_info_map& style, const tagvector& tags, bool is_ring, bool is_bp, const std::string& extra_tags_key);

PrimitiveBlockPtr make_geometries(const , const bbox& box, PrimitiveBlockPtr in);
*/
struct PolygonTag {
    enum Type {
        All,
        Include,
        Exclude
    };
    
    std::string key;
    Type type;
    std::set<std::string> values;
    
    PolygonTag(const std::string& key_, Type type_, const std::set<std::string>& values_) 
        : key(key_), type(type_), values(values_) {}
    
};


std::optional<int64> calc_zorder(const std::vector<Tag>& tags);

std::tuple<bool, std::vector<Tag>, std::optional<int64>> filter_tags(
    const std::set<std::string>& feature_keys,
    const std::set<std::string>& other_keys,
    const std::set<std::string>& drop_keys,
    bool all_other_keys,
    bool all_objs,
    const std::vector<Tag> in_tags);

bool check_polygon_tags(const std::map<std::string, PolygonTag>& polygon_tags, const std::vector<Tag>& tags); 

PrimitiveBlockPtr make_geometries(
    const std::set<std::string>& feature_keys,
    const std::map<std::string, PolygonTag>& polygon_tags,
    const std::set<std::string>& other_keys,
    const std::set<std::string>& drop_keys,
    bool all_other_keys,
    bool all_objs,
    const bbox& box,
    PrimitiveBlockPtr in,
    std::function<bool(ElementPtr)> check_feat);


size_t recalculate_quadtree(PrimitiveBlockPtr block, uint64 maxdepth, double buf);
void calculate_minzoom(PrimitiveBlockPtr block, std::shared_ptr<FindMinZoom> minzoom, int64 max_min_zoom_level);

std::shared_ptr<BlockHandler> make_geometryprocess(
    const std::set<std::string>& feature_keys,
    const std::map<std::string, PolygonTag>& polygon_tags,
    const std::set<std::string>& other_keys,
    const std::set<std::string>& drop_keys,
    bool all_other_keys,
    bool all_objs,
    const bbox& box,
    bool recalc,
    std::shared_ptr<FindMinZoom> fmz,
    int64 max_min_zoom_level);

}}

#endif //MAKEGEOMETRIES_HPP
