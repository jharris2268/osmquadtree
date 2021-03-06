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

#include "oqt/pbfformat/readfileblocks.hpp"
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
    //int64 part, geometry::Ring exterior, std::vector<geometry::Ring> interiors,
    const std::vector<geometry::PolygonPart>& parts,
    int64 zorder, int64 layer, int64 minzoom) {
        
    
    auto cp = std::make_shared<geometry::ComplicatedPolygon>(id, qt, inf, tags, parts, zorder, layer, bbox(), minzoom);
    cp->CheckParts();
    return cp;
}


geometry::mperrorvec process_geometry_py(const geometry::GeometryParameters& params, external_callback cb) {
    
    py::gil_scoped_release r;
    block_callback wrapped = prep_callback(cb, params.numblocks);
    
    return process_geometry(params, wrapped);
}
    
geometry::mperrorvec process_geometry_nothread_py(const geometry::GeometryParameters& params, external_callback cb) {
    
    py::gil_scoped_release r;
    block_callback wrapped = prep_callback(cb, params.numblocks);
    
    return process_geometry_nothread(params, wrapped);
}

geometry::mperrorvec process_geometry_sortblocks_py(const geometry::GeometryParameters& params, external_callback cb) {
    py::gil_scoped_release r;

    block_callback wrapped = prep_callback(cb, params.numblocks);
    
    return process_geometry_sortblocks(params, wrapped);
}


PrimitiveBlockPtr read_blocks_geometry_convfunc(std::shared_ptr<FileBlock> fb) {
    if (!fb) { return PrimitiveBlockPtr(); }
    if (fb->blocktype!="OSMData") { return std::make_shared<PrimitiveBlock>(fb->idx,0); }
    return read_primitive_block(fb->idx,fb->get_data(),false,ReadBlockFlags::Empty,nullptr,geometry::read_geometry);
}

geometry::mperrorvec process_geometry_from_vec_py(
    std::vector<PrimitiveBlockPtr> blocks,
    const geometry::GeometryParameters& params,
    bool nothread, 
    external_callback cb) {

    py::gil_scoped_release r;
    block_callback wrapped = prep_callback(cb, params.numblocks);
    
    return process_geometry_from_vec(blocks, params, nothread, wrapped);
}


