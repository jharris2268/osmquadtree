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

#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/pbfformat/idset.hpp"
#include "oqt/pbfformat/objsidset.hpp"
#include "oqt/pbfformat/readblock.hpp"
#include "oqt/pbfformat/readblockscaller.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/readfileparallel.hpp"
#include "oqt/pbfformat/readminimal.hpp"
#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/pbfformat/writepbffile.hpp"

#include "oqt/sorting/mergechanges.hpp"
#include "oqt/geometry/utils.hpp"

using namespace oqt;

template <class BlockType>
size_t read_blocks_merge_py(
    std::vector<std::string> filenames,
    std::function<bool(std::vector<std::shared_ptr<BlockType>>)> callback,
    src_locs_map locs, size_t numchan, size_t numblocks,
    IdSetPtr filter, ReadBlockFlags objflags,
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
    IdSetPtr filter, bool ischange, ReadBlockFlags objflags) {

    
    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    primitiveblock_callback cbf = [cb](PrimitiveBlockPtr bl) { cb->call(bl); };

    read_blocks_primitiveblock(filename, cbf, locs, numchan, filter, ischange, objflags);
    return cb->total();
}


size_t read_blocks_minimalblock_py(
    const std::string& filename,
    std::function<bool(std::vector<minimal::BlockPtr>)> callback,
    std::vector<int64> locs, size_t numchan, size_t numblocks,
    ReadBlockFlags objflags) {


    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<minimal::Block>>(wrap_callback(callback),numblocks);
    minimalblock_callback cbf = [cb](minimal::BlockPtr bl) { cb->call(bl); };
    read_blocks_minimalblock(filename, cbf, locs, numchan, objflags);
    return cb->total();
}


size_t read_blocks_caller_read_primitive(
    std::shared_ptr<ReadBlocksCaller> rbc,
    std::function<bool(std::vector<PrimitiveBlockPtr>)> callback,
    size_t numchan,
    size_t numblocks,
    ReadBlockFlags flags,
    IdSetPtr filter) {
        
    
    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<PrimitiveBlock>>(wrap_callback(callback),numblocks);
    auto cbf = multi_threaded_callback<PrimitiveBlock>::make([cb](PrimitiveBlockPtr bl) { cb->call(bl); },numchan);
    rbc->read_primitive(cbf, flags, filter);
    return cb->total();
    
    
}

size_t read_blocks_caller_read_minimal(
    std::shared_ptr<ReadBlocksCaller> rbc,
    std::function<bool(std::vector<minimal::BlockPtr>)> callback,
    size_t numchan,
    size_t numblocks,
    ReadBlockFlags flags,
    IdSetPtr filter) {
        
    
    py::gil_scoped_release r;
    auto cb = std::make_shared<collect_blocks<minimal::Block>>(wrap_callback(callback),numblocks);
    auto cbf = multi_threaded_callback<minimal::Block>::make([cb](minimal::BlockPtr bl) { cb->call(bl); },numchan);
    
    rbc->read_minimal(cbf, flags, filter);
    return cb->total();
    
    
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
        auto mm = read_all_pbf_tags(dd);
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
            first = read_pbf_tag(it->data,p).value;
        }
        if (mm.size()>2) {
            size_t p=0;
            last = read_pbf_tag(mm.back().data,p).value;
        }
        //std::cout << " " << first << " " << last << std::flush;
        return std::make_shared<tempobjs_tup>(fb->idx,fb->file_position,k,mm.size()-1,first,last);
    };
    
    read_blocks_convfunc<tempobjs_tup>(fn, cbf, {}, numchan, convblock);
    //read_tempobjs_file(cb, fn, {}, numchan);
};



void read_some_blocks_primitiveblock(
    std::shared_ptr<ReadFile> readfile, 
    primitiveblock_callback callback,
    size_t numblocks,
    size_t numchan,
    IdSetPtr filter, bool ischange, ReadBlockFlags objflags) {
        
    auto combine = multi_threaded_callback<PrimitiveBlock>::make(callback, numchan);
    
                
    std::vector<std::function<void(std::shared_ptr<FileBlock>)>> cbs;
    for (auto c: combine) {
        cbs.push_back(
            threaded_callback<FileBlock>::make(
                [c,filter,ischange,objflags](std::shared_ptr<FileBlock> fb) {
                    if (!fb) {
                        c(nullptr);
                    } else {
                        c(read_as_primitiveblock(fb, filter, ischange, objflags));
                    }
                }
            )
        );
    }
        
                    
    for (size_t i=0; i < numblocks; i++) {
        auto fb = readfile->next();
        if (!fb) { break; }
        cbs[fb->idx%numchan](fb);
    }
    
    
    for (auto& c: cbs) {
        c(nullptr);
    }
    
}


class ReadBlocksIter {
    
