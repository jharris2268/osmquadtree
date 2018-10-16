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

#include "oqt_python.hpp"
#include "oqt/utils/splitcallback.hpp"

#include "oqt/geometry/process.hpp"
#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"

#include <cmath> 
using namespace oqt;

ElementPtr make_point(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt, int64 lon, int64 lat, int64 layer, int64 minzoom) {
    return std::make_shared<geometry::Point>(id, qt, inf, tags, lon, lat, layer, minzoom);
}

ElementPtr make_linestring(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt, std::vector<int64> refs, std::vector<LonLat> lonlats, int64 zorder, int64 layer, int64 minzoom) {
    double ln = geometry::calc_line_length(lonlats);
    bbox bounds = geometry::lonlats_bounds(lonlats);
    return std::make_shared<geometry::Linestring>(id, qt, inf, tags, refs, lonlats, zorder, layer, ln, bounds, minzoom);
}

ElementPtr make_simplepolygon(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt, std::vector<int64> refs, std::vector<LonLat> lonlats, int64 zorder, int64 layer, int64 minzoom) {
    bool reversed=false;
    double ar = geometry::calc_ring_area(lonlats);
    if (ar < 0) {
        ar*=-1;
        reversed=true;
    }
    bbox bounds = geometry::lonlats_bounds(lonlats);
    return std::make_shared<geometry::SimplePolygon>(id, qt, inf, tags, refs, lonlats, zorder, layer, ar, bounds, minzoom,reversed);
}

ElementPtr make_complicatedpolygon(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 qt,
    int64 part, geometry::Ring exterior, std::vector<geometry::Ring> interiors, int64 zorder, int64 layer, int64 minzoom) {
    double ar = geometry::calc_ring_area(exterior);
    if (!interiors.empty()) {
        for (const auto& ii : interiors) {
            ar -= geometry::calc_ring_area(ii);
        }
    }
    bbox bounds = geometry::lonlats_bounds(geometry::ringpart_lonlats(exterior));
    return std::make_shared<geometry::ComplicatedPolygon>(id, qt, inf, tags, part, exterior, interiors, zorder, layer, ar, bounds, minzoom);
}
typedef std::function<bool(std::vector<PrimitiveBlockPtr>)> external_callback;
typedef std::function<void(PrimitiveBlockPtr)> block_callback;

std::pair<size_t,geometry::mperrorvec> process_geometry_py(const geometry::GeometryParameters& params, external_callback cb) {
    
    py::gil_scoped_release r;
    block_callback wrapped;
    
    auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(cb),params.numblocks);
    wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    
    return std::make_pair(collect->total(), process_geometry(params, wrapped));
}
    
std::pair<size_t,geometry::mperrorvec> process_geometry_nothread_py(const geometry::GeometryParameters& params, external_callback callback) {
    
    py::gil_scoped_release r;
    block_callback wrapped;
    auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),params.numblocks);
    wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    
    return std::make_pair(collect->total(), process_geometry_nothread(params, wrapped));
}

std::pair<size_t,geometry::mperrorvec> process_geometry_sortblocks_py(const geometry::GeometryParameters& params, external_callback callback) {
    py::gil_scoped_release r;

    block_callback wrapped;
    auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),params.numblocks);
    wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    
    return std::make_pair(collect->total(), process_geometry_sortblocks(params, wrapped));
}

std::pair<size_t,geometry::mperrorvec> process_geometry_csvcallback_nothread_py(const geometry::GeometryParameters& params,
    external_callback callback,
    std::function<void(std::shared_ptr<geometry::CsvBlock>)> csvblock_callback) {

    py::gil_scoped_release r;
    
    block_callback wrapped;
    auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),params.numblocks);
    wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    
    return std::make_pair(collect->total(), process_geometry_csvcallback_nothread(params, wrapped, wrap_callback(csvblock_callback)));
}

PrimitiveBlockPtr read_blocks_geometry_convfunc(std::shared_ptr<FileBlock> fb) {
    if (!fb) { return PrimitiveBlockPtr(); }
    if (fb->blocktype!="OSMData") { return std::make_shared<PrimitiveBlock>(fb->idx,0); }
    return read_primitive_block(fb->idx,fb->get_data(),false,15,nullptr,geometry::read_geometry);
}

