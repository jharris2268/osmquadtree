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
#include "oqt/pbfformat/objsidset.hpp"

using namespace oqt;

template <class BlockType>
size_t read_blocks_merge_py(
    std::vector<std::string> filenames,
    std::function<bool(std::vector<std::shared_ptr<BlockType>>)> callback,
    src_locs_map locs, size_t numchan, size_t numblocks,
    IdSetPtr filter, size_t objflags,
    size_t buffer) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<BlockType>>(wrap_callback(callback),numblocks);
    std::function<void(std::shared_ptr<BlockType>)> cbf = [cb](std::shared_ptr<BlockType> bl) { cb->call(bl); };
    read_blocks_merge(filenames, cbf, locs, numchan, filter, objflags,buffer);
    return cb->total();
}




size_t read_blocks_primitiveblock_py(
    const std::string& filename,
    std::function<bool(std::vector<PrimitiveBlockPtr>)> callback,
    std::vector<int64> locs, size_t numchan, size_t numblocks,
    IdSetPtr filter, bool ischange, size_t objflags) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    primitiveblock_callback cbf = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };
    read_blocks_primitiveblock(filename, cbf, locs, numchan, filter, ischange, objflags);
    return cb->total();
}


size_t read_blocks_minimalblock_py(
    const std::string& filename,
    std::function<bool(std::vector<minimalblock_ptr>)> callback,
    std::vector<int64> locs, size_t numchan, size_t numblocks,
    size_t objflags) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<minimalblock>>(wrap_callback(callback),numblocks);
    minimalblock_callback cbf = [cb](minimalblock_ptr bl) { cb->call(bl); };
    read_blocks_minimalblock(filename, cbf, locs, numchan, objflags);
    return cb->total();
}


size_t read_blocks_caller_read_primitive(
    std::shared_ptr<ReadBlocksCaller> rbc,
    std::function<bool(std::vector<PrimitiveBlockPtr>)> callback,
    size_t numchan,
    size_t numblocks,
    IdSetPtr filter) {
        
    
    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    auto cbf = multi_threaded_callback<PrimitiveBlock>::make([cb](PrimitiveBlockPtr bl) { cb->call(bl); },numchan);
    rbc->read_primitive(cbf, filter);
    return cb->total();
    
    
}

size_t read_blocks_caller_read_minimal(
    std::shared_ptr<ReadBlocksCaller> rbc,
    std::function<bool(std::vector<minimalblock_ptr>)> callback,
    size_t numchan,
    size_t numblocks,
    IdSetPtr filter) {
        
    
    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<minimalblock>>(wrap_callback(callback),numblocks);
    auto cbf = multi_threaded_callback<minimalblock>::make([cb](minimalblock_ptr bl) { cb->call(bl); },numchan);
    
    rbc->read_minimal(cbf, filter);
    return cb->total();
    
    
}


ElementPtr make_node(int64 id, info inf, tagvector tags, int64 lon, int64 lat, int64 qt, changetype ct) {
    return std::make_shared<Node>(ct, id, qt, inf, tags, lon, lat);
}

ElementPtr make_way(int64 id, info inf, tagvector tags, std::vector<int64> refs, int64 qt, changetype ct) {
    return std::make_shared<Way>(ct, id, qt, inf, tags,refs);
}

ElementPtr make_relation(int64 id, info inf, tagvector tags, std::vector<member> mems, int64 qt, changetype ct) {
    return std::make_shared<Relation>(ct, id, qt, inf, tags, mems);
}

PrimitiveBlockPtr readPrimitiveBlock_py(int64 idx, py::bytes data, bool change) {
    return readPrimitiveBlock(idx,data,change,15,nullptr,geometry::readGeometry);
}

class IdSetInvert : public IdSet {
    public:
        IdSetInvert(IdSetPtr ids_) : ids(ids_) {}
        virtual ~IdSetInvert() {}
        
        virtual bool contains(ElementType t, int64 i) const {
            return !ids->contains(t,i);
        }
    private:
        IdSetPtr ids;
};


typedef std::tuple<int64,int64,int64,int64,int64,int64> tempobjs_tup;

