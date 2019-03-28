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

#include "oqt/geometry/process.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"

#include "oqt/elements/header.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/writeblock.hpp"

#include "oqt/sorting/sortblocks.hpp"

#include "oqt/utils/logger.hpp"

#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/splitcallback.hpp"

namespace oqt {
namespace geometry {






typedef std::function<void(PrimitiveBlockPtr)> block_callback;
//typedef std::function<bool(std::vector<PrimitiveBlockPtr>)> external_callback;


void call_all(block_callback callback, primblock_vec bls) {
    for (auto bl : bls) {
        callback(bl);
    }
}

class BlockhandlerCallbackTime {
    public:
        BlockhandlerCallbackTime(const std::string& name_, std::shared_ptr<BlockHandler> handler_, block_callback callback_)
            : name(name_), handler(handler_), callback(callback_), wait(0), exec(0), cbb(0) {}
        
        void call(PrimitiveBlockPtr bl) {
            wait += ts.since_reset();
            if (!bl) {
                auto res = handler->finish();
                exec += ts.since_reset();
                call_all(callback, res);
                callback(nullptr);
                cbb += ts.since_reset();
                
                Logger::Message() << name << ": wait=" << TmStr{wait,7,1} << ", exec=" << TmStr{exec,7,1} << ", call cb=" << TmStr{cbb,7,1};
                return;
            }
            
            auto res = handler->process(bl);
            exec += ts.since_reset();
            call_all(callback,res);
            cbb += ts.since_reset();
        }
        
        static block_callback make(const std::string& name, std::shared_ptr<BlockHandler> handler, block_callback callback) {
            auto bct = std::make_shared<BlockhandlerCallbackTime>(name,handler,callback);
            return [bct](PrimitiveBlockPtr bl) {
                bct->call(bl);
            };
        }
        
    private:
        std::string name;
        std::shared_ptr<BlockHandler> handler;
        block_callback callback;
        