std::pair<size_t,geometry::mperrorvec> process_geometry_from_vec_py(
    std::vector<PrimitiveBlockPtr> blocks,
    const geometry::GeometryParameters& params,
    external_callback callback) {

    py::gil_scoped_release r;
    block_callback wrapped;
    auto collect = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),params.numblocks);
    wrapped = [collect](PrimitiveBlockPtr bl) { collect->call(bl); };
    
    return std::make_pair(collect->total(), process_geometry_from_vec(blocks, params, wrapped));
}







std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> make_read_blocks_geometry_convfunc_filter(bbox filt) {
    if (box_planet(filt)) { return read_blocks_geometry_convfunc; }
    if (box_empty(filt)) { return read_blocks_geometry_convfunc; }
    return [filt](std::shared_ptr<FileBlock> fb) {
        auto pb = read_blocks_geometry_convfunc(fb);
        if (!pb) { return pb; }
        
        auto pb2 = std::make_shared<PrimitiveBlock>(pb->Index(), pb->size());
        pb2->SetQuadtree(pb->Quadtree());
        pb2->SetStartDate(pb->StartDate());
        pb2->SetEndDate(pb->EndDate());
        for (auto o: pb->Objects()) {
            auto g = std::dynamic_pointer_cast<BaseGeometry>(o);
            if (overlaps(filt, g->Bounds())) {
                pb2->add(o);
            }
        }
        return pb2;
    };
}       

size_t read_blocks_geometry_py(
    const std::string& filename,
    std::function<bool(std::vector<PrimitiveBlockPtr>)> callback,
    const std::vector<int64>& locs, size_t numchan, size_t numblocks, bbox filt) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    std::function<void(PrimitiveBlockPtr)> cbf = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };
    
    read_blocks_convfunc_primitiveblock(filename, cbf, locs, numchan, make_read_blocks_geometry_convfunc_filter(filt));
    return cb->total();
}
    
                

PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void geometry_defs(py::module& m) {
   


    py::class_<geometry::WayWithNodes, Element, std::shared_ptr<geometry::WayWithNodes>>(m, "WayWithNodes")
        .def_property_readonly("Refs", &geometry::WayWithNodes::Refs)
        .def_property_readonly("LonLats", &geometry::WayWithNodes::LonLats)
        .def_property_readonly("Bounds", &geometry::WayWithNodes::Bounds)
        
    ;
    py::class_<BaseGeometry, Element, std::shared_ptr<BaseGeometry>>(m, "BaseGeometry")
        .def_property_readonly("OriginalType", &BaseGeometry::OriginalType)
        .def_property_readonly("Bounds", &BaseGeometry::Bounds)
        .def_property_readonly("MinZoom", &BaseGeometry::MinZoom)
        .def("SetMinZoom", &BaseGeometry::SetMinZoom)
        .def("Wkb", [](BaseGeometry& p,  bool transform, bool srid) { return py::bytes(p.Wkb(transform,srid)); }, py::arg("transform")=true, py::arg("srid")=true)
    ;
    
    py::class_<GeometryPacked, BaseGeometry, std::shared_ptr<GeometryPacked>>(m,"GeometryPacked")
        .def("unpack_geometry", [](const GeometryPacked& pp) { throw std::domain_error("not implemented"); })
    ;
    
    py::class_<geometry::Point, BaseGeometry, std::shared_ptr<geometry::Point>>(m, "Point")
        .def_property_readonly("LonLat", &geometry::Point::LonLat)
        .def_property_readonly("Layer", &geometry::Point::Layer)
        
        
    ;

    py::class_<geometry::Linestring, BaseGeometry, std::shared_ptr<geometry::Linestring>>(m, "Linestring")
        .def_property_readonly("Refs", &geometry::Linestring::Refs)
        .def_property_readonly("LonLats", &geometry::Linestring::LonLats)
        
        .def_property_readonly("ZOrder", &geometry::Linestring::ZOrder)
        .def_property_readonly("Layer", &geometry::Linestring::Layer)
        .def_property_readonly("Length", &geometry::Linestring::Length)
        
    ;

    py::class_<geometry::SimplePolygon, BaseGeometry, std::shared_ptr<geometry::SimplePolygon>>(m, "SimplePolygon")
        .def_property_readonly("Refs", &geometry::SimplePolygon::Refs)
        .def_property_readonly("LonLats", &geometry::SimplePolygon::LonLats)
        
        .def_property_readonly("Area", &geometry::SimplePolygon::Area)
        .def_property_readonly("ZOrder", &geometry::SimplePolygon::ZOrder)
        .def_property_readonly("Layer", &geometry::SimplePolygon::Layer)
        .def_property_readonly("Reversed", &geometry::SimplePolygon::Reversed)
        
    ;

    py::class_<geometry::ComplicatedPolygon, BaseGeometry, std::shared_ptr<geometry::ComplicatedPolygon>>(m, "ComplicatedPolygon")
    .def_property_readonly("Part", &geometry::ComplicatedPolygon::Part)
        .def_property_readonly("InnerRings", &geometry::ComplicatedPolygon::InnerRings)
        .def_property_readonly("OuterRing", &geometry::ComplicatedPolygon::OuterRing)
       
        .def_property_readonly("Area", &geometry::ComplicatedPolygon::Area)
        .def_property_readonly("ZOrder", &geometry::ComplicatedPolygon::ZOrder)
        .def_property_readonly("Layer", &geometry::ComplicatedPolygon::Layer)
        
    ;

    m.def("make_point", &make_point, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("lon"), py::arg("lat"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_linestring", &make_linestring, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("refs"), py::arg("lonlats"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_simplepolygon", &make_simplepolygon, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("refs"), py::arg("lonlats"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_complicatedpolygon", &make_complicatedpolygon, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("part"), py::arg("exterior"), py::arg("interiors"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));


    py::class_<LonLat>(m,"LonLat")
        .def(py::init<int64,int64>())
        .def_readonly("lon", &LonLat::lon)
        .def_readonly("lat", &LonLat::lat)
        .def_property_readonly("transform", [](const LonLat& ll) { return geometry::forward_transform(ll.lon,ll.lat); })
    ;

    py::class_<geometry::XY>(m,"XY")
        .def(py::init<double,double>())
        .def_readonly("x", &geometry::XY::x)
        .def_readonly("y", &geometry::XY::y)
        .def_property_readonly("transform", [](const geometry::XY& ll) { return geometry::inverse_transform(ll.x,ll.y); })
    ;
    py::class_<geometry::StyleInfo>(m,"StyleInfo")
        .def(py::init<bool,bool,bool,bool,bool>(),py::arg("IsFeature"),py::arg("IsArea"),py::arg("IsWay"),py::arg("IsNode"),py::arg("IsOtherTags"))
        .def_readwrite("IsFeature",&geometry::StyleInfo::IsFeature)
        .def_readwrite("IsNode",&geometry::StyleInfo::IsNode)
        .def_readwrite("IsWay",&geometry::StyleInfo::IsWay)
        .def_readwrite("IsArea",&geometry::StyleInfo::IsArea)
        .def_readwrite("OnlyArea",&geometry::StyleInfo::OnlyArea)
        .def_readwrite("IsOtherTags", &geometry::StyleInfo::IsOtherTags)
        .def_readwrite("ValueList", &geometry::StyleInfo::ValueList)
        .def("addValue", [](geometry::StyleInfo& s, std::string v) { s.ValueList.insert(v); })
        .def("removeValue", [](geometry::StyleInfo& s, std::string v) { s.ValueList.erase(v); })
    ;

    py::class_<geometry::ParentTagSpec>(m,"ParentTagSpec")
        .def(py::init<std::string,std::string,std::string,std::map<std::string,int>>())
        .def_readwrite("node_tag", &geometry::ParentTagSpec::node_tag)
        .def_readwrite("out_tag", &geometry::ParentTagSpec::out_tag)
        .def_readwrite("way_tag", &geometry::ParentTagSpec::way_tag)
        .def_readwrite("priority", &geometry::ParentTagSpec::priority)
    ;

    py::class_<geometry::Ring::Part>(m,"RingPart")
        .def(py::init<int64,const std::vector<int64>&,const std::vector<LonLat>&,bool>())
        .def_readonly("orig_id", &geometry::Ring::Part::orig_id)
        .def_readonly("refs", &geometry::Ring::Part::refs)
        .def_readonly("lonlats", &geometry::Ring::Part::lonlats)
        .def_readonly("reversed", &geometry::Ring::Part::reversed)
    ;
    py::class_<geometry::Ring>(m,"Ring")
        .def(py::init<std::vector<geometry::Ring::Part>>())
        .def_readonly("parts", &geometry::Ring::parts)
    ;
    
    m.def("lonlats_bounds", &geometry::lonlats_bounds);
    m.def("calc_ring_area", [](std::vector<LonLat> ll) { return geometry::calc_ring_area(ll); });
    m.def("calc_ring_area", [](geometry::Ring rp) { return geometry::calc_ring_area(rp); });
    m.def("ringpart_refs", &geometry::ringpart_refs);
    m.def("ringpart_lonlats", &geometry::ringpart_lonlats);

    m.def("unpack_geometry_element", &geometry::unpack_geometry_element);
    m.def("unpack_geometry_primitiveblock", &geometry::unpack_geometry_primitiveblock);

    
    py::class_<geometry::CsvBlock, std::shared_ptr<geometry::CsvBlock>>(m,"CsvBlock")
        .def_readonly("points", &geometry::CsvBlock::points)
        .def_readonly("lines", &geometry::CsvBlock::lines)
        .def_readonly("polygons", &geometry::CsvBlock::polygons)
    ;
    py::class_<geometry::CsvRows>(m, "CsvRows")
        .def("__getitem__", &geometry::CsvRows::at)
        .def("__len__", &geometry::CsvRows::size)
        .def("data", [](const geometry::CsvRows& c) { return py::bytes(c.data_blob()); })
    ;
    
    py::class_<geometry::FindMinZoom, std::shared_ptr<geometry::FindMinZoom>>(m,"FindMinZoom")
        .def("calculate", &geometry::FindMinZoom::calculate)
    ;
    
    
    m.def("findminzoom_onetag", &geometry::make_findminzoom_onetag);
    /*py::class_<findminzoom_onetag, geometry::findminzoom, std::shared_ptr<findminzoom_onetag>>(m,"findminzoom_onetag")
        .def(py::init<findminzoom_onetag::tag_spec, double, double>(), py::arg("spec"), py::arg("minlen"), py::arg("minarea"))
        .def("categories", &findminzoom_onetag::categories)
        .def("areaminzoom", &findminzoom_onetag::areaminzoom)
    ;*/

    m.def("make_geometries", &geometry::make_geometries, py::arg("style"), py::arg("bbox"), py::arg("block"));
    m.def("filter_node_tags", &geometry::filter_node_tags, py::arg("style"), py::arg("tags"), py::arg("extra_tags_key"));
    m.def("filter_way_tags", &geometry::filter_way_tags, py::arg("style"), py::arg("tags"), py::arg("is_ring"), py::arg("is_bp"), py::arg("extra_tags_key"));


    m.def("pack_tags", &geometry::pack_tags);
    m.def("unpack_tags", [](const std::string& str) {
        std::vector<Tag> tgs;
        geometry::unpack_tags(str, tgs);
        return tgs;
    });
    m.def("convert_packed_tags_to_json", &geometry::convert_packed_tags_to_json);


    py::class_<geometry::GeometryParameters>(m, "geometry_parameters")
        .def(py::init<>())
        .def_readwrite("filenames", &geometry::GeometryParameters::filenames)
        //.def_readwrite("callback", &geometry_parameters::callback)
        .def_readwrite("locs", &geometry::GeometryParameters::locs)
        .def_readwrite("numchan", &geometry::GeometryParameters::numchan)
        .def_readwrite("numblocks", &geometry::GeometryParameters::numblocks)
        .def_readwrite("style", &geometry::GeometryParameters::style)
        .def_readwrite("box", &geometry::GeometryParameters::box)
        .def_readwrite("apt_spec", &geometry::GeometryParameters::apt_spec)
        .def_readwrite("add_rels", &geometry::GeometryParameters::add_rels)
        .def_readwrite("add_mps", &geometry::GeometryParameters::add_mps)
        .def_readwrite("recalcqts", &geometry::GeometryParameters::recalcqts)
        .def_readwrite("findmz", &geometry::GeometryParameters::findmz)
        .def_readwrite("outfn", &geometry::GeometryParameters::outfn)
        .def_readwrite("indexed", &geometry::GeometryParameters::indexed)
        .def_readwrite("connstring", &geometry::GeometryParameters::connstring)
        .def_readwrite("tableprfx", &geometry::GeometryParameters::tableprfx)
        .def_readwrite("coltags", &geometry::GeometryParameters::coltags)
        .def_readwrite("groups", &geometry::GeometryParameters::groups)
        //.def_readwrite("csvblock_callback", &geometry_parameters::csvblock_callback)
    ;


    m.def("process_geometry", &process_geometry_py/* ,
       py::arg("filenames"), py::arg("callback"),
        py::arg("locs"), py::arg("numchan"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"),py::arg("outfn"), py::arg("indexed"),
        py::arg("connstring"), py::arg("table_prfx"), py::arg("coltags")*/
    );
    
    m.def("process_geometry_sortblocks", &process_geometry_sortblocks_py/*, 
        py::arg("filenames"), py::arg("callback"),
        py::arg("locs"), py::arg("numchan"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"),py::arg("outfn"), py::arg("indexed"),
        py::arg("groups")*/
    );
    
    m.def("process_geometry_nothread", &process_geometry_nothread_py/*,
        py::arg("filenames"), py::arg("callback"),
        py::arg("locs"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"), py::arg("outfn"), py::arg("indexed"),
        py::arg("connstring"), py::arg("table_prfx"), py::arg("coltags")*/
    );
    
    m.def("process_geometry_csvcallback_nothread", &process_geometry_csvcallback_nothread_py/*,
        py::arg("filenames"), py::arg("callback"),py::arg("csvblock_callback"),
        py::arg("locs"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz"), py::arg("coltags")*/
    );
    
    m.def("process_geometry_from_vec", &process_geometry_from_vec_py/*,
        py::arg("blocks"), py::arg("callback"), py::arg("numblocks"),
        py::arg("style"), py::arg("box"), py::arg("apt_spec"),
        py::arg("add_rels"), py::arg("add_mps"), py::arg("recalcqts"),
        py::arg("findmz")*/
    );
    

    
    
    m.def("read_blocks_geometry", &read_blocks_geometry_py,
        py::arg("filename"), py::arg("callback"),
        py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4, py::arg("numblocks")=32, py::arg("bbox")=bbox{1,1,0,0});
    
    m.def("res_zoom", &geometry::res_zoom);
    
    
    m.def("point_in_poly", &point_in_poly);
    
    m.def("segment_intersects", &segment_intersects);
    m.def("line_intersects", &line_intersects);
    m.def("line_box_intersects", &line_box_intersects);
    
    py::class_<geometry::PostgisWriter, std::shared_ptr<geometry::PostgisWriter>>(m, "PostgisWriter")
        .def("finish", &geometry::PostgisWriter::finish)
        .def("call", &geometry::PostgisWriter::call)
    ;
    m.def("make_postgiswriter", &geometry::make_postgiswriter);
    
    py::class_<geometry::PackCsvBlocks, std::shared_ptr<geometry::PackCsvBlocks>>(m, "PackCsvBlocks")
        .def("call", &geometry::PackCsvBlocks::call)
    ;
    m.def("make_pack_csvblocks", &geometry::make_pack_csvblocks);
    
    
}
