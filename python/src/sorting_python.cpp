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
#include "oqt/sorting/qttree.hpp"
#include "oqt/sorting/qttreegroups.hpp"
#include "oqt/sorting/sortblocks.hpp"
#include "oqt/sorting/mergechanges.hpp"
using namespace oqt;



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
    std::string tempfn, size_t blocksplit,bool fixstrs, bool seperate_filelocs) {

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
    
    int r = run_sortblocks(origfn,qtsfn,outfn,timestamp,numchan, tree,tempfn,blocksplit, fixstrs,seperate_filelocs);
    Logger::Get().timing_messages();
    return r;
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


PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void sorting_defs(py::module& m) {
    

    

    m.def("sortblocks", &run_sortblocks_py, "sort into quadtree blocks",
        py::arg("origfn"),
        py::arg("groups"),
        py::arg("qtsfn") = "",
        py::arg("outfn") = "",
        py::arg("timestamp") = 0,
        py::arg("numchan") = 4,
        py::arg("tempfn") = "",
        py::arg("blocksplit")=500,
        py::arg("fixstrs")=false,
        py::arg("seperate_filelocs")=true
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
        .def("__len__", &QtTree::size)
    ;
    m.def("make_tree_empty",&make_tree_empty);
    m.def("make_qts_tree_maxlevel", &make_qts_tree_maxlevel_py, py::arg("filename"), py::arg("numchan")=4, py::arg("maxlevel")=17);
    m.def("tree_rollup", &tree_rollup_py, py::arg("tree"), py::arg("minsize"));
    m.def("find_groups_copy", &find_groups_copy_py, py::arg("tree"), py::arg("targetsize"), py::arg("minsize"));
    m.def("tree_round_copy", &tree_round_copy_py, py::arg("tree"), py::arg("minsize"));

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
    
    
}


#ifdef INDIVIDUAL_MODULES
PYBIND11_MODULE(_sorting, m) {
    //py::module m("_sorting", "pybind11 example plugin");
    sorting_defs(m);
    //return m.ptr();
}
#endif