    public:
        ReadBlocksIter(const std::string& fn, std::vector<int64> locs, size_t numchan_, size_t nb_, std::shared_ptr<IdSet> ids_,bool change_, ReadBlockFlags flags_)
            : numchan(numchan_), change(change_), flags(flags_), ids(ids_), nb(nb_) {
                
            int64 fs = file_size(fn);
            readfile = make_readfile(fn, locs, 0, 0, fs);
                
        }
        
        
        
        std::vector<PrimitiveBlockPtr> next() {
            
            py::gil_scoped_release r;

            
            
            std::vector<PrimitiveBlockPtr> ans;
            auto cb = [&ans,this](PrimitiveBlockPtr bl) {
                if (bl && (bl->size()>0)) {
                    ans.push_back(bl);
                }
            };
            
            read_some_blocks_primitiveblock(readfile, cb, nb, numchan, ids, change, flags);
            
            return ans;
        }   
    private:
    
        size_t numchan;
        bool change;
        ReadBlockFlags flags;
        std::shared_ptr<IdSet> ids;
        size_t nb;
        
        std::shared_ptr<ReadFile> readfile;
            
    
};      

ReadBlockFlags make_readblockflags(
    bool skip_nodes, bool skip_ways, bool skip_relations, bool skip_geometries,
    bool skip_strings, bool skip_info, bool use_alternative) {
    
    ReadBlockFlags ans = ReadBlockFlags::Empty;
    
    if (skip_nodes) { ans = ans | ReadBlockFlags::SkipNodes; }
    if (skip_ways) { ans = ans | ReadBlockFlags::SkipWays; }
    if (skip_relations) { ans = ans | ReadBlockFlags::SkipRelations; }
    if (skip_geometries) { ans = ans | ReadBlockFlags::SkipGeometries; }
    if (skip_strings) { ans = ans | ReadBlockFlags::SkipStrings; }
    if (skip_info) { ans = ans | ReadBlockFlags::SkipInfo; }
    if (use_alternative) { ans = ans | ReadBlockFlags::UseAlternative; }
    return ans;
}




PrimitiveBlockPtr read_primitive_block_py(int64 idx, py::bytes data, bool change) {
    return read_primitive_block(idx,data,change,ReadBlockFlags::Empty,nullptr,geometry::read_geometry);
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
            
            
            auto head = std::make_shared<Header>();
            head->SetBBox(bounds);
            if ((!tempfile)&&(indexed)) {
                outobj = make_pbffilewriter_indexedinmem(fn, head);
            } else if (indexed) {
                outobj = make_pbffilewriter_filelocs(fn, head);
            } else {
                outobj = make_pbffilewriter(fn, head);
            }
            
        }

        void write(std::vector<PrimitiveBlockPtr> blocks) {
            
            
            
            if ((numchan==0) || (blocks.size()<numchan)) {
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
            auto data = pack_primitive_block(bl, !dropqts, change, true, true);
            auto block = prepare_file_block("OSMData", data);
            return std::make_shared<keystring>(bl->Quadtree(), block);
        }
            
        

};

std::shared_ptr<WritePbfFile> make_WritePbfFile(const std::string& fn, bbox bounds, size_t numchan, bool indexed, bool dropqts, bool change, bool tempfile) {
    return std::make_shared<WritePbfFileImpl>(fn,bounds,indexed,numchan,dropqts,change,tempfile);
}
   
    