        double wait, exec, cbb;
        TimeSingle ts;
};
            

block_callback blockhandler_callback(std::shared_ptr<BlockHandler> handler, block_callback callback) {
    
    
    return [handler,callback](PrimitiveBlockPtr bl) {
        if (!bl) {
            call_all(callback, handler->finish());
            callback(nullptr);
            return;
        }
        call_all(callback, handler->process(bl));
        
    };
}

            
block_callback process_geometry_blocks(
    std::vector<block_callback> final_callbacks,
    const style_info_map& style, bbox box,
    const std::vector<ParentTagSpec>& apt_spec, bool add_rels, bool add_mps,
    bool recalcqts, std::shared_ptr<FindMinZoom> findmz,
    std::function<void(mperrorvec&)> errors_callback,
    bool addwn_split) {
    
    
    auto errs = std::make_shared<mperrorvec>();
    
    std::vector<block_callback> makegeoms(final_callbacks.size());
    for (size_t i=0; i < final_callbacks.size(); i++) {
        auto finalcb = final_callbacks[i];
        if (i==0) {
            finalcb = [finalcb, errs, errors_callback](PrimitiveBlockPtr bl) {
                if (!bl && errs && (!errs->empty())) {
                    errors_callback(*errs);
                }
                finalcb(bl);
            };
        }
       
        makegeoms[i]  =threaded_callback<PrimitiveBlock>::make(
            BlockhandlerCallbackTime::make("MakeGeometries["+std::to_string(i)+"]",
                make_geometryprocess(style,box,recalcqts,findmz),
                finalcb
            )
        );
        
        
    }
    block_callback makegeoms_split;
    if (final_callbacks.size()==1) {
        makegeoms_split = makegeoms[0];
    } else {
        makegeoms_split = split_callback<PrimitiveBlock>::make(makegeoms);
    }
    
    block_callback make_mps = makegeoms_split;
    if (add_mps) {
        
        bool bounds = style.count("boundary")>0;
        bool mps = false;
        for (const auto& s: style) {
            if ((s.first!="boundary") && (s.second.IsArea)) {
                mps=true;
                break;
            }
        }
        Logger::Message() << "MultiPolygons(mps=" << mps << ", bounds=" << bounds << ")";
        make_mps = threaded_callback<PrimitiveBlock>::make(
            BlockhandlerCallbackTime::make("MultiPolygons", //blockhandler_callback(
                make_multipolygons(errs,style,box,bounds,mps),
                makegeoms_split
            )
        );
    }
            
    
    
    block_callback reltags = make_mps;
    if (add_rels) {
        bool add_bounds = style.count("boundary")>0;
        bool add_admin_levels=style.count("min_admin_level")>0;
        std::set<std::string> routes;
        for (const auto& st : style) {
            auto k = st.first;
            if ((k.size()>7) && (k.compare(k.size()-7, 7, "_routes")==0)) {
                routes.insert(k.substr(0,k.size()-7));
            }
        }
        std::cout << "make_handlerelations("
            << "add_bounds=" << (add_bounds ? "t" : "f") << ", "
            << "add_admin_levels=" << (add_admin_levels ? "t" : "f") << ", "
            << "routes={";
        bool f=true;
        for (const auto& r : routes) {
            if (!f) { std::cout << ", ";}
            std::cout << r;
            f=false;
        }
        std::cout <<"})" <<std::endl;
        
        reltags = threaded_callback<PrimitiveBlock>::make(
            BlockhandlerCallbackTime::make("HandleRelations", //blockhandler_callback(
                make_handlerelations(add_bounds, add_admin_levels, routes),
                make_mps
            )
        );
    }        
    
    block_callback apt = reltags;
    if (!apt_spec.empty()) {
        apt = threaded_callback<PrimitiveBlock>::make(
            BlockhandlerCallbackTime::make("AddParentTags", //blockhandler_callback(
                make_addparenttags(apt_spec),
                reltags
            )
        );
    }
    
    block_callback awn;
    if (addwn_split) {
        awn=make_waynodes_cb_split(apt);
    } else {
        awn=make_addwaynodes_cb(apt);
    }
    
    return threaded_callback<PrimitiveBlock>::make(awn);
        

    
}





block_callback process_geometry_blocks_nothread(
    block_callback final_callback,
    const style_info_map& style, bbox box,
    const std::vector<ParentTagSpec>& apt_spec, bool add_rels, bool add_mps,
    bool recalcqts, std::shared_ptr<FindMinZoom> findmz,
    std::function<void(mperrorvec&)> errors_callback) {
    
    auto errs = std::make_shared<mperrorvec>();
    
    block_callback make_geom = [&style, box, final_callback, errs, errors_callback, recalcqts, findmz](PrimitiveBlockPtr bl) {
        if (!bl) {
            std::cout << "make_geom done" << std::endl;
            final_callback(bl);
            if ((errs) && (!errs->empty())) {
                errors_callback(*errs);
            }
            return;
        }
                
        auto gg = make_geometries(style,box, bl);
        if (recalcqts) {
            recalculate_quadtree(gg, 18, 0.05);
        }
        if (findmz) {
            calculate_minzoom(gg, findmz);
        }
        final_callback(gg);
            
    };
    
    block_callback make_mps = make_geom;
    if (add_mps) {
        
        bool bounds = style.count("boundary")>0;
        bool mps = false;
        for (const auto& s: style) {
            if ((s.first!="boundary") && (s.second.IsArea)) {
                mps=true;
                break;
            }
        }
        make_mps = blockhandler_callback(
            make_multipolygons(errs,style,box,bounds,mps),
            make_geom);
    }
    
        
    block_callback reltags = make_mps;
    if (add_rels) {
        bool add_bounds = style.count("boundary")>0;
        bool add_admin_levels=style.count("min_admin_level")>0;
        std::set<std::string> routes;
        for (const auto& st : style) {
            auto k = st.first;
            if ((k.size()>7) && (k.compare(k.size()-7, 7, "_routes")==0)) {
                routes.insert(k.substr(0,k.size()-7));
            }
        }
        std::cout << "make_handlerelations("
            << "add_bounds=" << (add_bounds ? "t" : "f") << ", "
            << "add_admin_levels=" << (add_admin_levels ? "t" : "f") << ", "
            << "routes={";
        bool f=true;
        for (const auto& r : routes) {
            if (!f) { std::cout << ", ";}
            std::cout << r;
            f=false;
        }
        std::cout <<"})" <<std::endl;
        
        reltags = blockhandler_callback(
                make_handlerelations(add_bounds, add_admin_levels, routes),
                make_mps
            );
    }        
    
    block_callback apt = reltags;
    if (!apt_spec.empty()) {
        apt = blockhandler_callback(
                make_addparenttags(apt_spec),
                reltags
            );
    }
        
    return make_addwaynodes_cb(apt);
   
}




        
    

std::vector<block_callback> pack_and_write_callback(
    std::vector<block_callback> callbacks,
    std::string filename, bool indexed, bbox box, size_t numchan,
    bool writeqts, bool writeinfos, bool writerefs) {
        


    auto head = std::make_shared<Header>();
    head->SetBBox(box);
    
    write_file_callback write = make_pbffilewriter_filelocs_callback(filename,head);
    
    //auto write_split = threaded_callback<std::pair<int64,std::string>>::make(write, numchan);
    auto write_split = multi_threaded_callback<std::pair<int64,std::string>>::make(write, numchan);
    
    std::vector<block_callback> pack(numchan);
    
    for (size_t i=0; i < numchan; i++) {
        auto write_i = write_split[i];
        block_callback cb;
        if (!callbacks.empty()) { cb = callbacks[i]; }
        pack[i] = //threaded_callback<primitiveblock>::make(
            [write_i, cb, writeqts,writeinfos,writerefs,i](PrimitiveBlockPtr bl) {
                if (cb) {
                    cb(bl);
                }
                if (!bl) {
                    std::cout << "pack_and_write_callback finish " << i << std::endl;
                    write_i(keystring_ptr());
                    return;
                } else {
                    auto data = pack_primitive_block(bl, writeqts, false, writeinfos, writerefs);
                    auto packed = prepare_file_block("OSMData", data);
                    write_i(std::make_shared<keystring>(bl->Quadtree(),packed));
                }
            };
        //);
    }
    return pack;
}



block_callback pack_and_write_callback_nothread(
    block_callback callback,
    std::string filename, bool indexed, bbox box,
    bool writeqts, bool writeinfos, bool writerefs) {
        


    auto head = std::make_shared<Header>();
    head->SetBBox(box);
    
    write_file_callback write = make_pbffilewriter_filelocs_callback(filename,head);
    
    return [write, callback, writeqts,writeinfos,writerefs](PrimitiveBlockPtr bl) {
        if (callback) {
            callback(bl);
        }
        if (!bl) {
            write(keystring_ptr());
            return;
        } else {
            auto data = pack_primitive_block(bl, writeqts, false, writeinfos, writerefs);
            
            auto packed = prepare_file_block("OSMData", data);
            write(std::make_shared<keystring>(bl->Quadtree(),packed));
        }
    };
}
    

class GeomProgress {
    public:
        GeomProgress(const src_locs_map& locs) : nb(0), npt(0), nln(0), nsp(0), ncp(0), maxqt(0) {
            double p=100.0 / locs.size(); size_t i=0;
            for (const auto& l: locs) {
                progs[l.first] = p*i;
                i++;
            }
        }
        
