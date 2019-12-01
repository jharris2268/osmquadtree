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


#include "oqt/count.hpp"


#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/elements/geometry.hpp"


#include "oqt/pbfformat/readfileblocks.hpp"

#include "oqt/utils/invertedcallback.hpp"
#include "oqt/utils/geometry.hpp"
#include "oqt/utils/logger.hpp"

using namespace oqt;


std::shared_ptr<Count> run_count_py(const std::string& fn, size_t numchan, ReadBlockFlags objflags, bool use_minimal) {

    
    Logger::Get().reset_timing();
    py::gil_scoped_release r;
    auto res = run_count(fn,numchan,true,true,objflags, use_minimal);

    Logger::Get().timing_messages();

    return res;
}


class obj_iter2 {
    public:
        obj_iter2(std::function<PrimitiveBlockPtr(void)> reader_) : reader(reader_), idx(0), ii(0) { curr = reader(); }
        
        std::pair<size_t,ElementPtr> next() {
            while (curr && (ii==curr->size())) {
                curr=reader();
                ii=0;
            }
            if (!curr) {
                return std::make_pair(idx,nullptr);
            }
            
            auto o = curr->at(ii);
            ii++;
            return std::make_pair(idx++, o);
        }
    private:
        std::function<PrimitiveBlockPtr(void)> reader;
        size_t idx;
        size_t ii;
        PrimitiveBlockPtr curr;
};

enum class diffreason {
    Same,
    Object,
    Info,
    Tags,
    LonLat,
    Refs,
    Members,
    Quadtree,
    NoLeft,
    NoRight,
    GeomTags
};


struct objdiff {
    diffreason reason;
    size_t left_idx;
    ElementPtr left;
    
    size_t right_idx;
    ElementPtr right;
};

bool pbftag_equals(const PbfTag& left, const PbfTag& right) {
    if (left.tag!=right.tag) { return false; }
    if (left.value!=right.value) { return false; }
    if (left.data!=right.data) { return false; }
    return true;
}

diffreason compare_element(ElementPtr left, ElementPtr right) {
    if (!left) { return diffreason::NoLeft; }
    if (!right) { return diffreason::NoRight; }
    if (left->InternalId()!=right->InternalId()) { return diffreason::Object; }
    const ElementInfo& li = left->Info();
    const ElementInfo& ri = right->Info();
    if ((li.version!=ri.version) || (li.timestamp!=ri.timestamp) || (li.changeset!=ri.changeset) || (li.user_id != ri.user_id)) {
        return diffreason::Info;
    }
    std::vector<Tag> lt = left->Tags();
    std::vector<Tag> rt = right->Tags();
    if (lt.size()!=rt.size()) { return diffreason::Tags; }
    if (!lt.empty()) {
        std::sort(lt.begin(),lt.end(), [](const Tag& l, const Tag& r) { return l.key<r.key; });
        std::sort(rt.begin(),rt.end(), [](const Tag& l, const Tag& r) { return l.key<r.key; });
        for (size_t i=0; i < lt.size(); i++) {
            if ((lt[i].key!=rt[i].key) || (lt[i].val != rt[i].val)) {
                return diffreason::Tags;
            }
        }
    }
    if (left->Type()==ElementType::Node) {
        auto ln = std::dynamic_pointer_cast<Node>(left);
        auto rn = std::dynamic_pointer_cast<Node>(right);
        if ((!ln) || (!rn)) { throw std::domain_error("??"); }
        if ((ln->Lon()!=rn->Lon()) || (ln->Lat()!=rn->Lat())) {
            return diffreason::LonLat;
        }
    } else if (left->Type()==ElementType::Way) {
        auto lw = std::dynamic_pointer_cast<Way>(left);
        auto rw = std::dynamic_pointer_cast<Way>(right);
        
        if ((!lw) || (!rw)) { throw std::domain_error("??"); }
        
        if (lw->Refs().size()!=rw->Refs().size()) { return diffreason::Refs; }
        for (size_t i=0; i < lw->Refs().size(); i++) {
            if (lw->Refs()[i]!=rw->Refs()[i]) { return diffreason::Refs; }
        }
        
    } else if (left->Type()==ElementType::Relation) {
        auto lrel = std::dynamic_pointer_cast<Relation>(left);
        auto rrel = std::dynamic_pointer_cast<Relation>(right);
        if ((!lrel) || (!rrel)) { throw std::domain_error("??"); }
        auto lr=lrel->Members();
        auto rr=rrel->Members();
        if (lr.size()!=rr.size()) { return diffreason::Members; }
        for (size_t i=0; i < lr.size(); i++) {
            if ((lr[i].type!=rr[i].type) || (lr[i].ref!=rr[i].ref) || (lr[i].role!=rr[i].role)) { return diffreason::Members; }
        }
    } else if ( (left->Type()==ElementType::Point) 
            || (left->Type()==ElementType::Linestring)
            || (left->Type()==ElementType::SimplePolygon)
            || (left->Type()==ElementType::ComplicatedPolygon)) {
                
        auto lg = std::dynamic_pointer_cast<BaseGeometry>(left);
        auto rg = std::dynamic_pointer_cast<BaseGeometry>(right);
        if ((!lg) || (!rg)) { throw std::domain_error("??"); }
        auto lt = lg->pack_extras();
        auto rt = rg->pack_extras();
        
        if (lt.size()!=rt.size()) { return diffreason::GeomTags; }
        auto lt_iter=lt.begin();
        auto rt_iter=rt.begin();
        for ( ; lt_iter!= lt.end(); lt_iter++) {
            if (!pbftag_equals(*lt_iter,*rt_iter)) { return diffreason::GeomTags; }
            rt_iter++;
        }
    }
        
    
    if (left->Quadtree()!=right->Quadtree()) {
        return diffreason::Quadtree;
    }
    
    return diffreason::Same;
}

