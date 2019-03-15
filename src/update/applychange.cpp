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

#include "oqt/update/applychange.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/update/xmlchange.hpp"

#include "oqt/pbfformat/writepbffile.hpp"
#include "gzstream.hpp"
#include "oqt/update/applychange.hpp"
#include "oqt/sorting/final.hpp"
#include "oqt/sorting/splitbyid.hpp"

#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/pbfformat/readblock.hpp"

#include "oqt/utils/logger.hpp"


#include <algorithm>

namespace oqt {

class ApplyChange { 
    public:
        ApplyChange(std::function<void(ElementPtr)> collect_, typeid_element_map_ptr changeobjs_) :
            collect(collect_), changeobjs(changeobjs_), finished(false), change_key(0) {
            
            change_iter = changeobjs->begin();
            finished = change_iter == changeobjs->end();
            if (!finished) {
                change_key = change_iter->second->InternalId();
            }
        }
        
        void add_change_obj(bool has) {
            auto o = change_iter->second;
            if (o->ChangeType() >= changetype::Modify) {
                
                o->SetChangeType(changetype::Normal);
                Logger::Message() << (has ? "replace with" : "insert") << " obj " << o->Type() << " " << o->Id();
                collect(o);
            } else {
                Logger::Message() << (has ? "remove" : "skip") << " obj " << o->Type() << " " << o->Id() << " [" << change_iter->second->ChangeType() << "]";
            }
            change_iter++;
            finished = change_iter == changeobjs->end();
            if (!finished) {
                change_key = change_iter->second->InternalId();
            }
        }
        void apply(PrimitiveBlockPtr block) {
            if (!block) {
                while (!finished) {
                    add_change_obj(false);
                }
                collect(nullptr);
                return;
            }   
            for (auto obj: block->Objects()) {
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
        std::function<void(ElementPtr)> collect;
        typeid_element_map_ptr changeobjs;
        typeid_element_map::const_iterator change_iter;
        bool finished;
        uint64 change_key;
};


void run_applychange(const std::string& origfn, const std::string& outfn, size_t numchan, const std::vector<std::string>& changes) {

    
    auto change_objs = std::make_shared<typeid_element_map>();
    
    for (const auto& fn: changes) {
        if (ends_with(fn,".osc.gz")) {
            gzstream::igzstream src_fl(fn.c_str());
            if (!src_fl.good()) {
                throw std::domain_error("couldn't open "+fn);
            }
            read_xml_change_file_em(&src_fl, change_objs, false);
            src_fl.close();
        } else if (ends_with(fn,".osc")) {
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
    Logger::Get().time("read changes files");
    Logger::Message() << "have " << change_objs->size() << " change_objs";
    
    auto head_orig = get_header_block(origfn);
    if (!head_orig->Index().empty()) {
        throw std::domain_error("source file has file index");
    }
    auto head = head_orig;//std::make_shared<header>();
    //head->box=head_orig->box;
    
    std::shared_ptr<PbfFileWriter> writer;
    if (outfn!="NONE") {
        writer = make_pbffilewriter(outfn, head);
    }
    
    std::vector<primitiveblock_callback> packers;
    if (writer) {
        packers = make_final_packers_sync(writer,numchan,0,false,true);
    } else {
        packers.push_back([](PrimitiveBlockPtr p) {});
    }
    auto collect = make_collectobjs(packers, 8000);
    
    auto applychange = std::make_shared<ApplyChange>(collect,change_objs);
    
    
    size_t nc=0;
    auto fixtags = [applychange,&nc](PrimitiveBlockPtr pp) {
        if (!pp) {
            return applychange->apply(nullptr);
        }
        for (auto o: pp->Objects()) {
            if (fix_tags(*o)) { nc++; };
            if (o->Type()==ElementType::Relation) {
                auto rel = std::dynamic_pointer_cast<Relation>(o);
                if (fix_members(*rel)) {
                    nc++;
                }
            }
        }
        applychange->apply(pp);
    };
    
    
    auto cvf = [](std::shared_ptr<FileBlock> bl) {
        if (!bl) { return PrimitiveBlockPtr(); }
        if ((bl->blocktype=="OSMData")) {
            std::string dd = bl->get_data();//decompress(std::get<2>(*bl), std::get<3>(*bl))
            return read_primitive_block(bl->idx, dd, false,ReadBlockFlags::Empty,nullptr,nullptr);
            
        }
        return std::make_shared<PrimitiveBlock>(-1);
    };
    
    
    //[applychange](PrimitiveBlockPtr bl) { applychange->apply(bl); }
    read_blocks_convfunc_primitiveblock(origfn, fixtags, {}, numchan, cvf);
    Logger::Get().time("applied change");
    
    Logger::Message() << nc << " objects with '\xef' character fixed";
    if (writer) {
        writer->finish();
        Logger::Get().time("finished file");
    }
    
}

}
    
    