        void call(PrimitiveBlockPtr bl) {
            
            if (bl) {
                nb++;
                if (bl->Quadtree() > maxqt) { maxqt = bl->Quadtree(); }
                for (auto o: bl->Objects()) {
                    if (o->Type() == ElementType::Point) { npt++; }
                    else if (o->Type() == ElementType::Linestring) { nln++; }
                    else if (o->Type() == ElementType::SimplePolygon) { nsp++; }
                    else if (o->Type() == ElementType::ComplicatedPolygon) { ncp++; }
                }
            }
            if (( (nb % 1024) == 0) || !bl) {
                std::cout << "\r" << std::setw(8) << nb
                    << std::fixed << std::setprecision(1) << std::setw(5) << progs[maxqt] << "%"
                    << " " << TmStr{ts.since(),8,1} <<  " "
                    << "[" << std::setw(18) << quadtree::string(maxqt) << "]: "
                    << std::setw(8) << npt << " "
                    << std::setw(8) << nln << " "
                    << std::setw(8) << nsp << " "
                    << std::setw(8) << ncp
                    << std::flush;
            }
        }
    private:
        int64 nb, npt, nln, nsp, ncp, maxqt;
        std::map<int64,double> progs;
        TimeSingle ts;
};
block_callback make_geomprogress(const src_locs_map& locs) {
    auto gp = std::make_shared<GeomProgress>(locs);
    return [gp](PrimitiveBlockPtr bl) { gp->call(bl); };
}

    
//std::function<void(std::shared_ptr<CsvBlock>)> csvblock_callback;

mperrorvec process_geometry(const GeometryParameters& params, block_callback wrapped) {

    mperrorvec errors_res;
    
    if (!wrapped) {
        wrapped = make_geomprogress(params.locs);
    }
    
    std::vector<block_callback> writer = multi_threaded_callback<PrimitiveBlock>::make(wrapped,params.numchan);
    
        
    if (params.outfn!="") {
        writer = pack_and_write_callback(writer, params.outfn, params.indexed, params.box, params.numchan, true, true, true);
        
    }
    
    
    
    auto addwns = process_geometry_blocks(
            writer, params.style, params.box, params.parent_tag_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](mperrorvec& ee) { errors_res.swap(ee); }, params.addwn_split
    );
    