std::pair<std::map<diffreason,size_t>,std::map<std::string,std::string>> find_difference2(const std::string& left_fn, const std::string& right_fn, size_t numchan, std::function<bool(std::vector<objdiff>)> callback) {
    py::gil_scoped_release rel;
    
    auto callback_wrapped = wrap_callback(callback);
    
    size_t numchan_half = numchan/2;
    if (numchan_half==0) { numchan_half=1; }
            
    auto left_reader  = std::make_shared<inverted_callback<PrimitiveBlock>>([left_fn, numchan_half](primitiveblock_callback cb) { read_blocks_primitiveblock(left_fn,  cb, {}, numchan_half, nullptr, false, ReadBlockFlags::Empty); });
    auto right_reader = std::make_shared<inverted_callback<PrimitiveBlock>>([right_fn,numchan_half](primitiveblock_callback cb) { read_blocks_primitiveblock(right_fn, cb, {}, numchan_half, nullptr, false, ReadBlockFlags::Empty); });
    
    std::vector<objdiff> curr;
    curr.reserve(10000);
    auto left_obj = obj_iter2([left_reader]() { return left_reader->next(); });
    auto right_obj = obj_iter2([right_reader]() { return right_reader->next(); });
    
    auto left = left_obj.next();
    auto right = right_obj.next();
    
    std::cout << "first left=" << left.first << ", " << left.second->Type() << ", " << left.second->Id() << std::endl;
    std::cout << "first right=" << right.first << ", " << right.second->Type() << ", " << right.second->Id() << std::endl;
    
    TimeSingle ts;
    
    uint64 next_obj=0;
    std::map<std::string,std::string> users;
    std::map<diffreason,size_t> tot;
    
    
    while (left.second || right.second) {
        
        if (!left.second || (right.second->InternalId() < left.second->InternalId())) {
            curr.push_back(objdiff{diffreason::Object,0,nullptr,right.first,right.second});
            
            right=right_obj.next();
            tot[diffreason::Object]++;
        } else if (!right.second || (left.second->InternalId() < right.second->InternalId())) {
            curr.push_back(objdiff{diffreason::Object,left.first,left.second,0,nullptr});
            
            left=left_obj.next();
            tot[diffreason::Object]++;
        } else  {
            if (left.second->InternalId()>next_obj) {
                std::cout   << "\r[" << TmStr{ts.since(),7,1} <<"]"
                            << left.second->Type()
                            << " " << std::setw(10) << left.second->Id()
                            << " [" << std::setw(10) << left.first
                            << "//" << std::setw(10)  << right.first
                            << ": " << std::setw(5) << tot[diffreason::Object]
                            << " " << std::setw(5) << tot[diffreason::Info]
                            << " " << std::setw(5) << tot[diffreason::Tags]
                            << " " << std::setw(5) << tot[diffreason::LonLat]
                            << " " << std::setw(5) << tot[diffreason::Refs]
                            << " " << std::setw(5) << tot[diffreason::Members]
                            << ", found " << std::setw(6) << users.size() << " changed users]" << std::flush;
                if (left.second->Type()==ElementType::Node) {
                    next_obj = left.second->InternalId() + (1ll<<20);
                } else {
                    next_obj = left.second->InternalId() + (1ll<<17);
                }
            }
            auto same=compare_element(right.second,left.second);
            if (left.second->Info().user != right.second->Info().user) {
                users[left.second->Info().user]=right.second->Info().user;
            }
            
            if (same!=diffreason::Same) {
                curr.push_back(objdiff{same,left.first,left.second,right.first,right.second});
                
            }
            tot[same]++;
            right=right_obj.next();
            left=left_obj.next();
            
        }
        if (curr.size()==10000) {
            if (!callback_wrapped(curr)) {
                left_reader->cancel();
                right_reader->cancel();
                return std::make_pair(tot,users);
            }
            curr.clear();
        }
    }
    std::cout << " done: " << TmStr{ts.since(),1,7} << std::endl;
    if (!curr.empty()) {
        callback_wrapped(curr);
    }
    return std::make_pair(tot,users);
};


      
            
