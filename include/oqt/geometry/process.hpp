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

#ifndef GEOMETRY_PROCESS_HPP
#define GEOMETRY_PROCESS_HPP

#include "oqt/geometry/makegeometries.hpp"
#include "oqt/geometry/addwaynodes.hpp"
#include "oqt/geometry/addparenttags.hpp"

#include "oqt/geometry/multipolygons.hpp"
#include "oqt/geometry/handlerelations.hpp"
#include "oqt/geometry/findminzoom.hpp"

#include "oqt/sorting/qttree.hpp"

#include "oqt/pbfformat/readfileparallel.hpp"

#include "oqt/geometry/utils.hpp"

#include <map>
namespace oqt {
namespace geometry {


struct GeometryParameters {
    GeometryParameters() 
        : numchan(4), numblocks(512), all_other_keys(false), box(), add_multipolygons(false),
            add_boundary_polygons(false), recalcqts(false), outfn(""), indexed(false),
            max_min_zoom_level(0), max_number_errors(1000) {}
    
    std::vector<std::string> filenames;
    
    src_locs_map locs;
    size_t numchan;
    size_t numblocks;
    //style_info_map style;
    std::set<std::string> feature_keys;
    std::map<std::string, PolygonTag> polygon_tags;
    std::set<std::string> other_keys;
    bool all_other_keys;
    
    
    bbox box;
    
    
    std::vector<ParentTagSpec> parent_tag_spec;
    std::vector<RelationTagSpec> relation_tag_spec;
    bool add_multipolygons;
    bool add_boundary_polygons;
    
    bool recalcqts;
    std::shared_ptr<FindMinZoom> findmz;
    std::string outfn;
    bool indexed;
    
    std::shared_ptr<QtTree> groups;
    int64 max_min_zoom_level;
    int64 max_number_errors;
    
};

block_callback make_geomprogress(const src_locs_map& locs);

std::vector<block_callback> pack_and_write_callback(
    std::vector<block_callback> callbacks,
    std::string filename, bool indexed, bbox box, size_t numchan,
    bool writeqts, bool writeinfos, bool writerefs);
    
block_callback pack_and_write_callback_nothread(
    block_callback callback,
    std::string filename, bool indexed, bbox box,
    bool writeqts, bool writeinfos, bool writerefs);
    
block_callback process_geometry_blocks(
    std::vector<block_callback> final_callbacks,
    const GeometryParameters& params,
    std::function<void(mperrorvec&)> errors_callback);
    
block_callback process_geometry_blocks_nothread(
    block_callback final_callback,
    const GeometryParameters& params,
    std::function<void(mperrorvec&)> errors_callback);

mperrorvec process_geometry(const GeometryParameters& params, block_callback wrapped);
mperrorvec process_geometry_nothread(const GeometryParameters& params, block_callback wrapped);
mperrorvec process_geometry_sortblocks(const GeometryParameters& params, block_callback cb);

mperrorvec process_geometry_from_vec(
    std::vector<PrimitiveBlockPtr> blocks,
    const GeometryParameters& params,
    bool nothread,
    block_callback callback);
    
}
}

#endif

