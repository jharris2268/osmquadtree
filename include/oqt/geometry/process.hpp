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
#include "oqt/geometry/postgiswriter.hpp"
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
        : numchan(1), numblocks(512), box(), add_rels(false),
            add_mps(false), recalcqts(false), outfn(""), indexed(false),
            connstring(""), tableprfx("") {}
    
    std::vector<std::string> filenames;
    
    src_locs_map locs;
    size_t numchan;
    size_t numblocks;
    style_info_map style;
    bbox box;
    parenttag_spec_map apt_spec;
    bool add_rels;
    bool add_mps;
    bool recalcqts;
    std::shared_ptr<FindMinZoom> findmz;
    std::string outfn;
    bool indexed;
    std::string connstring;
    std::string tableprfx;
    PackCsvBlocks::tagspec coltags;
    std::shared_ptr<QtTree> groups;
    
};

mperrorvec process_geometry(const GeometryParameters& params, block_callback wrapped);
mperrorvec process_geometry_nothread(const GeometryParameters& params, block_callback wrapped);
mperrorvec process_geometry_sortblocks(const GeometryParameters& params, block_callback cb);
mperrorvec process_geometry_csvcallback_nothread(const GeometryParameters& params,
    block_callback callback,
    std::function<void(std::shared_ptr<CsvBlock>)> csvblock_callback);

mperrorvec process_geometry_from_vec(
    std::vector<PrimitiveBlockPtr> blocks,
    const GeometryParameters& params,
    block_callback callback);
    
}
}
#endif