void read_blocks_tempobjs(std::string fn, std::function<bool(py::list/*std::vector<std::shared_ptr<tempobjs_tup>>*/)> cb_in, size_t numchan) {
    
    py::gil_scoped_release r;
    auto fix_callback = [cb_in](std::vector<std::shared_ptr<tempobjs_tup>> p) {
        py::gil_scoped_acquire aq;
        py::list rs;
        for (auto& q: p) {
            if (!q) {
                rs.append(py::object());
            } else {
                rs.append(py::make_tuple(std::get<0>(*q),std::get<1>(*q),std::get<2>(*q),std::get<3>(*q),std::get<4>(*q),std::get<5>(*q)));
            }
        }
        
        return cb_in(rs);
    };
    
    auto cb = std::make_shared<collect_blocks<tempobjs_tup>>(fix_callback/*wrap_callback(cb_in)*/,256);
    std::function<void(std::shared_ptr<tempobjs_tup>)> cbf = [cb](std::shared_ptr<tempobjs_tup> bl) { cb->call(bl); };
    
    auto convblock = [](std::shared_ptr<FileBlock> fb) {
        if (!fb) {
            
            return std::shared_ptr<tempobjs_tup>();
        }
        if (fb->blocktype!="TempObjs") {
            std::cout << "??? node a tempobjs @ " << fb->idx << " " << fb->file_position << std::endl;
            return std::make_shared<tempobjs_tup>(fb->idx,fb->file_position,-1,-1,-1,-1);
        }
        //std::cout << "\r" << fb->idx << " " << fb->filepos << std::flush;
        auto dd = fb->get_data();
        //std::cout << " unc " << dd.size() << std::flush;
        auto mm = readAllPbfTags(dd);
        //std::cout << " with " << mm.size() << " tgs" << std::flush;
        size_t k=0;
        if (!mm.empty()) {
            k=mm.front().value;
        }
        //std::cout << " " << k << " " << mm.size()-1 << std::flush;
        size_t first=0; size_t last=0;
        if (mm.size()>1) {
            auto it=mm.begin(); it++;
            size_t p=0;
            first = readPbfTag(it->data,p).value;
        }
        if (mm.size()>2) {
            size_t p=0;
            last = readPbfTag(mm.back().data,p).value;
        }
        //std::cout << " " << first << " " << last << std::flush;
        return std::make_shared<tempobjs_tup>(fb->idx,fb->file_position,k,mm.size()-1,first,last);
    };
    
    read_blocks_convfunc<tempobjs_tup>(fn, cbf, {}, numchan, convblock);
    //read_tempobjs_file(cb, fn, {}, numchan);
};
    
    


PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void block_defs(py::module& m) {
    m.def("quadtree_string", &quadtree::string);
    m.def("quadtree_from_string", &quadtree::from_string);

    m.def("quadtree_from_tuple", &quadtree::from_tuple);
    m.def("quadtree_tuple", &quadtree::tuple/*[](int64 q) {
        auto r = quadtree::tuple(q);
        return py::make_tuple(r.x,r.y,r.z);
    }*/);

    m.def("quadtree_common", &quadtree::common);
    m.def("quadtree_round", &quadtree::round);

    m.def("quadtree_bbox", &quadtree::bbox);
    m.def("quadtree_calculate", &quadtree::calculate);


    py::enum_<changetype>(m, "changetype")
        .value("Normal", changetype::Normal)
        .value("Delete", changetype::Delete)
        .value("Remove", changetype::Remove)
        .value("Unchanged", changetype::Unchanged)
        .value("Modify", changetype::Modify)
        .value("Create", changetype::Create)
        .export_values();
    py::enum_<ElementType>(m, "ElementType")
        .value("ElementTypeNode", ElementType::Node)
        .value("ElementTypeWay", ElementType::Way)
        .value("ElementTypeRelation", ElementType::Relation)
        .value("ElementTypePoint", ElementType::Point)
        .value("ElementTypeLinestring", ElementType::Linestring)
        .value("ElementTypeSimplePolygon", ElementType::SimplePolygon)
        .value("ElementTypeComplicatedPolygon", ElementType::ComplicatedPolygon)
        .value("ElementTypeWayWithNodes", ElementType::WayWithNodes)
        .value("ElementTypeUnknown", ElementType::Unknown)
        .export_values();
        

    py::class_<bbox>(m,"bbox")
        .def(py::init<int64,int64,int64,int64>())
        .def_readwrite("minx", &bbox::minx)
        .def_readwrite("miny", &bbox::miny)
        .def_readwrite("maxx", &bbox::maxx)
        .def_readwrite("maxy", &bbox::maxy)
        .def("contains", &contains_point)
        .def("overlaps", &overlaps)
        .def("overlaps_quadtree", &overlaps_quadtree)
        .def("contains_box", &bbox_contains)
        .def("expand", &bbox_expand)
    ;
    m.def("bbox_empty", []() { return bbox{1800000000,900000000,-1800000000,-900000000}; });
    m.def("bbox_planet", []() { return bbox{-1800000000,-900000000,1800000000,900000000}; });

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
        .def_property_readonly("FileProgress", &PrimitiveBlock::FileProgress)
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

    py::class_<info>(m, "info")

        .def(py::init<int64,int64,int64,int64,std::string,bool>(),py::arg("version"),py::arg("timestamp"),py::arg("changeset"),py::arg("user_id"),py::arg("user"),py::arg("visible"))
        .def_readonly("version", &info::version)
        .def_readonly("timestamp", &info::timestamp)
        .def_readonly("changeset", &info::changeset)
        .def_readonly("user_id", &info::user_id)
        .def_readonly("user", &info::user)
        .def_readonly("visible", &info::visible)
    ;

    py::class_<member>(m, "member")
        .def(py::init<ElementType,int64,std::string>(),py::arg("type"),py::arg("ref"), py::arg("role"))
        .def_readonly("type", &member::type)
        .def_readonly("ref", &member::ref)
        .def_readonly("role", &member::role)
    ;

    py::class_<tag>(m, "tag")
        .def(py::init<std::string,std::string>(),py::arg("key"), py::arg("val"))
        .def_readonly("key", &tag::key)
        .def_property_readonly("val", [](const tag& t) {
            if (!t.val.empty() && (t.val[0]=='\0')) {
                //return py::cast(py::bytes(t.val));
                return py::object(py::bytes(t.val));
            }
            //return py::cast(py::str(t.val));
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


    py::class_<IdSet,IdSetPtr>(m, "IdSet")
        .def("contains", &IdSet::contains)
    ;
    
    py::class_<ObjsIdSet, IdSet, std::shared_ptr<ObjsIdSet>>(m, "ObjsIdSet")
        .def(py::init<>())
        .def_property_readonly("nodes", &ObjsIdSet::nodes)
        .def_property_readonly("ways", &ObjsIdSet::ways)
        .def_property_readonly("relations", &ObjsIdSet::relations)
        .def("add_node", &ObjsIdSet::add_node)
        .def("add_way", &ObjsIdSet::add_way)
        .def("add_relation", &ObjsIdSet::add_relation)
        .def("add_all", &ObjsIdSet::add_all)
        .def("add", &ObjsIdSet::add)

    ;
    py::class_<IdSetInvert, IdSet, std::shared_ptr<IdSetInvert>>(m, "IdSetInvert")
        .def(py::init<IdSetPtr>())
    ;

    
    m.def("readPrimitiveBlock", &readPrimitiveBlock_py, py::arg("index"), py::arg("data"), py::arg("change"));

    /*py::class_<packedobj, packedobj_ptr>(m, "packedobj")
        .def_property_readonly("InternalId", &packedobj::InternalId)
        .def_property_readonly("Type", &packedobj::Type)
        .def_property_readonly("Id", &packedobj::Id)
        .def_property_readonly("Quadtree", &packedobj::Quadtree)
        .def_property_readonly("ChangeType", &packedobj::ChangeType)
        .def("SetQuadtree", &packedobj::SetQuadtree)
        .def("SetChangeType", &packedobj::SetChangeType)
        
        .def_property_readonly("Data", [](const packedobj& p) -> py::bytes {
            return py::bytes(p.Data());
        })
        .def_property_readonly("Messages", [](const packedobj& p) -> std::list<PbfTag> {
            return readAllPbfTags(p.Data());
        })
        .def("pack", [](const packedobj& p) -> py::bytes {
            return py::bytes(p.pack());
        })
        .def("unpack", [](const packedobj& p) {
            if ((int) p.Type()<3) {
                return unpack_packedobj(p.Type(),p.Id(),p.ChangeType(),p.Quadtree(),p.Data());
            } else {
                return geometry::unpack_geometry(p.Type(),p.Id(),p.ChangeType(),p.Quadtree(),p.Data());
            }
        });
    ;
    m.def("read_packedobj", &read_packedobj);*/

    py::class_<minimalblock, std::shared_ptr<minimalblock>>(m, "minimalblock")
        .def_readonly("index", &minimalblock::index)
        .def_readonly("quadtree", &minimalblock::quadtree)
        //.def_readonly("nodes", &minimalblock::nodes)
        //.def_readonly("ways", &minimalblock::ways)
        //.def_readonly("relations", &minimalblock::relations)
        .def("nodes_len", [](const minimalblock& mb) { return mb.nodes.size(); })
        .def("nodes_getitem", [](const minimalblock& mb, int i) { if (i<0) { i+=mb.nodes.size(); } return mb.nodes.at(i); })
        .def("ways_len", [](const minimalblock& mb) { return mb.ways.size(); })
        .def("ways_getitem", [](const minimalblock& mb, int i) { if (i<0) { i+=mb.ways.size(); } return mb.ways.at(i); })
        .def("relations_len", [](const minimalblock& mb) { return mb.relations.size(); })
        .def("relations_getitem", [](const minimalblock& mb, int i) { if (i<0) { i+=mb.relations.size(); } return mb.relations.at(i); })
        .def("geometries_len", [](const minimalblock& mb) { return mb.geometries.size(); })
        .def("geometries_getitem", [](const minimalblock& mb, int i) { if (i<0) { i+=mb.geometries.size(); } return mb.geometries.at(i); })
        .def_readonly("file_progress", &minimalblock::file_progress)
        .def_readonly("file_position", &minimalblock::file_position)

    ;
    py::class_<minimalnode>(m, "minimalnode")
        .def_readonly("id", &minimalnode::id)
        .def_property_readonly("timestamp", [](const minimalnode& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimalnode& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimalnode& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimalnode::quadtree)
        .def_readonly("lon", &minimalnode::lon)
        .def_readonly("lat", &minimalnode::lat)
        .def("sizeof", [](const minimalnode&m ) { return sizeof(m); })
    ;

    py::class_<minimalway>(m, "minimalway")
        .def_readonly("id", &minimalway::id)
        //.def_readonly("timestamp", &minimalway::timestamp)
        .def_property_readonly("timestamp", [](const minimalway& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimalway& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimalway& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimalway::quadtree)
        .def_property_readonly("refs_data", [](const minimalway& w)->py::bytes { return py::bytes(w.refs_data); })
    ;

    py::class_<minimalrelation>(m, "minimalrelation")
        .def_readonly("id", &minimalrelation::id)
        //.def_readonly("timestamp", &minimalrelation::timestamp)
        .def_property_readonly("timestamp", [](const minimalrelation& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimalrelation& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimalrelation& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimalrelation::quadtree)
        .def_property_readonly("tys_data", [](const minimalrelation& r)->py::bytes { return py::bytes(r.tys_data); })
        .def_property_readonly("refs_data", [](const minimalrelation& r)->py::bytes { return py::bytes(r.refs_data); })
    ;
    
    py::class_<minimalgeometry>(m, "minimalgeometry")
        .def_property_readonly("ty", [](const minimalgeometry& mn) { return mn.ty; })
        .def_property_readonly("id", [](const minimalgeometry& mn) { return mn.id; })
        //.def_readonly("timestamp", &minimalgeometry::timestamp)
        .def_property_readonly("timestamp", [](const minimalgeometry& mn) { return mn.timestamp; })
        .def_property_readonly("version", [](const minimalgeometry& mn) { return mn.version; })
        .def_property_readonly("changetype", [](const minimalgeometry& mn) { return mn.changetype; })
        .def_readonly("quadtree", &minimalgeometry::quadtree)
    ;

    
    m.def("readMinimalBlock", &readMinimalBlock);

    py::class_<PbfTag>(m,"PbfTag")
        .def_readonly("tag", &PbfTag::tag)
        .def_readonly("value", &PbfTag::value)
        .def_property_readonly("data", [](const PbfTag& t)->py::object {
            if (t.data.empty()) {
                return py::none();
            }
            return py::bytes(t.data);
        })
    ;



    m.def("readAllPbfTags", &readAllPbfTags);
    m.def("readPackedDelta", &readPackedDelta);
    m.def("readPackedInt", &readPackedInt);




    py::class_<header, std::shared_ptr<header>>(m, "header")
        .def_readonly("writer", &header::writer)
        .def_readonly("features", &header::features)
        .def_readonly("box", &header::box)
        .def_readonly("index", &header::index)
    ;
    
    
    py::class_<ReadBlocksCaller, std::shared_ptr<ReadBlocksCaller>>(m, "ReadBlocksCaller")
        .def("num_tiles", &ReadBlocksCaller::num_tiles)
    ;
    
    m.def("read_blocks_caller_read_primitive", &read_blocks_caller_read_primitive);
    m.def("read_blocks_caller_read_minimal", &read_blocks_caller_read_minimal);
    m.def("make_read_blocks_caller", &make_read_blocks_caller);

    m.def("calc_idset_filter", [](std::shared_ptr<ReadBlocksCaller> rbc, bbox bx, lonlatvec llv, size_t nc) {
        py::gil_scoped_release r;
        return calc_idset_filter(rbc,bx,llv,nc);
    });


    m.def("read_blocks_primitive", &read_blocks_primitiveblock_py,
        py::arg("filename"), py::arg("callback"), py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("filter")=nullptr, py::arg("ischange")=false,py::arg("objflags")=7);
    
    m.def("read_blocks_minimal", &read_blocks_minimalblock_py,
        py::arg("filename"), py::arg("callback"), py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("objflags")=7);
   
   m.def("read_blocks_merge_primitive", &read_blocks_merge_py<PrimitiveBlock>,
        py::arg("filenames"), py::arg("callback"), py::arg("locs"),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("filter")=nullptr, py::arg("objflags")=7, py::arg("buffer")=0);
    
   m.def("read_blocks_merge_minimal", &read_blocks_merge_py<minimalblock>,
        py::arg("filenames"), py::arg("callback"), py::arg("locs"),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("filter")=nullptr, py::arg("objflags")=7, py::arg("buffer")=0);
    
   
   m.def("read_blocks_tempobjs", &read_blocks_tempobjs);
}