    read_blocks_merge(params.filenames, addwns, params.locs, params.numchan, nullptr, ReadBlockFlags::Empty, 1<<14);
      
    
    return errors_res;

}



mperrorvec process_geometry_sortblocks(const GeometryParameters& params, block_callback cb) {


    
    mperrorvec errors_res;
    
    auto sb = make_sortblocks(params.locs.size()/16, params.groups, params.outfn+"-interim",50, params.numchan, 0);
    auto sb_callbacks = sb->make_addblocks_cb(false);
  
    auto addwns = process_geometry_blocks(
            sb_callbacks, params.style, params.box, params.parent_tag_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](mperrorvec& ee) { errors_res.swap(ee); }, params.addwn_split
    );
    
    read_blocks_merge(params.filenames, addwns, params.locs, params.numchan, nullptr, ReadBlockFlags::Empty, 1<<14);
    
    
    sb->finish();
    
    if ((params.outfn!="") && (!cb)) {
        auto head = std::make_shared<Header>();
        head->SetBBox(params.box);
    
        write_file_callback write = make_pbffilewriter_filelocs_callback(params.outfn,head);
        
        sb->read_blocks_packed(write);
        
    } else {
    
    
        std::vector<block_callback> writer;
        if (cb) {
            
            auto writerp = multi_threaded_callback<PrimitiveBlock>::make(cb,params.numchan);
            for (size_t i=0; i < params.numchan; i++) {
                writer.push_back(writerp[i]);
            }
                
        }
        
            
        if (params.outfn!="") {
            writer = pack_and_write_callback(writer, params.outfn, params.indexed, params.box, params.numchan, true, true, true);
            
        }
        sb->read_blocks(split_callback<PrimitiveBlock>::make(writer), true);
    
    }

    return errors_res;

}





mperrorvec process_geometry_nothread(const GeometryParameters& params, block_callback writer) {


    
    mperrorvec errors_res;
    
    
    if (params.outfn!="") {
        writer = pack_and_write_callback_nothread(writer, params.outfn, params.indexed, params.box, true, true, true);
    }
    
    
    block_callback addwns = process_geometry_blocks_nothread(
            writer, params.style, params.box, params.parent_tag_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](mperrorvec& ee) { errors_res.swap(ee); }
    );
    
    read_blocks_merge_nothread(params.filenames, addwns, params.locs, nullptr, ReadBlockFlags::Empty);
      
    
    return errors_res;

}





mperrorvec process_geometry_from_vec(
    std::vector<PrimitiveBlockPtr> blocks,
    const GeometryParameters& params,
    block_callback callback) {


    
    
    
    mperrorvec errors_res;
    
    
    auto addwns = process_geometry_blocks(
            {callback}, params.style, params.box, params.parent_tag_spec, params.add_rels,
            params.add_mps, params.recalcqts, params.findmz,
            [&errors_res](mperrorvec& ee) { errors_res.swap(ee); }, params.addwn_split
    );
    
    for (auto bl : blocks) {
        addwns(bl);
    }
    addwns(PrimitiveBlockPtr());
      
    
    return errors_res;

}

    
}
}
