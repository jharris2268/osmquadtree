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
class logger_python : public logger {
    public:
        //using logger::logger;
        logger_python() {};

        virtual void message(const std::string& msg) {
            py::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(
                void,
                logger,
                message,
                msg);
        }

        virtual void progress(double p, const std::string& msg) {
            py::gil_scoped_acquire acquire;
            PYBIND11_OVERLOAD_PURE(
                void,
                logger,
                progress,
                p,
                msg);
        }
        virtual ~logger_python() {}
};




void run_calcqts_py(std::string origfn, std::string qtsfn, size_t numchan, bool splitways, bool resort, double buffer, size_t max_depth) {


     if (qtsfn=="") {
        qtsfn = origfn.substr(0,origfn.size()-4)+"-qts.pbf";
    }
    //auto lg = get_logger(lg_in);
    get_logger()->reset_timing();
    py::gil_scoped_release release;
    run_calcqts(origfn, qtsfn, numchan, splitways, resort, buffer, max_depth);
    get_logger()->timing_messages();

}
/*
std::shared_ptr<qttree> run_findgroups_py(const std::string& qtsfn, size_t numchan,
    bool rollup, int64 targetsize, int64 minsize, std::shared_ptr<logger> lg) {

    lg->reset_timing();
    py::gil_scoped_release release;
    auto tree = run_findgroups(qtsfn, numchan, rollup, targetsize,minsize,lg);
    lg->timing_messages();
    return tree;
}*/

std::shared_ptr<qttree> find_groups_copy_py(std::shared_ptr<qttree> tree, 
        int64 target, int64 minsize) {
    if (!tree) { throw std::domain_error("no tree!"); }
    py::gil_scoped_release release;
    return find_groups_copy(tree, target, minsize);
}
    

std::shared_ptr<qttree> make_qts_tree_py(const std::string& qtsfn, size_t numchan) {
    py::gil_scoped_release release;
    return make_qts_tree(qtsfn, numchan);
}

void tree_rollup_py(std::shared_ptr<qttree> tree, int64 minsize) {
    if (!tree) { throw std::domain_error("no tree!"); }
    py::gil_scoped_release release;
    tree_rollup(tree, minsize);
}

std::shared_ptr<qttree> tree_round_copy_py(std::shared_ptr<qttree> tree, int64 minsize) {
    if (!tree) { throw std::domain_error("no tree!"); }
    py::gil_scoped_release release;
    return tree_round_copy(tree, minsize);
}


    

int run_sortblocks_py(
    std::string origfn, std::shared_ptr<qttree> tree,
    std::string qtsfn, std::string outfn,
    int64 timestamp, size_t numchan,
    std::string tempfn, size_t blocksplit) {

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
    
    
    get_logger()->reset_timing();
    py::gil_scoped_release release;
    
    int r = run_sortblocks(origfn,qtsfn,outfn,timestamp,numchan, tree,tempfn,blocksplit);
    get_logger()->timing_messages();
    return r;
}

std::tuple<std::shared_ptr<WayNodesFile>,std::shared_ptr<calculate_relations>,std::string,std::vector<int64>>
    run_write_waynodes_py(const std::string& origfn, const std::string& waynodes_fn, size_t numchan, bool sortinmem) {
    
    py::gil_scoped_release release;
    return write_waynodes(origfn, waynodes_fn, numchan, sortinmem, get_logger()); 
}


void find_way_quadtrees_py(
    const std::string& source_filename,
    const std::vector<int64>& source_locs, 
    size_t numchan,
    std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns,
    double buffer, size_t max_depth,int64 minway, int64 maxway) {
    
    
    py::gil_scoped_release release;
    find_way_quadtrees(source_filename, source_locs, numchan, way_qts, wns, buffer, max_depth, minway, maxway);
}
void write_qts_file_py(const std::string& qtsfn, const std::string& nodes_fn, size_t numchan,
    const std::vector<int64>& node_locs, std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns, std::shared_ptr<calculate_relations> rels, double buffer, size_t max_depth) {
    
    py::gil_scoped_release release;
    write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth);
}

void run_mergechanges_py(std::string origfn, std::string outfn, size_t numchan, bool sort_objs, bool filter_objs, py::object filter_box_in, lonlatvec poly, int64 enddate, std::string tempfn, size_t grptiles, bool sortfile, bool inmem) {
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


    get_logger()->reset_timing();
    py::gil_scoped_release release;
    run_mergechanges(origfn, outfn, numchan, sort_objs, filter_objs, filter_box, poly, enddate,tempfn, grptiles,sortfile,inmem);
    get_logger()->timing_messages();
}