bool has_weird_string(ElementPtr ele) {
    for (const auto& t: ele->Tags()) {
        if (t.key.find(127)!=std::string::npos) {
            return true;
        }
        if (t.val.find(127)!=std::string::npos) {
            return true;
        }
    }
    if (ele->Type()==ElementType::Relation) {
        auto rel = std::dynamic_pointer_cast<Relation>(ele);
        for (const auto& m: rel->Members()) {
            if (!m.role.empty()) {
                if (m.role.find(127)!=std::string::npos) {
                    return true;
                }
            }
        }
    }
    return false;
}
        
std::vector<std::pair<int64,ElementPtr>> filter_weird(std::vector<PrimitiveBlockPtr> bls, bool block_qts) {
    std::vector<std::pair<int64,ElementPtr>> res;
    for (auto bl: bls) {
        for (auto ele: bl->Objects()) {
            if (has_weird_string(ele)) {
                res.push_back(
                    std::make_pair(
                        block_qts ? bl->Quadtree() : ((int64)bl->Index()),
                        ele));
            }
        }
    }
    return res;
}
    
            
    

PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void count_defs(py::module& m) {
    py::class_<CountElement>(m,"CountElement")
        .def_readonly("min_id", &CountElement::min_id)
        .def_readonly("max_id", &CountElement::max_id)
        .def_readonly("num_objects", &CountElement::num_objects)
        .def_readonly("min_timestamp", &CountElement::min_timestamp)
        .def_readonly("max_timestamp", &CountElement::max_timestamp)
    ;
    
    py::class_<CountNode, CountElement>(m,"CountNode")
        .def_readonly("min_lon", &CountNode::min_lon)
        .def_readonly("max_lon", &CountNode::max_lon)
        .def_readonly("min_lat", &CountNode::min_lat)
        .def_readonly("max_lat", &CountNode::max_lat)
    ;
    
    py::class_<CountWay, CountElement>(m,"CountWay")
        .def_readonly("num_refs", &CountWay::min_id)
        .def_readonly("min_ref", &CountWay::min_ref)
        .def_readonly("max_ref", &CountWay::max_ref)
        .def_readonly("max_num_refs", &CountWay::max_num_refs)
    ;
    
    py::class_<CountRelation, CountElement>(m,"CountRelation")
        .def_readonly("num_nodes", &CountRelation::num_nodes)
        .def_readonly("num_ways", &CountRelation::num_ways)
        .def_readonly("num_rels", &CountRelation::num_rels)
        .def_readonly("num_empties", &CountRelation::num_empties)
        .def_readonly("max_len", &CountRelation::max_len)
    ;
    
   py::class_<BlockSummary>(m,"BlockSummary")
       .def_readonly("idx", &BlockSummary::idx)
       .def_readonly("quadtree", &BlockSummary::quadtree)
       .def_readonly("file_position", &BlockSummary::file_position)
       .def_readonly("num_nodes", &BlockSummary::num_nodes)
       .def_readonly("num_ways", &BlockSummary::num_ways)
       .def_readonly("num_relations", &BlockSummary::num_relations)
       .def_readonly("num_geometries", &BlockSummary::num_geometries)
    ;
    

    py::class_<Count, std::shared_ptr<Count>>(m, "Count")
        .def(py::init<int,bool,bool,bool>(),py::arg("index"),py::arg("tiles"), py::arg("geom"), py::arg("change"))
        .def("nodes", &Count::nodes)
        .def("ways", &Count::ways)
        .def("relations", &Count::relations)
        .def("geometries", &Count::geometries)
        .def("blocks_at", [](const Count& cc, size_t i) { return cc.blocks().at(i); })
        .def("blocks_size", [](const Count& cc) { return cc.blocks().size(); })
        .def("summary", &Count::summary)
        .def("short_str", &Count::short_str)
        .def("long_str", &Count::long_str)
        
        .def("add", [](Count& cnt, size_t i, minimal::BlockPtr block) { cnt.add(i,block); })
        .def("add", [](Count& cnt, size_t i, PrimitiveBlockPtr block) { cnt.add(i,block); })
    ;

    m.def("run_count", &run_count_py, "count pbf file contents",
         py::arg("fn"),
         py::arg("numchan")=4,
         py::arg("objflags")=ReadBlockFlags::Empty,
         py::arg("use_minimal")=true
    );

    
 
    py::enum_<diffreason>(m, "diffreason")
        .value("Same", diffreason::Same)
        .value("Object", diffreason::Object)
        .value("Info", diffreason::Info)
        .value("Tags", diffreason::Tags)
        .value("LonLat", diffreason::LonLat)
        .value("Refs", diffreason::Refs)
        .value("Members", diffreason::Members)
        .value("Quadtree", diffreason::Quadtree)
        .value("NoLeft", diffreason::NoLeft)
        .value("NoRight", diffreason::NoRight)
        .value("GeomTags", diffreason::GeomTags)
    ;
    py::class_<objdiff>(m, "objdiff")
        .def_readonly("reason", &objdiff::reason)
        .def_readonly("left_idx", &objdiff::left_idx)
        .def_readonly("left", &objdiff::left)
        .def_readonly("right_idx", &objdiff::right_idx)
        .def_readonly("right", &objdiff::right)
    ;
    //m.def("find_difference", &find_difference);
    m.def("find_difference2", &find_difference2);
    m.def("compare_element", &compare_element);
    
    m.def("fix_str", [](const std::string& s) { return py::bytes(fix_str(s)); });
    
    
    
    
    
    
   
   
   m.def("has_weird_string", &has_weird_string);
   m.def("filter_weird", &filter_weird);
   
}
#ifdef INDIVIDUAL_MODULES
PYBIND11_PLUGIN(_count) {
    py::module m("_count", "pybind11 example plugin");
    count_defs(m);
    return m.ptr();
}
#endif
