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

#include "oqt/elements/baseelement.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/elements/combineblocks.hpp"
#include "oqt/elements/element.hpp"
#include "oqt/elements/header.hpp"
#include "oqt/elements/info.hpp"
#include "oqt/elements/member.hpp"
#include "oqt/elements/minimalblock.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/quadtree.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/tag.hpp"
#include "oqt/elements/way.hpp"

using namespace oqt;

ElementPtr make_node(int64 id, ElementInfo inf, std::vector<Tag> tags, int64 lon, int64 lat, int64 qt, changetype ct) {
    return std::make_shared<Node>(ct, id, qt, inf, tags, lon, lat);
}

ElementPtr make_way(int64 id, ElementInfo inf, std::vector<Tag> tags, std::vector<int64> refs, int64 qt, changetype ct) {
    return std::make_shared<Way>(ct, id, qt, inf, tags,refs);
}

ElementPtr make_relation(int64 id, ElementInfo inf, std::vector<Tag> tags, std::vector<Member> mems, int64 qt, changetype ct) {
    return std::make_shared<Relation>(ct, id, qt, inf, tags, mems);
}



PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void elements_defs(py::module& m) {
    
    m.def("quadtree_string", &quadtree::string);
    m.def("quadtree_from_string", &quadtree::from_string);

    m.def("quadtree_from_tuple", &quadtree::from_tuple);
    m.def("quadtree_tuple", &quadtree::tuple);

    m.def("quadtree_common", &quadtree::common);
    m.def("quadtree_round", &quadtree::round);

    m.def("quadtree_bbox", &quadtree::bbox);
    m.def("quadtree_calculate", &quadtree::calculate);
    
    m.def("overlaps_quadtree", &overlaps_quadtree);

    py::enum_<changetype>(m, "changetype")
        .value("Normal", changetype::Normal)
        .value("Delete", changetype::Delete)
        .value("Remove", changetype::Remove)
        .value("Unchanged", changetype::Unchanged)
        .value("Modify", changetype::Modify)
        .value("Create", changetype::Create);
        //.export_values();
    py::enum_<ElementType>(m, "ElementType")
        .value("Node", ElementType::Node)
        .value("Way", ElementType::Way)
        .value("Relation", ElementType::Relation)
        .value("Point", ElementType::Point)
        .value("Linestring", ElementType::Linestring)
        .value("SimplePolygon", ElementType::SimplePolygon)
        .value("ComplicatedPolygon", ElementType::ComplicatedPolygon)
        .value("WayWithNodes", ElementType::WayWithNodes)
        .value("Unknown", ElementType::Unknown);
        //.export_values();
        

    

    py::class_<PrimitiveBlock, PrimitiveBlockPtr>(m, "PrimitiveBlock")
        .def(py::init<int64,size_t>())
        .def_property_readonly("index", &PrimitiveBlock::Index)
        .def_property("Quadtree", &PrimitiveBlock::Quadtree, &PrimitiveBlock::SetQuadtree)
        .def_property("StartDate", &PrimitiveBlock::StartDate, &PrimitiveBlock::SetStartDate)
        .def_property("EndDate", &PrimitiveBlock::EndDate, &PrimitiveBlock::SetEndDate)
        .def("__len__", [](const PrimitiveBlock& pb) { return pb.size(); })
        .def("__getitem__", [](const PrimitiveBlock& pb, int i) { if (i<0) { i+= pb.size(); } return pb.at(i); })
        .def("__setitem__", [](PrimitiveBlock& pb, int i, ElementPtr e) { if (i<0) { i+= pb.size(); } pb.at(i)=e; })
        .def("add", [](PrimitiveBlock& pb, ElementPtr e) { pb.add(e); })
        .def("sort", [](PrimitiveBlock& pb) { std::sort(pb.Objects().begin(),pb.Objects().end(),element_cmp); })
        .def_property("FileProgress", &PrimitiveBlock::FileProgress, &PrimitiveBlock::SetFileProgress)
        .def_property_readonly("FilePosition", &PrimitiveBlock::FilePosition)
    ;
    py::class_<Element,std::shared_ptr<Element>>(m, "element")
        .def_property_readonly("InternalId", &Element::InternalId)
        .def_property_readonly("Type", &Element::Type)
        .def_property_readonly("Id", &Element::Id)
        .def_property_readonly("ChangeType", &Element::ChangeType)
        .def_property_readonly("Quadtree", &Element::Quadtree)
        .def_property_readonly("Tags", &Element::Tags)
        .def_property_readonly("Info", &Element::Info)
        .def("SetQuadtree", &Element::SetQuadtree)
        .def("SetChangeType", &Element::SetChangeType)
        .def("SetTags", &Element::SetTags)
        .def("RemoveTag", &Element::RemoveTag)
        .def("AddTag", &Element::AddTag)
        .def("copy", &Element::copy)
        
    ;

    py::class_<ElementInfo>(m, "ElementInfo")

        .def(py::init<int64,int64,int64,int64,std::string,bool>(),py::arg("version"),py::arg("timestamp"),py::arg("changeset"),py::arg("user_id"),py::arg("user"),py::arg("visible"))
        .def_readonly("version", &ElementInfo::version)
        .def_readonly("timestamp", &ElementInfo::timestamp)
        .def_readonly("changeset", &ElementInfo::changeset)
        .def_readonly("user_id", &ElementInfo::user_id)
        .def_readonly("user", &ElementInfo::user)
        .def_readonly("visible", &ElementInfo::visible)
    ;

    py::class_<Member>(m, "Member")
        .def(py::init<ElementType,int64,std::string>(),py::arg("type"),py::arg("ref"), py::arg("role"))
        .def_readonly("type", &Member::type)
        .def_readonly("ref", &Member::ref)
        .def_readonly("role", &Member::role)
    ;

    py::class_<Tag>(m, "Tag")
        .def(py::init<std::string,std::string>(),py::arg("key"), py::arg("val"))
        .def_readonly("key", &Tag::key)
        .def_property_readonly("val", [](const Tag& t) {
            if (!t.val.empty() && (t.val[0]=='\0')) {
                return py::object(py::bytes(t.val));
            }
            return py::object(py::str(t.val));
            
        })
    ;
    
    py::class_<Node, Element, std::shared_ptr<Node>>(m,"Node")
        .def_property_readonly("Lon", &Node::Lon)
        .def_property_readonly("Lat", &Node::Lat)
        //.def("copy", &node::copy)
    ;

    py::class_<Way, Element, std::shared_ptr<Way>>(m,"Way")
        .def_property_readonly("Refs", &Way::Refs)
        //.def("copy", &way::copy)
    ;

    py::class_<Relation, Element, std::shared_ptr<Relation>>(m,"Relation")
        .def_property_readonly("Members", &Relation::Members)
        //.def("copy", &relation::copy)
    ;

    m.def("make_node", &make_node, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("lon"), py::arg("lat"), py::arg("quadtree"), py::arg("changetype"));
    m.def("make_way", &make_way, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("refs"), py::arg("quadtree"), py::arg("changetype"));
    m.def("make_relation", &make_relation, py::arg("id"), py::arg("info"), py::arg("tags"), py::arg("members"), py::arg("quadtree"), py::arg("changetype"));
    
    
    
    py::class_<minimal::Block, std::shared_ptr<minimal::Block>>(m, "MinimalBlock")
        .def_readonly("index", &minimal::Block::index)
        .def_readonly("quadtree", &minimal::Block::quadtree)
        
        .def("nodes_len", [](const minimal::Block& mb) { return mb.nodes.size(); })
        .def("nodes_getitem", [](const minimal::Block& mb, int i) { if (i<0) { i+=mb.nodes.size(); } return mb.nodes.at(i); })
        .def("ways_len", [](const minimal::Block& mb) { return mb.ways.size(); })
        .def("ways_getitem", [](const minimal::Block& mb, int i) { if (i<0) { i+=mb.ways.size(); } return mb.ways.at(i); })
        .def("relations_len", [](const minimal::Block& mb) { return mb.relations.size(); })
        .def("relations_getitem", [](const minimal::Block& mb, int i) { if (i<0) { i+=mb.relations.size(); } return mb.relations.at(i); })
        .def("geometries_len", [](const minimal::Block& mb) { return mb.geometries.size(); })
        .def("geometries_getitem", [](const minimal::Block& mb, int i) { if (i<0) { i+=mb.geometries.size(); } return mb.geometries.at(i); })
        .def_readonly("file_progress", &minimal::Block::file_progress)
        .def_readonly("file_position", &minimal::Block::file_position)

    ;
    py::class_<minimal::Node>(m, "MinimalNode")
        .def_property_readonly("id", [](const minimal::Node& mn) { return mn.id; })
        .def_property_readonly("timestamp", [](const minimal::Node& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimal::Node& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimal::Node& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimal::Node::quadtree)
        .def_readonly("lon", &minimal::Node::lon)
        .def_readonly("lat", &minimal::Node::lat)
        .def("sizeof", [](const minimal::Node& m ) { return sizeof(m); })
    ;

    py::class_<minimal::Way>(m, "MinimalWay")
        .def_property_readonly("id", [](const minimal::Way& mn) { return mn.id; })
        .def_property_readonly("timestamp", [](const minimal::Way& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimal::Way& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimal::Way& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimal::Way::quadtree)
        .def_property_readonly("refs_data", [](const minimal::Way& w)->py::bytes { return py::bytes(w.refs_data); })
    ;

    py::class_<minimal::Relation>(m, "MinimalRelation")
        .def_property_readonly("id", [](const minimal::Relation& mn) { return mn.id; })
        .def_property_readonly("timestamp", [](const minimal::Relation& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimal::Relation& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimal::Relation& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimal::Relation::quadtree)
        .def_property_readonly("tys_data", [](const minimal::Relation& r)->py::bytes { return py::bytes(r.tys_data); })
        .def_property_readonly("refs_data", [](const minimal::Relation& r)->py::bytes { return py::bytes(r.refs_data); })
    ;
    
    py::class_<minimal::Geometry>(m, "MinimalGeometry")
        .def_property_readonly("ty", [](const minimal::Geometry& mn) { return mn.ty; })
        .def_property_readonly("id", [](const minimal::Geometry& mn) { return mn.id; })
        .def_property_readonly("timestamp", [](const minimal::Geometry& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimal::Geometry& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimal::Geometry& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimal::Geometry::quadtree)
    ;
    
    py::class_<Header, HeaderPtr>(m, "Header")
        .def_property("Writer", &Header::Writer, &Header::SetWriter)
        .def_property("Features", &Header::Features,
            [](Header& h, std::vector<std::string>& f) {
                h.Features().clear();
                std::copy(f.begin(),f.end(),std::back_inserter(h.Features()));
            })
        .def_property("Box", &Header::BBox, &Header::SetBBox)
        .def_property("Index", &Header::Index,
            [](Header& h, const block_index& f) {
                h.Index().clear();
                std::copy(f.begin(),f.end(),std::back_inserter(h.Index()));
            })
    ;
    
    
    m.def("combine_primitiveblock_two", &combine_primitiveblock_two);
    m.def("combine_primitiveblock_many", &combine_primitiveblock_many);
    
    m.def("combine_minimalblock_two", &combine_minimalblock_two);
    m.def("combine_minimalblock_many", &combine_minimalblock_many);
    
}


#ifdef INDIVIDUAL_MODULES
PYBIND11_MODULE(_elements, m) {
    //py::module m("_elements", "pybind11 example plugin");
    elements_defs(m);
    //return m.ptr();
}
#endif
