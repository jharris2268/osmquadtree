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
#include "oqt/calcqts/waynodes.hpp"
#include "oqt/calcqts/writewaynodes.hpp"
#include "oqt/calcqts/waynodesfile.hpp"
#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/calcqts/qtstore.hpp"
#include "oqt/calcqts/qtstoresplit.hpp"

#include "oqt/utils/invertedcallback.hpp"

using namespace oqt;
class logger_python : public Logger {
    public:
        //using logger::logger;
        logger_python() {};

        virtual void message(const std::string& msg) {
            py::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(
                void,
                Logger,
                message,
                msg);
        }

        virtual void progress(double p, const std::string& msg) {
            py::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(
                void,
                Logger,
                progress,
                p,
                msg);
        }
        virtual ~logger_python() {}
};




void run_calcqts_py(std::string origfn, std::string qtsfn, size_t numchan, bool splitways, bool resort, double buffer, size_t max_depth, bool use_48bit_quadtree) {


     if (qtsfn=="") {
        qtsfn = origfn.substr(0,origfn.size()-4)+"-qts.pbf";
    }
    //auto lg = get_logger(lg_in);
    Logger::Get().reset_timing();
    py::gil_scoped_release release;
    run_calcqts(origfn, qtsfn, numchan, splitways, resort, buffer, max_depth, use_48bit_quadtree);
    Logger::Get().timing_messages();

}
/*
std::shared_ptr<QtTree> run_findgroups_py(const std::string& qtsfn, size_t numchan,
    bool rollup, int64 targetsize, int64 minsize, std::shared_ptr<logger> lg) {

    lg->reset_timing();
    py::gil_scoped_release release;
    auto tree = run_findgroups(qtsfn, numchan, rollup, targetsize,minsize,lg);
    lg->timing_messages();
    return tree;
}*/

std::shared_ptr<QtTree> find_groups_copy_py(std::shared_ptr<QtTree> tree, 
        int64 target, int64 minsize) {
    if (!tree) { throw std::domain_error("no tree!"); }
    py::gil_scoped_release release;
    return find_groups_copy(tree, target, minsize);
}
    

std::shared_ptr<QtTree> make_qts_tree_maxlevel_py(const std::string& qtsfn, size_t numchan, size_t maxlevel) {
    py::gil_scoped_release release;
    return make_qts_tree_maxlevel(qtsfn, numchan, maxlevel);
}

void tree_rollup_py(std::shared_ptr<QtTree> tree, int64 minsize) {
    if (!tree) { throw std::domain_error("no tree!"); }
    py::gil_scoped_release release;
    tree_rollup(tree, minsize);
}

std::shared_ptr<QtTree> tree_round_copy_py(std::shared_ptr<QtTree> tree, int64 minsize) {
    if (!tree) { throw std::domain_error("no tree!"); }
    py::gil_scoped_release release;
    return tree_round_copy(tree, minsize);
}


    

int run_sortblocks_py(
    std::string origfn, std::shared_ptr<QtTree> tree,
    std::string qtsfn, std::string outfn,
    int64 timestamp, size_t numchan,
    std::string tempfn, size_t blocksplit,bool fixstrs) {

    if (!tree) { throw std::domain_error("no tree!"); }
    if (qtsfn=="") {
        qtsfn = origfn.substr(0,origfn.size()-4)+"-qts.pbf";
    }
    if (outfn=="") {
        outfn = origfn.substr(0,origfn.size()-4)+"-sorted.pbf";
    }

    if (tempfn=="") {
        tempfn = outfn+"-temp";
    }
    
    
    Logger::Get().reset_timing();
    py::gil_scoped_release release;
    
    int r = run_sortblocks(origfn,qtsfn,outfn,timestamp,numchan, tree,tempfn,blocksplit, fixstrs);
    Logger::Get().timing_messages();
    return r;
}