PYBIND11_DECLARE_HOLDER_TYPE(XX, std::shared_ptr<XX>);
void pbfformat_defs(py::module& m) {
    


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
    
    py::class_<IdSetRange,IdSet,std::shared_ptr<IdSetRange>>(m,"IdSetRange")
        .def(py::init<ElementType,int64,int64>())
        .def_readonly("ty", &IdSetRange::ty)
        .def_readonly("min_id", &IdSetRange::min_id)
        .def_readonly("max_id", &IdSetRange::max_id)
    ;
    
    py::class_<IdSetInvert, IdSet, std::shared_ptr<IdSetInvert>>(m, "IdSetInvert")
        .def(py::init<IdSetPtr>())
    ;

    
    m.def("read_primitive_block", &read_primitive_block_py, py::arg("index"), py::arg("data"), py::arg("change"));
    m.def("read_primitive_block_new", [](size_t idx, const std::string& d, bool c) { return read_primitive_block_new(idx,d,c,ReadBlockFlags::Empty,nullptr,geometry::read_geometry); },
        py::arg("index"), py::arg("data"), py::arg("change"));

    
    
    m.def("read_minimal_block", &read_minimal_block);
    m.def("read_header_block", &read_header_block);

    
    
    py::enum_<ReadBlockFlags>(m, "ReadBlockFlags_");
    m.def("ReadBlockFlags", &make_readblockflags, 
        py::arg("skip_nodes")=false, py::arg("skip_ways")=false,
        py::arg("skip_relations")=false, py::arg("skip_geometries")=false,
        py::arg("skip_strings")=false, py::arg("skip_info")=false,
        py::arg("use_alternative")=false
    );
    
/*        .value("Empty", ReadBlockFlags::Empty)
        .value("SkipNodes", ReadBlockFlags::SkipNodes)
        .value("SkipWays", ReadBlockFlags::SkipWays)
        .value("SkipRelations", ReadBlockFlags::SkipRelations)
        .value("SkipGeometries", ReadBlockFlags::SkipGeometries)
        .value("SkipStrings", ReadBlockFlags::SkipStrings)
        .value("SkipInfo", ReadBlockFlags::SkipInfo)
        .value("UseAlternative", ReadBlockFlags::UseAlternative)
    ;*/
    
    
    py::class_<ReadBlocksCaller, std::shared_ptr<ReadBlocksCaller>>(m, "ReadBlocksCaller")
        .def("num_tiles", &ReadBlocksCaller::num_tiles)
    ;
    
    m.def("read_blocks_caller_read_primitive", &read_blocks_caller_read_primitive);
    m.def("read_blocks_caller_read_minimal", &read_blocks_caller_read_minimal);
    m.def("make_read_blocks_caller", &make_read_blocks_caller);

    m.def("calc_idset_filter", [](std::shared_ptr<ReadBlocksCaller> rbc, bbox bx, std::vector<LonLat> llv, size_t nc) {
        py::gil_scoped_release r;
        return calc_idset_filter(rbc,bx,llv,nc);
    });


    m.def("read_blocks_primitive", &read_blocks_primitiveblock_py,
        py::arg("filename"), py::arg("callback"), py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("filter")=nullptr, py::arg("ischange")=false,py::arg("objflags")=ReadBlockFlags::Empty);
    
    m.def("read_blocks_minimal", &read_blocks_minimalblock_py,
        py::arg("filename"), py::arg("callback"), py::arg("locs")=std::vector<int64>(),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("objflags")=ReadBlockFlags::Empty);
   
   m.def("read_blocks_merge_primitive", &read_blocks_merge_py<PrimitiveBlock>,
        py::arg("filenames"), py::arg("callback"), py::arg("locs"),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("filter")=nullptr, py::arg("objflags")=ReadBlockFlags::Empty, py::arg("buffer")=0);
    
   m.def("read_blocks_merge_minimal", &read_blocks_merge_py<minimal::Block>,
        py::arg("filenames"), py::arg("callback"), py::arg("locs"),
        py::arg("numchan")=4,py::arg("numblocks")=32,
        py::arg("filter")=nullptr, py::arg("objflags")=ReadBlockFlags::Empty, py::arg("buffer")=0);
    
   
   m.def("read_blocks_tempobjs", &read_blocks_tempobjs);
  
   py::class_<ReadBlocksIter, std::shared_ptr<ReadBlocksIter>>(m, "ReadBlocksIter")
        .def(py::init<std::string,std::vector<int64>,size_t,size_t,std::shared_ptr<IdSet>,bool,ReadBlockFlags>(),
            py::arg("fn"), py::arg("locs")=std::vector<int64>(),
            py::arg("numchan")=4,py::arg("numblocks")=256,
            py::arg("filter")=nullptr, py::arg("ischange")=false,py::arg("objflags")=ReadBlockFlags::Empty)
     
        .def("next", &ReadBlocksIter::next)
    ;
    
    
    m.def("get_header_block", &get_header_block);
    
    m.def("pack_primitive_block", 
        [](PrimitiveBlockPtr b, bool incQts, bool change, bool incInfo, bool incRefs) {
                return py::bytes(pack_primitive_block(b,incQts,change,incInfo,incRefs)); },
        py::arg("block"), py::arg("includeQts")=true, py::arg("change")=false, py::arg("includeInfo")=true, py::arg("includeRefs")=true);
    

    
    

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
    
   
    py::class_<FileBlock,std::shared_ptr<FileBlock>>(m, "FileBlock")
        .def_readonly("idx", &FileBlock::idx)
        .def_readonly("blocktype", &FileBlock::blocktype)
        .def_readonly("uncompressed_size", &FileBlock::uncompressed_size)
        .def_readonly("compressed", &FileBlock::compressed)
        .def_readonly("file_position", &FileBlock::file_position)
        .def_readonly("file_progress", &FileBlock::file_progress)
        .def_property_readonly("data", [](const FileBlock& fb) { return py::bytes(fb.data); })
        .def("get_data", [](FileBlock& fb) { return py::bytes(fb.get_data()); })
    ;
   
    py::class_<ReadFile,std::shared_ptr<ReadFile>>(m, "ReadFile")
        .def("file_position", &ReadFile::file_position)
        .def("next", &ReadFile::next)
    ;
        
    m.def("make_readfile", make_readfile, py::arg("filename"), py::arg("locs")=std::vector<int64>(), py::arg("index_offset")=0, py::arg("buffer")=0, py::arg("file_size")=0);
    
}

#ifdef INDIVIDUAL_MODULES
PYBIND11_MODULE(_pbfformat, m) {
    //py::module m("_pbfformat", "pybind11 example plugin");
    pbfformat_defs(m);
    //return m.ptr();
}
#endif
