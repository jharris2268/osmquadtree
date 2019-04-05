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

#include "oqt/calcqts/calcqts.hpp"
#include "oqt/calcqts/calcqtsinmem.hpp"
#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/calcqts/qtstore.hpp"
#include "oqt/calcqts/qtstoresplit.hpp"
#include "oqt/calcqts/waynodes.hpp"
#include "oqt/calcqts/waynodesfile.hpp"
#include "oqt/calcqts/writeqts.hpp"
#include "oqt/calcqts/writewaynodes.hpp"

using namespace oqt;


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

PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void calcqts_defs(py::module& m) {
    
    py::class_<QtStore, std::shared_ptr<QtStore>>(m,"QtStore")
        .def("__getitem__", &QtStore::at)
        .def("expand",  &QtStore::expand)
        .def("__contains__", &QtStore::contains)
        .def("__len__",&QtStore::size)
        .def("first", &QtStore::first)
        .def("next", &QtStore::next)
    ;
    m.def("make_qtstore", &make_qtstore_map);
    
    
    py::class_<QtStoreSplit, std::shared_ptr<QtStoreSplit>, QtStore>(m,"QtStoreSplit")
        .def("split_at", &QtStoreSplit::split_at)
        .def("num_tiles", &QtStoreSplit::num_tiles)
        .def("last_tile", &QtStoreSplit::last_tile)
        .def("use_arr", &QtStoreSplit::use_arr)
        .def("tile", &QtStoreSplit::tile)
    ;
    m.def("make_qtstore_split", &make_qtstore_split);
    
    
    
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
    
}


#ifdef INDIVIDUAL_MODULES
PYBIND11_PLUGIN(_calcqts) {
    py::module m("_calcqts", "pybind11 example plugin");
    calcqts_defs(m);
    return m.ptr();
}
#endif
