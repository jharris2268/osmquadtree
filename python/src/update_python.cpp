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

#include "oqt/update/update.hpp"
#include "oqt/update/xmlchange.hpp"
#include "gzstream.hpp"

using namespace oqt;
struct element_map {
    typeid_element_map_ptr u;
};

void add_all_element_map(std::shared_ptr<ObjsIdSet> ii, const element_map& mm) {
    if (!mm.u) { return; }
    for (auto& o : (*mm.u)) {
        if (o.second->Type()==ElementType::Node) {
            auto n = std::dynamic_pointer_cast<Node>(o.second);
            if (!n) { throw std::domain_error("WTF"); }
            ii->add_node(n);
        } else if (o.second->Type()==ElementType::Way) {
            auto w = std::dynamic_pointer_cast<Way>(o.second);
            if (!w) { throw std::domain_error("WTF"); }
            ii->add_way(w);
        } else if (o.second->Type()==ElementType::Relation) {
            auto r = std::dynamic_pointer_cast<Relation>(o.second);
            if (!r) { throw std::domain_error("WTF"); }
            ii->add_relation(r);
        }
    }
}




PrimitiveBlockPtr read_xml_change_file_py(std::string fn, bool isgzip) {
    if (isgzip) {
        gzstream::igzstream f(fn.c_str());
        return read_xml_change_file(&f);
    }
    std::ifstream f(fn,std::ios::in|std::ios::binary);
    return read_xml_change_file(&f);
}

void read_xml_change_file_em_py(std::string fn, bool isgzip, element_map& em, bool allow_missing_users) {
    if (!em.u) { em.u=std::make_shared<typeid_element_map>(); }
    if (isgzip) {
        gzstream::igzstream f(fn.c_str());
        return read_xml_change_file_em(&f,em.u,allow_missing_users);
    }
    std::ifstream f(fn,std::ios::in|std::ios::binary);
    return read_xml_change_file_em(&f,em.u, allow_missing_users);
}

std::tuple<std::shared_ptr<QtStore>,std::shared_ptr<QtStore>,std::shared_ptr<QtTree>>
    add_orig_elements_py(element_map& em, const std::string& prfx, std::vector<std::string> fls) {
    py::gil_scoped_release r;
    return add_orig_elements(em.u,prfx,fls);
}
std::tuple<std::shared_ptr<QtStore>,std::shared_ptr<QtStore>,std::shared_ptr<QtTree>>
    add_orig_elements_altxx_py(element_map& em, const std::string& prfx, std::vector<std::string> fls) {
    py::gil_scoped_release r;
    return add_orig_elements_altxx(em.u,prfx,fls);
}
std::tuple<std::shared_ptr<QtStore>,std::shared_ptr<QtStore>,std::shared_ptr<QtTree>>
    add_orig_elements_alt_py(element_map& em, const std::string& prfx, std::vector<std::string> fls) {
    py::gil_scoped_release r;
    return add_orig_elements_alt(em.u,prfx,fls);
}

std::vector<PrimitiveBlockPtr> find_change_tiles_py(element_map& em, std::shared_ptr<QtStore> orig_allocs,
    std::shared_ptr<QtTree> tree,
    int64 startdate,
    int64 enddate){

    py::gil_scoped_release r;
    return find_change_tiles(em.u,orig_allocs,tree,startdate,enddate);
}



void calc_change_qts_py(element_map& em, std::shared_ptr<QtStore> qts) {
    py::gil_scoped_release r;
    calc_change_qts(em.u, qts);
}

std::pair<int64,int64> find_change_all_py(const std::vector<std::string>& src_filenames, const std::string& prfx, const std::vector<std::string>& fls, int64 st, int64 et, const std::string& outfn) {

    py::gil_scoped_release r;
    return find_change_all(src_filenames,prfx,fls,st,et,outfn);
}
size_t write_index_file_py(const std::string& fn, size_t numchan, const std::string& outfn) {
    py::gil_scoped_release r;
    return write_index_file(fn,numchan,outfn);
}

std::set<int64> check_index_file_py(const std::string& idxfn, HeaderPtr head, size_t numchan, IdSetPtr ids) {
    py::gil_scoped_release r;
    return check_index_file(idxfn,head,numchan,ids);
}
PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void update_defs(py::module& m) {
    


    m.def("add_all_element_map", &add_all_element_map);
    py::class_<element_map>(m,"element_map")
        .def(py::init<>())
        .def("__getitem__",[](const element_map& f, std::pair<ElementType,int64> t) {
            if (!f.u) { throw pybind11::index_error("nf"); }
            auto it=f.u->find(t);
            if (it==f.u->end()) { throw pybind11::index_error("nf"); }
            return it->second;
        })
        .def("__setitem__",[](element_map& f, std::pair<ElementType,int64> t,  ElementPtr u) {
            if (!f.u) {
                f.u = std::make_shared<std::map<std::pair<ElementType,int64>,ElementPtr>>();
            }
            (*f.u)[t]=u; //.insert(std::make_pair(t,u));
        })
        .def("__len__",[](const element_map& f)->int {
            if (!f.u) { return 0; }
            return f.u->size();
        })
        .def("__contains__",[](const element_map& f, std::pair<ElementType,int64> t) {
            if (!f.u) { return false; }
            return f.u->count(t)==1;
        })
        .def("__iter__", [](const element_map& f) {
            if (!f.u) { throw pybind11::index_error("nf"); }
            return py::make_iterator(f.u->begin(),f.u->end());
        }, py::keep_alive<0,1>())
        .def("erase", [](element_map& f, std::pair<ElementType,int64> t) { if (!f.u) { return; } f.u->erase(t); })
        .def("clear", [](element_map& f) {
            if (!f.u) { return; }
            f.u->clear();
            typeid_element_map e;
            f.u->swap(e);
            f.u.reset();
        })
    ;

    
    m.def("read_xml_change", &read_xml_change);
    m.def("read_xml_change_file", &read_xml_change_file_py);
    m.def("read_xml_change_em", [](std::string d, element_map& em, bool allow_missing_users) {
        if (!em.u) { em.u=std::make_shared<typeid_element_map>(); }
        read_xml_change_em(d,em.u,allow_missing_users);
    });
    m.def("read_xml_change_file_em", &read_xml_change_file_em_py);

    
    m.def("add_orig_elements", &add_orig_elements_py);
    m.def("add_orig_elements_alt", &add_orig_elements_alt_py);
    m.def("add_orig_elements_altxx", &add_orig_elements_altxx_py);

    m.def("find_change_tiles", &find_change_tiles_py);
    m.def("calc_change_qts", &calc_change_qts_py);

    m.def("find_change_all", &find_change_all_py);

    m.def("check_index_file", &check_index_file_py);
    m.def("write_index_file", &write_index_file_py, py::arg("fn"), py::arg("numchan")=4, py::arg("outfn")="");
    
    m.def("read_file_blocks", [](const std::string& fn, std::vector<int64> locs, size_t numchan,size_t index_offset, bool change, ReadBlockFlags objflags, IdSetPtr ids   ) {
        py::gil_scoped_release r;
        return read_file_blocks(fn,locs,numchan,index_offset,change,objflags,ids);
    });
    
    m.def("make_idset", [](element_map &em) { return make_idset(em.u);  });
}

#ifdef INDIVIDUAL_MODULES
PYBIND11_MODULE(_update, m) {
    //py::module m("_update", "pybind11 example plugin");
    update_defs(m);
    //return m.ptr();
}
#endif
