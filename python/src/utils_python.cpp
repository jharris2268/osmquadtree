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

#include "oqt/geometry/utils.hpp"

#include "oqt/utils/compress.hpp"
#include "oqt/utils/bbox.hpp"
#include "oqt/utils/date.hpp"
#include "oqt/utils/geometry.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/utils/operatingsystem.hpp"

#include "oqt/utils/pbf/varint.hpp"
#include "oqt/utils/pbf/protobuf.hpp"


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



PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void utils_defs(py::module& m) {
    
    // include/oqt/utils/logger.hpp
    py::class_<Logger,std::shared_ptr<Logger>, logger_python>(m, "Logger")
        .def(py::init<>())
        .def("message", &Logger::message)
        .def("progress", &Logger::progress)
        .def("reset_timing", &Logger::reset_timing)
        .def("timing_messages", &Logger::timing_messages)
    ;
    m.def("get_logger", &Logger::Get);
    m.def("set_logger", &Logger::Set);

    // include/oqt/utils/bbox.hpp
    py::class_<bbox>(m,"bbox")
        .def(py::init<int64,int64,int64,int64>())
        .def_readwrite("minx", &bbox::minx)
        .def_readwrite("miny", &bbox::miny)
        .def_readwrite("maxx", &bbox::maxx)
        .def_readwrite("maxy", &bbox::maxy)
        .def("contains", &contains_point)
        .def("overlaps", &overlaps)
        //.def("overlaps_quadtree", &overlaps_quadtree)
        .def("contains_box", &bbox_contains)
        .def("expand", &bbox_expand)
    ;
    m.def("bbox_empty", []() { return bbox{1800000000,900000000,-1800000000,-900000000}; });
    m.def("bbox_planet", []() { return bbox{-1800000000,-900000000,1800000000,900000000}; });

    // include/oqt/utils/geometry.hpp
    py::class_<LonLat>(m,"LonLat")
        .def(py::init<int64,int64>())
        .def_readonly("lon", &LonLat::lon)
        .def_readonly("lat", &LonLat::lat)
        .def_property_readonly("transform", [](const LonLat& ll) { return geometry::forward_transform(ll.lon,ll.lat); })
    ;
    
    m.def("point_in_poly", &point_in_poly);
    m.def("segment_intersects", &segment_intersects);
    m.def("line_intersects", &line_intersects);
    m.def("line_box_intersects", &line_box_intersects);

    
    // include/oqt/utils/compress.hpp
    m.def("compress", [](const std::string& s, int l) { return py::bytes(compress(s,l)); }, py::arg("data"), py::arg("level")=-1);
    m.def("decompress", [](const std::string& s, size_t l) { return py::bytes(decompress(s,l)); });
    m.def("compress_gzip", [](const std::string& fn, const std::string& s, int l) { return py::bytes(compress_gzip(fn,s,l)); }, py::arg("filename"), py::arg("data"), py::arg("level")=-1);
    m.def("decompress_gzip", [](const std::string& s) { return py::bytes(decompress_gzip(s)); });
    m.def("decompress_gzip_info", [](const std::string& s) {
        auto ii = gzip_info(s);
        return py::make_tuple(py::str(ii.first), ii.second, py::bytes(decompress_gzip(s)));
    });
    
    
    // include/oqt/utils/operatingsystem.hpp
    m.def("checkstats", &checkstats);
    m.def("file_size", &file_size);



    // include/oqt/utils/pbf/protobuf.hpp
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
    m.def("PbfValue", [](size_t tag, size_t val) { return PbfTag{tag,val,""}; });
    m.def("PbfData", [](size_t tag, py::bytes data) { return PbfTag{tag,0,data}; });

    m.def("read_all_pbf_tags", &read_all_pbf_tags);
    m.def("pack_pbf_tags", &pack_pbf_tags);
    
    
    // include/oqt/utils/pbf/varint.hpp
    m.def("read_packed_delta", &read_packed_delta);
    m.def("read_packed_int", &read_packed_int);
    m.def("zig_zag", &zig_zag);
    m.def("un_zig_zag", &un_zig_zag);
};
#ifdef INDIVIDUAL_MODULES
PYBIND11_PLUGIN(_utils) {
    py::module m("_utils", "bindings for oqt/utils/");
    utils_defs(m);
    return m.ptr();
}
#endif