std::function<PrimitiveBlockPtr(std::shared_ptr<FileBlock>)> make_read_blocks_geometry_convfunc_filter(bbox filt, int64 minzoom) {
    std::function<bool(int64)> test_minzoom;    
    std::function<bool(bbox)> test_bbox;
    
    if (box_planet(filt) || box_empty(filt)) {
        //pass
    } else {
        test_bbox = [filt](bbox test) { return overlaps(filt, test); };
    }
    
    if (minzoom >= 0) {
        test_minzoom = [minzoom](int64 test) {
      
            if (test<0) { return false; }
            return test <= minzoom;
        };
    }
    
    if ( (!test_bbox) && (!test_minzoom)) {
        return read_blocks_geometry_convfunc;
    }       
    
    return [test_bbox,test_minzoom](std::shared_ptr<FileBlock> fb) {
        auto pb = read_blocks_geometry_convfunc(fb);
        if (!pb) { return pb; }
        
        auto pb2 = std::make_shared<PrimitiveBlock>(pb->Index(), pb->size());
        pb2->SetQuadtree(pb->Quadtree());
        pb2->SetStartDate(pb->StartDate());
        pb2->SetEndDate(pb->EndDate());
        for (auto o: pb->Objects()) {
            auto g = std::dynamic_pointer_cast<BaseGeometry>(o);
            if ((!test_bbox) || test_bbox(g->Bounds())) {
                if ((!test_minzoom) || ((g->MinZoom()) && (test_minzoom(*g->MinZoom())))) {
                    pb2->add(o);
                }
            }
        }
        return pb2;
    };
}       
size_t read_blocks_geometry_py(
    const std::string& filename,
    std::function<bool(std::vector<PrimitiveBlockPtr>)> callback,
    const std::vector<int64>& locs, size_t numchan, size_t numblocks, bbox filt,int64 minzoom) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    std::function<void(PrimitiveBlockPtr)> cbf = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };
    
    if (numchan==0) {
        read_blocks_nothread_convfunc<PrimitiveBlock>(filename, cbf, locs, make_read_blocks_geometry_convfunc_filter(filt,minzoom));
    } else {
        read_blocks_convfunc_primitiveblock(filename, cbf, locs, numchan, make_read_blocks_geometry_convfunc_filter(filt,minzoom));
    }
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
        .def("pack_extras", &BaseGeometry::pack_extras)
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
        //.def_property_readonly("Part", &geometry::ComplicatedPolygon::Part)
        //.def_property_readonly("InnerRings", &geometry::ComplicatedPolygon::InnerRings)
        //.def_property_readonly("OuterRing", &geometry::ComplicatedPolygon::OuterRing)
       
        .def_property_readonly("Area", &geometry::ComplicatedPolygon::Area)
        .def_property_readonly("Parts", &geometry::ComplicatedPolygon::Parts)
        .def_property_readonly("ZOrder", &geometry::ComplicatedPolygon::ZOrder)
        .def_property_readonly("Layer", &geometry::ComplicatedPolygon::Layer)
        
    ;

    m.def("make_point", &make_point, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("lon"), py::arg("lat"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_linestring", &make_linestring, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("refs"), py::arg("lonlats"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_simplepolygon", &make_simplepolygon, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("refs"), py::arg("lonlats"), py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));
    m.def("make_complicatedpolygon", &make_complicatedpolygon, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("quadtree"), py::arg("parts")/*, py::arg("exterior"), py::arg("interiors")*/, py::arg("zorder"),py::arg("layer"),py::arg("minzoom"));


    

    

    py::class_<geometry::XY>(m,"XY")
        .def(py::init<double,double>())
        .def_readonly("x", &geometry::XY::x)
        .def_readonly("y", &geometry::XY::y)
        .def_property_readonly("transform", [](const geometry::XY& ll) { return geometry::inverse_transform(ll.x,ll.y); })
        .def_property_readonly("round_2dp", &geometry::XY::round_2dp)
    ;
    
    /*
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
    */ 
    py::enum_<geometry::PolygonTag::Type>(m,"PolygonTag_Type")
        .value("All", geometry::PolygonTag::Type::All)
        .value("Include", geometry::PolygonTag::Type::Include)
        .value("Exclude", geometry::PolygonTag::Type::Exclude)
    ;
    
    
    py::class_<geometry::PolygonTag>(m,"PolygonTag")
        .def(py::init<std::string,geometry::PolygonTag::Type,std::set<std::string>>())
        .def_readwrite("key",&geometry::PolygonTag::key)
        .def_readwrite("type",&geometry::PolygonTag::type)
        .def_readwrite("values",&geometry::PolygonTag::values)
    ;
    
    py::class_<geometry::ParentTagSpec>(m,"ParentTagSpec")
        .def(py::init<std::string,std::string,std::string,std::map<std::string,int>>())
        .def_readwrite("node_tag", &geometry::ParentTagSpec::node_tag)
        .def_readwrite("out_tag", &geometry::ParentTagSpec::out_tag)
        .def_readwrite("way_tag", &geometry::ParentTagSpec::way_tag)
        .def_readwrite("priority", &geometry::ParentTagSpec::priority)
    ;
    
    
    py::enum_<geometry::RelationTagSpec::Type>(m,"RelationTagSpec_Type")
        .value("Min", geometry::RelationTagSpec::Type::Min)
        .value("Max", geometry::RelationTagSpec::Type::Max)
        .value("List", geometry::RelationTagSpec::Type::List)
    ;
    
    py::class_<geometry::RelationTagSpec>(m,"RelationTagSpec")
        .def(py::init<std::string,std::vector<Tag>,std::string,geometry::RelationTagSpec::Type>())
        .def_readwrite("target_key", &geometry::RelationTagSpec::target_key)
        .def_readwrite("source_filter", &geometry::RelationTagSpec::source_filter)
        .def_readwrite("source_key", &geometry::RelationTagSpec::source_key)
        .def_readwrite("type", &geometry::RelationTagSpec::type)
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
    py::class_<geometry::PolygonPart>(m, "PolygonPart")
        .def(py::init<int64,geometry::Ring,std::vector<geometry::Ring>,double>())
        .def_readonly("index", &geometry::PolygonPart::index)
        .def_readonly("outer", &geometry::PolygonPart::outer)
        .def_readonly("inners", &geometry::PolygonPart::inners)
        .def_readonly("area", &geometry::PolygonPart::area)
    ;
    m.def("polygon_part_wkb",
        [](const geometry::PolygonPart& part, bool transform, bool srid) {
                return py::bytes(geometry::polygon_part_wkb(part,transform,srid));
            }, py::arg("part"), py::arg("transform")=true, py::arg("srid")=true);
    m.def("pack_polygon_part", [](const geometry::PolygonPart& part) { return py::bytes(geometry::pack_polygon_part(part)); } );
    m.def("unpack_polygon_part", &geometry::unpack_polygon_part);
    
    m.def("lonlats_bounds", &geometry::lonlats_bounds);
    m.def("calc_ring_area", [](std::vector<LonLat> ll) { return geometry::calc_ring_area(ll); });
    m.def("calc_ring_centroid", &geometry::calc_ring_centroid);
    m.def("calc_ring_area", [](geometry::Ring rp) { return geometry::calc_ring_area(rp); });
    m.def("ringpart_refs", &geometry::ringpart_refs);
    m.def("ringpart_lonlats", &geometry::ringpart_lonlats);

    m.def("unpack_geometry_element", &geometry::unpack_geometry_element);
    m.def("unpack_geometry_primitiveblock", &geometry::unpack_geometry_primitiveblock);

    
    py::class_<geometry::FindMinZoom, std::shared_ptr<geometry::FindMinZoom>>(m,"FindMinZoom")
        .def("calculate", &geometry::FindMinZoom::calculate)
        .def("check_feature", &geometry::FindMinZoom::check_feature);
    ;
    
    
    
    m.def("findminzoom_onetag", &geometry::make_findminzoom_onetag);
    /*py::class_<findminzoom_onetag, geometry::findminzoom, std::shared_ptr<findminzoom_onetag>>(m,"findminzoom_onetag")
        .def(py::init<findminzoom_onetag::tag_spec, double, double>(), py::arg("spec"), py::arg("minlen"), py::arg("minarea"))
        .def("categories", &findminzoom_onetag::categories)
        .def("areaminzoom", &findminzoom_onetag::areaminzoom)
    ;*/

    m.def("make_geometries", &geometry::make_geometries, py::arg("feature_keys"), py::arg("polygon_tags"), py::arg("other_tags"), py::arg("drop_keys"), py::arg("all_other_tags"), py::arg("all_objs"), py::arg("bbox"), py::arg("block"), py::arg("max_min_zoom_level")=0);
    
    m.def("filter_tags", &geometry::filter_tags, py::arg("feature_keys"), py::arg("other_tags"), py::arg("drop_keys"), py::arg("all_other_tags"), py::arg("all_objs"), py::arg("tags"));
    m.def("check_polygon_tags", &geometry::check_polygon_tags, py::arg("polygon_tags"), py::arg("tags"));
    m.def("calc_zorder", &geometry::calc_zorder, py::arg("tags"));
    
    //m.def("filter_node_tags", &geometry::filter_node_tags, py::arg("style"), py::arg("tags"), py::arg("extra_tags_key"));
    //m.def("filter_way_tags", &geometry::filter_way_tags, py::arg("style"), py::arg("tags"), py::arg("is_ring"), py::arg("is_bp"), py::arg("extra_tags_key"));


    m.def("pack_tags", &geometry::pack_tags);
    m.def("unpack_tags", [](const std::string& str) {
        std::vector<Tag> tgs;
        geometry::unpack_tags(str, tgs);
        return tgs;
    });
    m.def("convert_packed_tags_to_json", &geometry::convert_packed_tags_to_json);


    py::class_<geometry::GeometryParameters>(m, "GeometryParameters")
        .def(py::init<>())
        .def_readwrite("filenames", &geometry::GeometryParameters::filenames)
        //.def_readwrite("callback", &geometry_parameters::callback)
        .def_readwrite("locs", &geometry::GeometryParameters::locs)
        .def_readwrite("numchan", &geometry::GeometryParameters::numchan)
        .def_readwrite("numblocks", &geometry::GeometryParameters::numblocks)
        //.def_readwrite("style", &geometry::GeometryParameters::style)
        .def_readwrite("feature_keys", &geometry::GeometryParameters::feature_keys)
        .def_readwrite("other_keys", &geometry::GeometryParameters::other_keys)
        .def_readwrite("drop_keys", &geometry::GeometryParameters::drop_keys)
        .def_readwrite("all_other_keys", &geometry::GeometryParameters::all_other_keys)
        .def_readwrite("all_objs", &geometry::GeometryParameters::all_objs)
        .def_readwrite("polygon_tags", &geometry::GeometryParameters::polygon_tags)
        
        .def_readwrite("box", &geometry::GeometryParameters::box)
        .def_readwrite("parent_tag_spec", &geometry::GeometryParameters::parent_tag_spec)
        .def_readwrite("relation_tag_spec", &geometry::GeometryParameters::relation_tag_spec)
        .def_readwrite("add_multipolygons", &geometry::GeometryParameters::add_multipolygons)
        .def_readwrite("add_boundary_polygons", &geometry::GeometryParameters::add_boundary_polygons)
        
        .def_readwrite("recalcqts", &geometry::GeometryParameters::recalcqts)
        .def_readwrite("findmz", &geometry::GeometryParameters::findmz)
        .def_readwrite("outfn", &geometry::GeometryParameters::outfn)
        .def_readwrite("indexed", &geometry::GeometryParameters::indexed)
        
            
        .def_readwrite("groups", &geometry::GeometryParameters::groups)
        .def_readwrite("max_min_zoom_level", &geometry::GeometryParameters::max_min_zoom_level)
        //.def_readwrite("csvblock_callback", &geometry_parameters::csvblock_callback)
    ;
    
    py::class_<geometry::GeometryTagsParams>(m, "GeometryTagsParams")
        .def(py::init<>())
        .def_readwrite("feature_keys", &geometry::GeometryTagsParams::feature_keys)
        .def_readwrite("other_keys", &geometry::GeometryTagsParams::other_keys)
        .def_readwrite("drop_keys", &geometry::GeometryTagsParams::drop_keys)
        .def_readwrite("all_other_keys", &geometry::GeometryTagsParams::all_other_keys)
        .def_readwrite("all_objs", &geometry::GeometryTagsParams::all_objs)
    ;
    m.def("process_multipolygon", &geometry::process_multipolygon);

    m.def("process_geometry", &process_geometry_py);
    
    m.def("process_geometry_sortblocks", &process_geometry_sortblocks_py);
    
    m.def("process_geometry_nothread", &process_geometry_nothread_py);
    
    
    
    m.def("process_geometry_from_vec", &process_geometry_from_vec_py);
    

    
    
    m.def("read_blocks_geometry", &read_blocks_geometry_py,
        py::arg("filename"), py::arg("callback"),
        py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4, py::arg("numblocks")=32, py::arg("bbox")=bbox{1,1,0,0},py::arg("minzoom")=-1);
    
    m.def("res_zoom", &geometry::res_zoom);
    
    
    py::class_<geometry::mperrorvec>(m, "mperrorvec")
        .def_readonly("count", &geometry::mperrorvec::count)
        .def("__len__", [](const geometry::mperrorvec& e) { return e.errors.size(); })
        .def("__getitem__", [](const geometry::mperrorvec& e, int64 i) {
            if (i<0) { i += e.errors.size(); }
            return e.errors.at(i);
        })
    ;
    
    
    py::class_<geometry::LonLatStore, std::shared_ptr<geometry::LonLatStore>>(m, "LonLatStore")
        .def("add_tile", &geometry::LonLatStore::add_tile)
        .def("finish", &geometry::LonLatStore::finish)
        .def("get_lonlats", &geometry::LonLatStore::get_lonlats)
    ;

    m.def("make_lonlatstore", &geometry::make_lonlatstore);
    m.def("make_waywithnodes", [](std::shared_ptr<Way> way, const std::vector<LonLat>& lonlats) { return std::make_shared<geometry::WayWithNodes>(way, lonlats); });
    
}
#ifdef INDIVIDUAL_MODULES
PYBIND11_MODULE(_geometry, m) {
    //py::module m("_geometry", "pybind11 example plugin");
    geometry_defs(m);
    //return m.ptr();
}
#endif
