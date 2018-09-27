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
struct element_map {
    typeid_element_map_ptr u;
};

void add_all_element_map(std::shared_ptr<objs_idset> ii, const element_map& mm) {
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

std::tuple<std::shared_ptr<qtstore>,std::shared_ptr<qtstore>,std::shared_ptr<qttree>>
    add_orig_elements_py(element_map& em, const std::string& prfx, std::vector<std::string> fls) {
    py::gil_scoped_release r;
    return add_orig_elements(em.u,prfx,fls);
}

std::tuple<std::shared_ptr<qtstore>,std::shared_ptr<qtstore>,std::shared_ptr<qttree>>
    add_orig_elements_alt_py(element_map& em, const std::string& prfx, std::vector<std::string> fls) {
    py::gil_scoped_release r;
    return add_orig_elements_alt(em.u,prfx,fls);
}

std::vector<PrimitiveBlockPtr> find_change_tiles_py(element_map& em, std::shared_ptr<qtstore> orig_allocs,
    std::shared_ptr<qttree> tree,
    int64 startdate,
    int64 enddate){

    py::gil_scoped_release r;
    return find_change_tiles(em.u,orig_allocs,tree,startdate,enddate);
}



void calc_change_qts_py(element_map& em, std::shared_ptr<qtstore> qts) {
    py::gil_scoped_release r;
    calc_change_qts(em.u, qts);
}

std::pair<int64,int64> find_change_all_py(const std::string& src, const std::string& prfx, const std::vector<std::string>& fls, int64 st, int64 et, const std::string& outfn) {

    py::gil_scoped_release r;
    return find_change_all(src,prfx,fls,st,et,outfn);
}

size_t writeIndexFile_py(const std::string& fn, size_t numchan, const std::string& outfn) {
    py::gil_scoped_release r;
    return writeIndexFile(fn,numchan,outfn);
}

std::set<int64> checkIndexFile_py(const std::string& idxfn, std::shared_ptr<header> head, size_t numchan, std::shared_ptr<idset> ids) {
    py::gil_scoped_release r;
    return checkIndexFile(idxfn,head,numchan,ids);
}

class WritePbfFile {
    public:
        virtual void write(std::vector<PrimitiveBlockPtr> blocks)=0;
        virtual std::pair<int64,int64> finish()=0;
        virtual ~WritePbfFile() {}
};

class WritePbfFileImpl : public WritePbfFile {
    public:
        WritePbfFileImpl(const std::string& fn, bbox bounds, size_t numchan_, bool indexed, bool dropqts_, bool change_, bool tempfile)
            : numchan(numchan_), dropqts(dropqts_), change(change_) {
            
            
            auto head = std::make_shared<header>();
            head->box=bounds;
            if ((!tempfile)&&(indexed)) {
                outobj = make_pbffilewriter_indexedinmem(fn, head);
            } else {
                outobj = make_pbffilewriter(fn, head, indexed);
            }
            
        }

        void write(std::vector<PrimitiveBlockPtr> blocks) {
            
            
            
            if (blocks.size()<numchan) {
                for (auto bl: blocks) {
                    auto dd = write_and_pack_pbfblock(bl);
                    
                    outobj->writeBlock(dd->first,dd->second);
                }
                return;
            }
            
            auto outcb = multi_threaded_callback<keystring>::make([this](keystring_ptr p) {
                if (p) {
                    outobj->writeBlock(p->first,p->second);
                }
            }, numchan);
            
            std::vector<std::function<void(PrimitiveBlockPtr)>> packers;
            for (auto cb: outcb) {
                packers.push_back(threaded_callback<PrimitiveBlock>::make(
                    [cb,this](PrimitiveBlockPtr pb) {
                        if (!pb) { return cb(nullptr); }
                        cb(write_and_pack_pbfblock(pb));
                    }
                ));
            }
            
            for (size_t i=0; i < blocks.size(); i++) {
                packers[i%packers.size()](blocks.at(i));
            }
            for (auto& p: packers) { p(nullptr); }
        }
        
        
        
        
        std::pair<int64,int64> finish() {
            
            auto rr = outobj->finish();
            
            auto ll = std::get<1>(rr.back())+std::get<2>(rr.back());
            return std::make_pair(ll, rr.size());
        }
        
        
    private:
        
        std::shared_ptr<PbfFileWriter> outobj;
        size_t numchan;
        bool indexed;
        bool dropqts;
        bool change;
        
        
        
        keystring_ptr write_and_pack_pbfblock(PrimitiveBlockPtr bl) {
            auto data = writePbfBlock(bl, !dropqts, change, true, true);
            auto block = prepareFileBlock("OSMData", data);
            return std::make_shared<keystring>(bl->Quadtree(), block);
        }
            
        

};

std::shared_ptr<WritePbfFile> make_WritePbfFile(const std::string& fn, bbox bounds, size_t numchan, bool indexed, bool dropqts, bool change, bool tempfile) {
    return std::make_shared<WritePbfFileImpl>(fn,bounds,indexed,numchan,dropqts,change,tempfile);
}
PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void change_defs(py::module& m) {
    m.def("getHeaderBlock", &getHeaderBlock);
    m.def("checkIndexFile", &checkIndexFile_py);
    m.def("writeIndexFile", writeIndexFile_py, py::arg("fn"), py::arg("numchan")=4, py::arg("outfn")="");
    m.def("writePbfBlock", 
        [](PrimitiveBlockPtr b, bool incQts, bool change, bool incInfo, bool incRefs) {
                return py::bytes(writePbfBlock(b,incQts,change,incInfo,incRefs)); },
        py::arg("block"), py::arg("includeQts")=true, py::arg("change")=false, py::arg("includeInfo")=true, py::arg("includeRefs")=true);
    

    m.def("combine_primitiveblock_two", &combine_primitiveblock_two);
    m.def("combine_primitiveblock_many", &combine_primitiveblock_many);


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

    py::class_<qtstore, std::shared_ptr<qtstore>>(m,"qtstore")
        .def("__getitem__", [](qtstore& qq, std::pair<int64,int64> p) { return qq.at((p.first<<61) | p.second);})
        .def("expand", [](qtstore& qq, int64 t, int64 i, int64 q) { return qq.expand((t<<61) | i, q);})
        .def("__contains__", [](qtstore& qq, std::pair<int64,int64> p) { return qq.contains((p.first<<61) | p.second);})
        .def("__len__",&qtstore::size)
        .def("first", &qtstore::first)
        .def("next", &qtstore::next)
    ;
    m.def("make_qtstore", &make_qtstore_map);

    py::class_<WritePbfFile, std::shared_ptr<WritePbfFile>>(m, "WritePbfFile_obj")
        //.def(py::init<std::string,bool,bool,bool,bool,size_t,bbox>())
        .def("write", [](WritePbfFile& wpf, std::vector<PrimitiveBlockPtr> tls) {
            py::gil_scoped_release r;
            wpf.write(tls);
        })
        .def("finish", [](WritePbfFile& wpf)->std::pair<int,int> {
            py::gil_scoped_release r;
            return wpf.finish();
        })
    ;
    m.def("WritePbfFile", make_WritePbfFile,
            py::arg("filename"), py::arg("bounds"),
            py::arg("numchan")=4, py::arg("indexed")=true,
            py::arg("dropqts")=false, py::arg("change")=false,
            py::arg("tempfile")=true);

    m.def("read_xml_change", &read_xml_change);
    m.def("read_xml_change_file", &read_xml_change_file_py);
    m.def("read_xml_change_em", [](std::string d, element_map& em, bool allow_missing_users) {
        if (!em.u) { em.u=std::make_shared<typeid_element_map>(); }
        read_xml_change_em(d,em.u,allow_missing_users);
    });
    m.def("read_xml_change_file_em", &read_xml_change_file_em_py);

    m.def("read_file_blocks", [](const std::string& fn, std::vector<int64> locs, size_t numchan,size_t index_offset, bool change, size_t objflags, std::shared_ptr<idset> ids   ) {
        py::gil_scoped_release r;
        return read_file_blocks(fn,locs,numchan,index_offset,change,objflags,ids);
    });
    m.def("add_orig_elements", &add_orig_elements_py);
    m.def("add_orig_elements_alt", &add_orig_elements_alt_py);

    m.def("find_change_tiles", &find_change_tiles_py);
    m.def("calc_change_qts", &calc_change_qts_py);

    m.def("find_change_all", &find_change_all_py);


}