std::tuple<std::shared_ptr<WayNodesFile>,std::shared_ptr<CalculateRelations>,std::string,std::vector<int64>>
    run_write_waynodes_py(const std::string& origfn, const std::string& waynodes_fn, size_t numchan, bool sortinmem) {
    
    py::gil_scoped_release release;
    return write_waynodes(origfn, waynodes_fn, numchan, sortinmem); 
}


void find_way_quadtrees_py(
    const std::string& source_filename,
    const std::vector<int64>& source_locs, 
    size_t numchan,
    std::shared_ptr<QtStoreSplit> way_qts,
    std::shared_ptr<WayNodesFile> wns,
    double buffer, size_t max_depth,int64 minway, int64 maxway,
    bool use_48bit_quadtrees) {
    
    
    py::gil_scoped_release release;
    find_way_quadtrees(source_filename, source_locs, numchan, way_qts, wns, buffer, max_depth, minway, maxway,use_48bit_quadtrees);
}
void write_qts_file_py(const std::string& qtsfn, const std::string& nodes_fn, size_t numchan,
    const std::vector<int64>& node_locs, std::shared_ptr<QtStoreSplit> way_qts,
    std::shared_ptr<WayNodesFile> wns, std::shared_ptr<CalculateRelations> rels, double buffer, size_t max_depth) {
    
    py::gil_scoped_release release;
    write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth);
}

void run_mergechanges_py(std::string origfn, std::string outfn, size_t numchan, bool sort_objs, bool filter_objs, py::object filter_box_in, std::vector<LonLat> poly, int64 enddate, std::string tempfn, size_t grptiles, bool sortfile, bool inmem) {
    if (outfn=="") {
        outfn = origfn.substr(0,origfn.size()-4)+"-merged.pbf";
    }
    
    bbox filter_box{-1800000000, -900000000, 1800000000, 900000000};
    if (filter_box_in.is(py::none())) {
        try {
            filter_box = py::cast<bbox>(filter_box_in);
        } catch (py::cast_error& c) {
            auto filter_box_list = filter_box_in.cast<py::list>();
            filter_objs=true;
            filter_box.minx = py::cast<int64>(filter_box_list[0]);
            filter_box.maxx = py::cast<int64>(filter_box_list[1]);
            filter_box.miny = py::cast<int64>(filter_box_list[2]);
            filter_box.maxy = py::cast<int64>(filter_box_list[3]);
        }
        std::cout << "filter_box=" << filter_box << std::endl;
        filter_objs=true;
    }


    Logger::Get().reset_timing();
    py::gil_scoped_release release;
    run_mergechanges(origfn, outfn, numchan, sort_objs, filter_objs, filter_box, poly, enddate,tempfn, grptiles,sortfile,inmem);
    Logger::Get().timing_messages();
}


std::shared_ptr<Count> run_count_py(const std::string& fn, size_t numchan, size_t objflags, bool count_full) {

    
    Logger::Get().reset_timing();
    py::gil_scoped_release r;
    auto res = run_count(fn,numchan,true,true,objflags, count_full);

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
    Quadtree
};


struct objdiff {
    diffreason reason;
    size_t left_idx;
    ElementPtr left;
    
    size_t right_idx;
    ElementPtr right;
};