py::object count_blocks_call(const std::string& fn, size_t numchan, size_t objflags) {

    
    get_logger()->reset_timing();
    py::gil_scoped_release release;
    auto res = run_count(fn,numchan,true,true,objflags);
    //return py::cast(res.long_str());
    //return res.long_str();
    py::dict res_dict;
    res_dict["index"]=py::cast(res.index);
    res_dict["numblocks"]=py::cast(res.numblocks);
    res_dict["progress"]=py::cast(res.progress);
    res_dict["uncomp"]=py::cast(res.uncomp);
    res_dict["long_str"]=py::cast(res.long_str());

    py::list allnn;
    for (auto nn: res.nn) {
        py::dict nodes;
        nodes["min_id"]=py::cast(nn.min_id);
        nodes["max_id"]=py::cast(nn.max_id);
        nodes["num_objects"]=py::cast(nn.num_objects);
        nodes["min_timestamp"]=py::cast(nn.min_timestamp);
        nodes["max_timestamp"]=py::cast(nn.max_timestamp);
        nodes["minlon"]=py::cast(nn.minlon);
        nodes["minlat"]=py::cast(nn.minlat);
        nodes["maxlon"]=py::cast(nn.maxlon);
        nodes["maxlat"]=py::cast(nn.maxlat);
        allnn.append(nodes);
    }


    py::list allww;
    for (auto ww: res.ww) {
        py::dict ways;
        ways["min_id"]=py::cast(ww.min_id);
        ways["max_id"]=py::cast(ww.max_id);
        ways["num_objects"]=py::cast(ww.num_objects);
        ways["min_timestamp"]=py::cast(ww.min_timestamp);
        ways["max_timestamp"]=py::cast(ww.max_timestamp);
        ways["num_refs"]=py::cast(ww.num_refs);
        ways["min_ref"]=py::cast(ww.min_ref);
        ways["max_ref"]=py::cast(ww.max_ref);
        ways["max_num_refs"]=py::cast(ww.max_num_refs);
        allww.append(ways);
    }

    py::list allrr;
    for (auto rr: res.rr) {
        py::dict relations;
        relations["min_id"]=py::cast(rr.min_id);
        relations["max_id"]=py::cast(rr.max_id);
        relations["num_objects"]=py::cast(rr.num_objects);
        relations["min_timestamp"]=py::cast(rr.min_timestamp);
        relations["max_timestamp"]=py::cast(rr.max_timestamp);
        relations["num_nodes"]=py::cast(rr.num_nodes);
        relations["num_ways"]=py::cast(rr.num_ways);
        relations["num_rels"]=py::cast(rr.num_rels);
        relations["num_empties"]=py::cast(rr.num_empties);
        relations["max_len"]=py::cast(rr.max_len);
        allrr.append(relations);
    }
    
    py::list allgg;
    for (auto gg: res.gg) {
        py::dict geoms;
        geoms["min_id"]=py::cast(gg.min_id);
        geoms["max_id"]=py::cast(gg.max_id);
        geoms["num_objects"]=py::cast(gg.num_objects);
        geoms["min_timestamp"]=py::cast(gg.min_timestamp);
        geoms["max_timestamp"]=py::cast(gg.max_timestamp);
        allgg.append(geoms);
    }
    
    res_dict["nodes"]=allnn;
    res_dict["ways"]=allww;
    res_dict["relations"]=allrr;
    res_dict["geometries"]=allgg;
    
    res_dict["tiles"] = py::cast(res.tiles);

    get_logger()->timing_messages();

    return res_dict;
}

struct objdiff {
    size_t left_idx;
    element_ptr left;
    
    size_t right_idx;
    element_ptr right;
};


class obj_iter2 {
    public:
        obj_iter2(std::function<primitiveblock_ptr(void)> reader_) : reader(reader_), idx(0), ii(0) { curr = reader(); }
        
