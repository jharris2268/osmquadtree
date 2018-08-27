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

#include "oqt/operations.hpp"
#include "oqt/readpbffile.hpp"
#include "oqt/xmlchange.hpp"

#include "oqt/writepbffile.hpp"
#include "oqt/gzstream.hpp"
#include "oqt/sortfile.hpp"

#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/readpbf/readblock.hpp"
#include <algorithm>

namespace oqt {

class ApplyChange { 
    public:
        ApplyChange(std::function<void(element_ptr)> collect_, typeid_element_map_ptr changeobjs_) :
            collect(collect_), changeobjs(changeobjs_), finished(false), change_key(0) {
            
            change_iter = changeobjs->begin();
            finished = change_iter == changeobjs->end();
            if (!finished) {
                change_key = change_iter->second->InternalId();
            }
        }
        
        void add_change_obj(bool has) {
            auto o = change_iter->second;
            if (o->ChangeType() >= Modify) {
                
                o->SetChangeType(Normal);
                logger_message() << (has ? "replace with" : "insert") << " obj " << o->Type() << " " << o->Id();
                collect(o);
            } else {
                logger_message() << (has ? "remove" : "skip") << " obj " << o->Type() << " " << o->Id() << " [" << change_iter->second->ChangeType() << "]";
            }
            change_iter++;
            finished = change_iter == changeobjs->end();
            if (!finished) {
                change_key = change_iter->second->InternalId();
            }
        }
        void apply(primitiveblock_ptr block) {
            if (!block) {
                while (!finished) {
                    add_change_obj(false);
                }
                collect(nullptr);
                return;
            }   
            for (auto obj: block->objects) {
                while ((!finished) && (change_key < obj->InternalId())) {
                    add_change_obj(false);                    
                }
                if ((!finished) && (change_key == obj->InternalId())) {
                    add_change_obj(true);
                } else {
                    collect(obj);
                }
            }
        }
    private:
        std::function<void(element_ptr)> collect;
        typeid_element_map_ptr changeobjs;
        typeid_element_map::const_iterator change_iter;
        bool finished;
        uint64 change_key;
};


void run_applychange(const std::string& origfn, const std::string& outfn, size_t numchan, const std::vector<std::string>& changes) {

    auto lg=get_logger();
    auto change_objs = std::make_shared<typeid_element_map>();
    
    for (const auto& fn: changes) {
        if (EndsWith(fn,".osc.gz")) {
            gzstream::igzstream src_fl(fn.c_str());
            if (!src_fl.good()) {
                throw std::domain_error("couldn't open "+fn);
            }
            read_xml_change_file_em(&src_fl, change_objs, false);
            src_fl.close();
        } else if (EndsWith(fn,".osc")) {
            std::ifstream src_fl(fn, std::ios::in);
            if (!src_fl.good()) {
                throw std::domain_error("couldn't open "+fn);
            }
            read_xml_change_file_em(&src_fl, change_objs, false);
            src_fl.close();
        } else {
            throw std::domain_error(fn+" not a osc file");
        }
    }
    lg->time("read changes files");
    logger_message() << "have " << change_objs->size() << " change_objs";
    
    auto head_orig = getHeaderBlock(origfn);
    if (!head_orig->index.empty()) {
        throw std::domain_error("source file has file index");
    }
    auto head = head_orig;//std::make_shared<header>();
    //head->box=head_orig->box;
    
    std::shared_ptr<PbfFileWriter> writer;
    if (outfn!="NONE") {
        writer = make_pbffilewriter(outfn, head, false);
    }
    
    std::vector<primitiveblock_callback> packers;
    if (writer) {
        packers = make_final_packers_sync(writer,numchan,0,false,true);
    } else {
        packers.push_back([](primitiveblock_ptr p) {});
    }
    auto collect = make_collectobjs(packers, 8000);
    
    auto applychange = std::make_shared<ApplyChange>(collect,change_objs);
    
    
    size_t nc=0;
    auto fixtags = [applychange,&nc](primitiveblock_ptr pp) {
        if (!pp) {
            return applychange->apply(nullptr);
        }
        for (auto o: pp->objects) {
            if (fix_tags(*o)) { nc++; };
            if (o->Type()==Relation) {
                auto rel = std::dynamic_pointer_cast<relation>(o);
                if (fix_members(*rel)) {
                    nc++;
                }
            }
        }
        applychange->apply(pp);
    };
    
    
    auto cvf = [](std::shared_ptr<FileBlock> bl) {
        if (!bl) { return primitiveblock_ptr(); }
        if ((bl->blocktype=="OSMData")) {
            std::string dd = bl->get_data();//decompress(std::get<2>(*bl), std::get<3>(*bl))
            return readPrimitiveBlock(bl->idx, dd, false,7,nullptr,nullptr);
            
        }
        return std::make_shared<primitiveblock>(-1);
    };
    
    
    //[applychange](primitiveblock_ptr bl) { applychange->apply(bl); }
    read_blocks_convfunc<primitiveblock>(origfn, fixtags, {}, numchan, cvf);
    lg->time("applied change");
    
    logger_message() << nc << " objects with '\xef' character fixed";
    if (writer) {
        writer->finish();
        lg->time("finished file");
    }
    
}

}
    
    