diffreason compare_element(ElementPtr left, ElementPtr right) {
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
        if ((ln->Lon()!=rn->Lon()) || (ln->Lat()!=rn->Lat())) {
            return diffreason::LonLat;
        }
    } else if (left->Type()==ElementType::Way) {
        auto lw = std::dynamic_pointer_cast<Way>(left)->Refs();
        auto rw = std::dynamic_pointer_cast<Way>(right)->Refs();
        if (lw.size()!=rw.size()) { return diffreason::Refs; }
        for (size_t i=0; i < lw.size(); i++) {
            if (lw[i]!=rw[i]) { return diffreason::Refs; }
        }
        
    } else if (left->Type()==ElementType::Relation) {
        auto lr = std::dynamic_pointer_cast<Relation>(left)->Members();
        auto rr = std::dynamic_pointer_cast<Relation>(right)->Members();
        if (lr.size()!=rr.size()) { return diffreason::Members; }
        for (size_t i=0; i < lr.size(); i++) {
            if ((lr[i].type!=rr[i].type) || (lr[i].ref!=rr[i].ref) || (lr[i].role!=rr[i].role)) { return diffreason::Members; }
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
            
    auto left_reader  = std::make_shared<inverted_callback<PrimitiveBlock>>([left_fn, numchan_half](primitiveblock_callback cb) { read_blocks_primitiveblock(left_fn,  cb, {}, numchan_half, nullptr, false, 7); });
    auto right_reader = std::make_shared<inverted_callback<PrimitiveBlock>>([right_fn,numchan_half](primitiveblock_callback cb) { read_blocks_primitiveblock(right_fn, cb, {}, numchan_half, nullptr, false, 7); });
    
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
void core_defs(py::module& m) {
    //py::class_<logger_python, std::shared_ptr<logger_python>> lg(m, "logger");
    
    
    py::class_<Logger,std::shared_ptr<Logger>, logger_python>(m, "Logger")
        .def(py::init<>())
        .def("message", &Logger::message)
        .def("progress", &Logger::progress)
        .def("reset_timing", &Logger::reset_timing)
        .def("timing_messages", &Logger::timing_messages)
    ;
    m.def("get_logger", &Logger::Get);
    m.def("set_logger", &Logger::Set);


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
       .def_readonly("len", &BlockSummary::len)
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
        .def("blocks", &Count::blocks)
        .def("summary", &Count::summary)
        .def("short_str", &Count::short_str)
        .def("long_str", &Count::long_str)
        
        .def("add", [](Count& cnt, size_t i, minimal::BlockPtr block) { cnt.add(i,block); })
        .def("add", [](Count& cnt, size_t i, PrimitiveBlockPtr block) { cnt.add(i,block); })
    ;

    m.def("run_count", &run_count_py, "count pbf file contents",
         py::arg("fn"),
         py::arg("numchan")=4,
         py::arg("objflags")=8,
         py::arg("count_full")=false
    );

    m.def("calcqts", &run_calcqts_py, "calculate quadtrees",
        py::arg("origfn"),
        py::arg("qtsfn")= "",
        py::arg("numchan")= 4,
        py::arg("splitways")=true,
        py::arg("resort") = true,
        py::arg("buffer") = 0.05,
        py::arg("maxdepth") = 18,
        py::arg("use_48bit_quadtree")=false
    );

    m.def("sortblocks", &run_sortblocks_py, "sort into quadtree blocks",
        py::arg("origfn"),
        py::arg("groups"),
        py::arg("qtsfn") = "",
        py::arg("outfn") = "",
        py::arg("timestamp") = 0,
        py::arg("numchan") = 4,
        py::arg("tempfn") = "",
        py::arg("blocksplit")=500,
        py::arg("fixstrs")=false
    );

    

    m.def("mergechanges", &run_mergechanges_py, "merge changes files, optionally filter and sort",
        py::arg("origfn"),
        py::arg("outfn") = "",
        py::arg("numchan") = 4,
        py::arg("sort") = false,
        py::arg("filterobjs") = false,
        py::arg("filter") = py::none(),
        py::arg("poly") = py::none(),
        py::arg("timestamp") = 0,
        py::arg("tempfn") = "",
        py::arg("grptiles") = 500,
        py::arg("sortfile")=true,
        py::arg("inmem")=false

    );

    py::class_<QtTree,std::shared_ptr<QtTree>>(m,"QtTree")
        .def("add", &QtTree::add, py::arg("qt"), py::arg("val"))
        .def("find", &QtTree::find_tile, py::arg("qt"))
        .def("at", &QtTree::at)
    ;
    m.def("make_tree_empty",&make_tree_empty);
    m.def("make_qts_tree_maxlevel", &make_qts_tree_maxlevel_py);
    m.def("tree_rollup", &tree_rollup_py);
    m.def("find_groups_copy", &find_groups_copy_py);
    m.def("tree_round_copy", &tree_round_copy_py);

    py::class_<QtTree::Item>(m,"QtTreeItem")
        .def_readonly("qt", &QtTree::Item::qt)
        .def_readonly("parent", &QtTree::Item::parent)
        .def_readonly("idx", &QtTree::Item::idx)
        .def_readonly("weight", &QtTree::Item::weight)
        .def_readonly("total", &QtTree::Item::total)
        .def("children", [](const QtTree::Item& q, int i) {
            if ((i<0) || (i>3)) { throw std::range_error("children len 4"); };
            return q.children[i];
        });
            //return py::make_tuple(q.children[0],q.children[1],q.children[2],q.children[3]);})
    ;
    
 
    py::enum_<diffreason>(m, "diffreason")
        .value("Same", diffreason::Same)
        .value("Object", diffreason::Object)
        .value("Info", diffreason::Info)
        .value("Tags", diffreason::Tags)
        .value("LonLat", diffreason::LonLat)
        .value("Refs", diffreason::Refs)
        .value("Members", diffreason::Members)
        .value("Quadtree", diffreason::Quadtree)
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
    
    
    py::class_<WayNodesFile, std::shared_ptr<WayNodesFile>>(m,"WayNodesFile");
    py::class_<CalculateRelations, std::shared_ptr<CalculateRelations>>(m,"CalculateRelations")
        .def("str",&CalculateRelations::str)    
    ;
    
    
    m.def("write_waynodes", &run_write_waynodes_py, "write waynodes",
        py::arg("origfn"),
        py::arg("qtsfn")= "",
        py::arg("numchan")= 4,
        py::arg("resort") = true
    );

    m.def("find_way_quadtrees", &find_way_quadtrees_py, "find_way_quadtrees",
        py::arg("source_filename"), py::arg("source_locs"), py::arg("numchan"),
        py::arg("way_qts"), py::arg("wns"), py::arg("buffer"), py::arg("max_depth"),
        py::arg("minway"), py::arg("maxway"),py::arg("use_48bit_quadtrees")
    );
    m.def("write_qts_file", &write_qts_file_py, "write_qts_file", 
        py::arg("qtsfn"), py::arg("nodes_fn"), py::arg("numchan"),
        py::arg("node_locs"), py::arg("way_qts"),
        py::arg("wns"), py::arg("rels"), py::arg("buffer"), py::arg("max_depth")
    );
    
    py::class_<QtStoreSplit, std::shared_ptr<QtStoreSplit>>(m,"QtStoreSplit")
        .def("split_at", &QtStoreSplit::split_at)
        .def("num_tiles", &QtStoreSplit::num_tiles)
        .def("last_tile", &QtStoreSplit::last_tile)
        .def("use_arr", &QtStoreSplit::use_arr)
        .def("tile", &QtStoreSplit::tile)
    ;
    m.def("make_qtstore_split", &make_qtstore_split);
    
    m.def("compress", [](const std::string& s, int l) { return py::bytes(compress(s,l)); }, py::arg("data"), py::arg("level")=-1);
    m.def("decompress", [](const std::string& s, size_t l) { return py::bytes(decompress(s,l)); });
    m.def("compress_gzip", [](const std::string& fn, const std::string& s, int l) { return py::bytes(compress_gzip(fn,s,l)); }, py::arg("filename"), py::arg("data"), py::arg("level")=-1);
    m.def("decompress_gzip", [](const std::string& s) { return py::bytes(decompress_gzip(s)); });
    m.def("decompress_gzip_info", [](const std::string& s) {
        auto ii = gzip_info(s);
        return py::make_tuple(py::str(ii.first), ii.second, py::bytes(decompress_gzip(s)));
    });
   m.def("checkstats", &checkstats);
   
   m.def("has_weird_string", &has_weird_string);
   m.def("filter_weird", &filter_weird);
   m.def("file_size", &file_size);
}