        std::pair<size_t,element_ptr> next() {
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
        std::function<primitiveblock_ptr(void)> reader;
        size_t idx;
        size_t ii;
        primitiveblock_ptr curr;
};
enum diffreason {
    Same,
    Object,
    Info,
    Tags,
    LonLat,
    Refs,
    Members
};
diffreason compare_element(element_ptr left, element_ptr right) {
    if (left->InternalId()!=right->InternalId()) { return diffreason::Object; }
    const info& li = left->Info();
    const info& ri = right->Info();
    if ((li.version!=ri.version) || (li.timestamp!=ri.timestamp) || (li.changeset!=ri.changeset) || (li.user_id != ri.user_id)) {
        return diffreason::Info;
    }
    tagvector lt = left->Tags();
    tagvector rt = right->Tags();
    if (lt.size()!=rt.size()) { return diffreason::Tags; }
    if (!lt.empty()) {
        std::sort(lt.begin(),lt.end(), [](const tag& l, const tag& r) { return l.key<r.key; });
        std::sort(rt.begin(),rt.end(), [](const tag& l, const tag& r) { return l.key<r.key; });
        for (size_t i=0; i < lt.size(); i++) {
            if ((lt[i].key!=rt[i].key) || (lt[i].val != rt[i].val)) {
                return diffreason::Tags;
            }
        }
    }
    if (left->Type()==Node) {
        auto ln = std::dynamic_pointer_cast<node>(left);
        auto rn = std::dynamic_pointer_cast<node>(right);
        if ((ln->Lon()!=rn->Lon()) || (ln->Lat()!=rn->Lat())) {
            return diffreason::LonLat;
        }
    } else if (left->Type()==Way) {
        auto lw = std::dynamic_pointer_cast<way>(left)->Refs();
        auto rw = std::dynamic_pointer_cast<way>(right)->Refs();
        if (lw.size()!=rw.size()) { return diffreason::Refs; }
        for (size_t i=0; i < lw.size(); i++) {
            if (lw[i]!=rw[i]) { return diffreason::Refs; }
        }
        
    } else if (left->Type()==Relation) {
        auto lr = std::dynamic_pointer_cast<relation>(left)->Members();
        auto rr = std::dynamic_pointer_cast<relation>(right)->Members();
        if (lr.size()!=rr.size()) { return diffreason::Members; }
        for (size_t i=0; i < lr.size(); i++) {
            if ((lr[i].type!=rr[i].type) || (lr[i].ref!=rr[i].ref) || (lr[i].role!=rr[i].role)) { return diffreason::Members; }
        }
    }
    return diffreason::Same;
}
typedef std::function<void(primitiveblock_ptr)> primitiveblock_callback;
std::pair<std::vector<int64>,std::map<std::string,std::string>> find_difference2(const std::string& left_fn, const std::string& right_fn, size_t numchan, std::function<bool(std::vector<objdiff>)> callback) {
    py::gil_scoped_release rel;
    
    auto callback_wrapped = wrap_callback(callback);
    
    size_t numchan_half = numchan/2;
    if (numchan_half==0) { numchan_half=1; }
            
    auto left_reader  = std::make_shared<inverted_callback<primitiveblock>>([left_fn, numchan_half](primitiveblock_callback cb) { read_blocks_primitiveblock(left_fn,  cb, {}, numchan_half, nullptr, false, 7); });
    auto right_reader = std::make_shared<inverted_callback<primitiveblock>>([right_fn,numchan_half](primitiveblock_callback cb) { read_blocks_primitiveblock(right_fn, cb, {}, numchan_half, nullptr, false, 7); });
    
    std::vector<objdiff> curr;
    curr.reserve(10000);
    auto left_obj = obj_iter2([left_reader]() { return left_reader->next(); });
    auto right_obj = obj_iter2([right_reader]() { return right_reader->next(); });
    
    auto left = left_obj.next();
    auto right = right_obj.next();
    time_single ts;
    
    uint64 next_obj=0;
    std::map<std::string,std::string> users;
    std::vector<int64> tot(7);
    
    
    while (left.second || right.second) {
        
        if (!left.second || (right.second->InternalId() < left.second->InternalId())) {
            curr.push_back(objdiff{0,nullptr,right.first,right.second});
            
            right=right_obj.next();
            tot[Object]++;
        } else if (!right.second || (left.second->InternalId() < right.second->InternalId())) {
            curr.push_back(objdiff{left.first,left.second,0,nullptr});
            
            left=left_obj.next();
            tot[Object]++;
        } else  {
            if (left.second->InternalId()>next_obj) {
                std::cout   << "\r[" << TmStr{ts.since(),7,1} <<"]"
                            << left.second->Type()
                            << " " << std::setw(10) << left.second->Id()
                            << " [" << std::setw(10) << left.first
                            << "//" << std::setw(10)  << right.first
                            << ": " << std::setw(5) << tot[1]
                            << " " << std::setw(5) << tot[2]
                            << " " << std::setw(5) << tot[3]
                            << " " << std::setw(5) << tot[4]
                            << " " << std::setw(5) << tot[5]
                            << " " << std::setw(5) << tot[6]
                            << ", found " << std::setw(6) << users.size() << " changed users]" << std::flush;
                if (left.second->Type()==Node) {
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
                curr.push_back(objdiff{left.first,left.second,right.first,right.second});
                
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


PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void core_defs(py::module& m) {
    //py::class_<logger_python, std::shared_ptr<logger_python>> lg(m, "logger");
    
    
    py::class_<logger,std::shared_ptr<logger>, logger_python>(m, "logger")
        .def(py::init<>())
        .def("message", &logger::message)
        .def("progress", &logger::progress)
        .def("reset_timing", &logger::reset_timing)
        .def("timing_messages", &logger::timing_messages)
    ;
    m.def("get_logger", &get_logger);
    m.def("set_logger", &set_logger);


    m.def("count", &count_blocks_call, "count pbf file contents",
         py::arg("fn"),
         py::arg("numchan")=4,
         py::arg("objflags")=7
    );

    m.def("calcqts", &run_calcqts_py, "calculate quadtrees",
        py::arg("origfn"),
        py::arg("qtsfn")= "",
        py::arg("numchan")= 4,
        py::arg("splitways")=true,
        py::arg("resort") = true,
        py::arg("buffer") = 0.05,
        py::arg("maxdepth") = 18
    );

    m.def("sortblocks", &run_sortblocks_py, "sort into quadtree blocks",
        py::arg("origfn"),
        py::arg("groups"),
        py::arg("qtsfn") = "",
        py::arg("outfn") = "",
        py::arg("timestamp") = 0,
        py::arg("numchan") = 4,
        py::arg("tempfn") = "",
        py::arg("blocksplit")=500
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

    py::class_<qttree,std::shared_ptr<qttree>>(m,"qttree")
        //.def("__init__", [](qttree&) { return make_tree_empty(); })
        .def("add", &qttree::add, py::arg("qt"), py::arg("val"))
        .def("find", &qttree::find_tile, py::arg("qt"))
        .def("at", &qttree::at)
    ;
    m.def("make_tree_empty",&make_tree_empty);
    m.def("make_qts_tree", &make_qts_tree_py);
    m.def("tree_rollup", &tree_rollup_py);
    m.def("find_groups_copy", &find_groups_copy_py);
    m.def("tree_round_copy", &tree_round_copy_py);

    py::class_<qttree_item>(m,"qttree_item")
        .def_readonly("qt", &qttree_item::qt)
        .def_readonly("parent", &qttree_item::parent)
        .def_readonly("idx", &qttree_item::idx)
        .def_readonly("weight", &qttree_item::weight)
        .def_readonly("total", &qttree_item::total)
        .def("children", [](const qttree_item& q, int i) {
            if ((i<0) || (i>3)) { throw std::range_error("children len 4"); };
            return q.children[i];
        });
            //return py::make_tuple(q.children[0],q.children[1],q.children[2],q.children[3]);})
    ;
    
    py::class_<count_tile>(m,"count_tile")
        .def_readonly("idx", &count_tile::idx)
        .def_readonly("quadtree", &count_tile::quadtree)
        .def_readonly("len", &count_tile::len)
        .def_readonly("num_nodes", &count_tile::num_nodes)
        .def_readonly("num_ways", &count_tile::num_ways)
        .def_readonly("num_relations", &count_tile::num_relations)
    ;
    
    py::class_<objdiff>(m, "objdiff")
        .def_readonly("left_idx", &objdiff::left_idx)
        .def_readonly("left", &objdiff::left)
        .def_readonly("right_idx", &objdiff::right_idx)
        .def_readonly("right", &objdiff::right)
    ;
    //m.def("find_difference", &find_difference);
    m.def("find_difference2", &find_difference2);
    m.def("fix_str", [](const std::string& s) { return py::bytes(fix_str(s)); });
    
    
    py::class_<WayNodesFile, std::shared_ptr<WayNodesFile>>(m,"WayNodesFile");
    py::class_<calculate_relations, std::shared_ptr<calculate_relations>>(m,"calculate_relations")
        .def("str",&calculate_relations::str)    
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
        py::arg("minway"), py::arg("maxway")
    );
    m.def("write_qts_file", &write_qts_file_py, "write_qts_file", 
        py::arg("qtsfn"), py::arg("nodes_fn"), py::arg("numchan"),
        py::arg("node_locs"), py::arg("way_qts"),
        py::arg("wns"), py::arg("rels"), py::arg("buffer"), py::arg("max_depth")
    );
    
    py::class_<qtstore_split, std::shared_ptr<qtstore_split>>(m,"qtstore_split")
        .def("split_at", &qtstore_split::split_at)
        .def("num_tiles", &qtstore_split::num_tiles)
        .def("last_tile", &qtstore_split::last_tile)
        .def("use_arr", &qtstore_split::use_arr)
        .def("tile", &qtstore_split::tile)
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

}
