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

#include "oqt/update/update.hpp"

#include "gzstream.hpp"


#include "oqt/pbfformat/readfile.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/elements/minimalblock.hpp"

#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"
#include "oqt/pbfformat/objsidset.hpp"


#include "oqt/pbfformat/writeblock.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/logger.hpp"

namespace oqt {
int64 make_internalid(ElementType ty, int64 rf) {
    if (ty==ElementType::Node) { return rf; }
    if (ty==ElementType::Way) { return (1ll<<61) | rf; }
    if (ty==ElementType::Relation) { return (2ll<<61) | rf; }
    throw std::domain_error("wrong type");
}


bool has_id(ElementType ty, const std::string& id_data, IdSetPtr ids) {
    size_t id_pos=0;
    int64 id=0;
    while ( (id_pos < id_data.size())) {
        id += readVarint(id_data,id_pos);

        if (ids->contains(ty, id)) {
            return true;
        }
    }
    return false;
}

void checkIndexBlock(const std::string& ss, IdSetPtr ids, std::shared_ptr<std::set<int64>> blocks) {

    size_t pos=0;

    int64 qt =-1;

    for (auto tg = readPbfTag(ss,pos); tg.tag>0; tg = readPbfTag(ss,pos)) {
        if (tg.tag==1) { qt=unZigZag(tg.value); }

        //if ((tg.tag==2) || (tg.tag==3) || (tg.tag==4)) {
        if (tg.tag==2) {
            if (has_id(ElementType::Node, tg.data, ids)) {
                blocks->insert(qt);
            }
        } else if (tg.tag==3) {
            if (has_id(ElementType::Way, tg.data, ids)) {
                blocks->insert(qt);
            }
        } else if (tg.tag==4) {
            if (has_id(ElementType::Relation, tg.data, ids)) {
                blocks->insert(qt);
            }
        }

    }
}




template <class T>
std::string pack_minimal_ids(const std::vector<T>& in) {
    std::string res(10+in.size()*5, 0);
    size_t p=0;
    int64 i=0;
    for (auto o : in) {
        if ((p+10) > res.size()) { res.resize(res.size()+100); }
        p = writeVarint(res,p,o.id-i);
        i=o.id;

    }
    return res.substr(0,p);
}


std::string makeIndexBlock(PrimitiveBlockPtr in) {
    std::vector<int64> n,w,r;
    for (auto o : in->Objects()) {
        if (o->Type()==ElementType::Node) { n.push_back(o->Id());}
        if (o->Type()==ElementType::Way) { w.push_back(o->Id());}
        if (o->Type()==ElementType::Relation) { r.push_back(o->Id());}
    }

    std::list<PbfTag> mm;
    mm.push_back(PbfTag{1,zigZag(in->Quadtree()),""});
    if (!n.empty()) {
        mm.push_back(PbfTag{2,0,writePackedDelta(n)});
    }
    if (!w.empty()) {
        mm.push_back(PbfTag{3,0,writePackedDelta(w)});
    }
    if (!r.empty()) {
        mm.push_back(PbfTag{4,0,writePackedDelta(r)});
    }
    std::string dd = packPbfTags(mm);

    return prepare_file_block("IndexBlock", dd);
}

size_t writeIndexFile(const std::string& fn, size_t numchan, const std::string& outfn_in) {
    if ((numchan==0) || (numchan > 8)) {
        throw std::domain_error("numchan should be between 1 and 8");
    }
    std::string outfn = outfn_in;
    if (outfn=="") {
        outfn = fn + "-index.pbf";
    }
    
    std::vector<int64> locs;
    auto hh = getHeaderBlock(fn);
    if (hh && (!hh->Index().empty())) {
        for (auto& l : hh->Index()) {
            locs.push_back(std::get<1>(l));
        }
    } else {
        throw std::domain_error("not an indexed pbf file: "+fn);
    }
        
    auto out_obj = make_pbffilewriter(outfn, nullptr, false);
    auto out_callback = multi_threaded_callback<keystring>::make([out_obj](keystring_ptr p) {
        if (p) {
            out_obj->writeBlock(p->first,p->second);
        }
    }, numchan);
    
    std::vector<std::function<void(PrimitiveBlockPtr)>> converts;
    for (auto oo: out_callback) {
        converts.push_back(threaded_callback<PrimitiveBlock>::make(
            [oo](PrimitiveBlockPtr mb) {
                if (!mb) {
                    return oo(nullptr);
                }
                oo(std::make_shared<keystring>(mb->Quadtree(), makeIndexBlock(mb)));
            }
        ));
    }           
    
    
    
    read_blocks_split_primitiveblock(fn, converts, locs, nullptr, false, 7);
    
    auto ii = out_obj->finish();
    
    Logger::Message() << "written " << ii.size() << " blocks, " << std::get<1>(ii.back())+std::get<2>(ii.back()) << " bytes";
    return ii.size();
}



void checkIdxBlock(const std::string& ss, IdSetPtr ids, std::shared_ptr<std::set<int64>> blocks, HeaderPtr head) {
    size_t pos=0;
    std::string id_data,bl_data;

    for (auto tg = readPbfTag(ss,pos); tg.tag>0; tg = readPbfTag(ss,pos)) {
        if (tg.tag==1) { id_data=tg.data; }
        if (tg.tag==2) { bl_data=tg.data; }
    }
    size_t id_pos=0, bl_pos=0;
    int64 id=0, bl=0;
    while ( (id_pos < id_data.size()) && (bl_pos < bl_data.size())) {
        id += readVarint(id_data,id_pos);
        bl += readVarint(bl_data,bl_pos);
        ElementType t = (ElementType) (id>>59);
        if (ids->contains(t, id&0xffffffffffffll)) {
            blocks->insert(std::get<0>(head->Index().at(bl)));
        }
    }
}

std::set<int64> checkIndexFile(const std::string& idxfn, HeaderPtr head, size_t numchan, IdSetPtr ids) {
    if ((numchan==0) || (numchan > 8)) {
        throw std::domain_error("numchan should be between 1 and 8");
    }


    auto conv_func = [head,ids](std::shared_ptr<FileBlock> bl) -> std::shared_ptr<std::set<int64>> {
        if (!bl) { return nullptr; }
        auto block = std::make_shared<std::set<int64>>();
        if (bl->blocktype=="IdxBlock") {
            checkIdxBlock(bl->get_data(), ids, block, head);
            return block;
        } else if (bl->blocktype=="IndexBlock") {
            checkIndexBlock(bl->get_data(), ids, block);
            return block;
        }
        throw std::domain_error("not an indexblock!!!");
        return nullptr;
    };
    
    std::set<int64> result;
    
    auto cb = [&result](std::shared_ptr<std::set<int64>> t) {
        if (!t) { return; }
        for (const auto& x: *t) {
            result.insert(x);
        }
    };
    
    read_blocks_convfunc<std::set<int64>>(idxfn, cb, {}, numchan, conv_func);
    
    return result;
}








std::vector<PrimitiveBlockPtr> read_file_blocks(
    const std::string& fn, std::vector<int64> locs, size_t numchan,
    size_t index_offset, bool change, size_t objflags, IdSetPtr ids) {


    
    std::vector<PrimitiveBlockPtr> res;
    if (locs.empty()) { return res; }
    if (locs.size() < numchan) {
        numchan=locs.size();
   }

    auto cb = [&res](PrimitiveBlockPtr bl) {
        if (bl) {
            res.push_back(bl);
        }
    };
    
    read_blocks_primitiveblock(fn, cb, locs, numchan, ids, change, objflags);
    return res;
}

IdSetPtr make_idset(typeid_element_map_ptr em) {
    auto ids = std::make_shared<ObjsIdSet>();
    for (auto& tie : *em) {
        ids->add(tie.first.first, tie.first.second);
        if (tie.first.first==ElementType::Way) {
            auto w = std::dynamic_pointer_cast<Way>(tie.second);
            for (auto& n : w->Refs()) {
                ids->add(ElementType::Node,n);
            }
        } else if (tie.first.first==ElementType::Relation) {
            auto r = std::dynamic_pointer_cast<Relation>(tie.second);
            for (auto& m : r->Members()) {
                ids->add(m.type,m.ref);
            }
        }
    }
    return ids;
}

std::tuple<std::shared_ptr<QtTree>,std::vector<std::string>, src_locs_map> check_index_files(const std::string& prfx, const std::vector<std::string>& fls, IdSetPtr ids) {
    
    src_locs_map locs_all;
    std::set<int64> passed;
    std::vector<std::string> outfl;
    auto tree=make_tree_empty();
    
    
    for (size_t i=0; i < fls.size(); i++) {
        std::string fn = prfx+fls[i];
        outfl.push_back(fn);
        auto hh = getHeaderBlock(fn);
        
        for (auto& q: hh->Index()) {
            if (i==0) {
                tree->add(std::get<0>(q),1);
            }
            locs_all[std::get<0>(q)].push_back(std::make_pair(i, std::get<1>(q)));
        }
        
        
        Logger::Progress(i*100.0/fls.size()) << "scan " << fn+"-index.pbf [" << passed.size() << " locs]";
        auto p = checkIndexFile(fn+"-index.pbf", hh, 4, ids);
        for (auto& q: p) {
            passed.insert(q);
        }
    }
    Logger::Progress(100.0)  << "scaned " << fls.size() << " files, have" << passed.size() << " locs";
    src_locs_map locs;
    for (auto& ll: locs_all) {
        if (passed.count(ll.first)>0) {
            locs[ll.first]=ll.second;
        }
    }
    return std::make_tuple(tree, outfl, locs);
}

std::tuple<std::shared_ptr<QtStore>,std::shared_ptr<QtStore>,std::shared_ptr<QtTree>> add_orig_elements_alt(
    typeid_element_map_ptr em, const std::string& prfx, const std::vector<std::string>& fls) {
    
    auto ids = make_idset(em);
    std::shared_ptr<QtTree> tree; std::vector<std::string> fl; src_locs_map locs;
    std::tie(tree, fl, locs) = check_index_files(prfx, fls, ids);
    
    auto allocs = make_qtstore_map();
    auto qts = make_qtstore_map();
    size_t orig_size=em->size();
    auto cb = [allocs, qts, em, orig_size](PrimitiveBlockPtr bl) {
        if (!bl) { return; }
        Logger::Progress(bl->FileProgress()) << " read " << qts->size() << " qts, added " << (em->size()-orig_size) << " nodes"; 
        for (auto o : bl->Objects()) {
            int64 k =o->InternalId();//(o->Type() << 61) | o->Id();
            allocs->expand(k, bl->Quadtree());
            qts->expand(k, o->Quadtree());
            if (o->Type()==ElementType::Node) {
                //o->SetChangeType(Normal);
                auto k2=std::make_pair(o->Type(),o->Id());
                if (em->count(k2)==0) {
                    (*em)[k2] = o;
                }
            }
        }
    };
    
    read_blocks_merge<PrimitiveBlock>(fl, cb, locs, 4, ids, 7, 1<<14);
    return std::make_tuple(allocs, qts,tree);
}
    


std::tuple<std::shared_ptr<QtStore>,std::shared_ptr<QtStore>,std::shared_ptr<QtTree>> add_orig_elements(
    typeid_element_map_ptr em, const std::string& prfx, const std::vector<std::string>& fls) {


    auto ids = make_idset(em);
    

    std::vector<HeaderPtr> headers;

    std::set<int64> tt;
    size_t fi=0;
    for (auto& f: fls) {
        headers.push_back(getHeaderBlock(prfx+f));
        Logger::Progress(fi*100/fls.size()) << "scan " << f+"-index.pbf [" << tt.size() << " locs]";
        auto p = checkIndexFile(prfx+f+"-index.pbf", headers.back(), (headers.back()->Index().size()>20000 ? 4 : 1), ids);
        for (auto& q: p) { tt.insert(q); }
        ++fi;
    }
    Logger::Progress(100) << "scaned " << fls.size() << " files, have" << tt.size() << " locs";


    auto tree=make_tree_empty();
    for (auto& q: headers[0]->Index()) {
        tree->add(std::get<0>(q),1);
    }
    auto allocs = make_qtstore_map();
    auto qts = make_qtstore_map();
    std::set<int64> dels;
    size_t ne=0;
    for (size_t i=0; i < fls.size(); i++) {
        size_t j = fls.size()-i-1;
        auto fn=prfx+fls[j];
        std::vector<int64> locs;
        for (auto& p: headers[j]->Index()) {
            if (tt.count(std::get<0>(p))==1) {
                locs.push_back(std::get<1>(p));
            }
        }
        if (!locs.empty()) {
            Logger::Progress(i*100.0/fls.size()) << "have " << em->size() << "objs (" << ne << " empty blocks): read " << locs.size() << " blocks from " << fn;
            auto bb = read_file_blocks(fn,locs,4,0,true,7,ids);
            for (auto bl: bb) {
                if (bl->size() == 0) {
                    
                    ne++;
                } else {
                    for (auto o : bl->Objects()) {
                        int64 k =o->InternalId();//(o->Type() << 61) | o->Id();
                        if (dels.count(k)==1) { continue; }
                        if ((o->ChangeType()==changetype::Normal) || (o->ChangeType()==changetype::Unchanged) || (o->ChangeType()==changetype::Modify) || (o->ChangeType()==changetype::Create)) {

                            if (allocs->contains(k)==0) {
                                allocs->expand(k, bl->Quadtree());
                                qts->expand(k, o->Quadtree());
                                if (o->Type()==ElementType::Node) {
                                    o->SetChangeType(changetype::Normal);
                                    auto k2=std::make_pair(o->Type(),o->Id());
                                    if (em->count(k2)==0) {
                                        (*em)[k2] = o;
                                    }
                                }
                            }
                        }
                        if (o->ChangeType()==changetype::Delete) {
                            dels.insert(k);
                        }
                    }
                }
            }
            
        }
    }
    Logger::Progress(100) << " have " << em->size() << "objs (" << ne << " empty blocks)";

    return std::make_tuple(allocs, qts,tree);
}

void calc_change_qts(typeid_element_map_ptr em, std::shared_ptr<QtStore> qts) {
    std::set<int64> nq;
    Logger::Message() << "calc way qts";
    for (auto& pp : (*em)) {
        if ((pp.first.first == ElementType::Way) && (pp.second->ChangeType()>changetype::Delete)) {
            bbox bx;
            auto o = std::dynamic_pointer_cast<Way>(pp.second);
            for (auto& n: o->Refs()) {
                auto it=em->find(std::make_pair(ElementType::Node,n));
                if (it==em->end()) { throw std::domain_error("node not present"); }
                auto no = std::dynamic_pointer_cast<Node>(it->second);
                expand_point(bx, no->Lon(),no->Lat());
            }
            int64 k = pp.second->InternalId();//(1ll<<61) | pp.first.second;
            qts->expand(k, -1);
            int64 qt = quadtree::calculate(bx.minx,bx.miny,bx.maxx,bx.maxy,0.05,18);
            if (qt<0) {
                Logger::Message() << bx << "=>" << qt;
                throw std::domain_error("??");
            }
            for (auto& n: o->Refs()) {
                qts->expand(n,qt);
                nq.insert(n);
            }
            qts->expand(k,qt);
        }
    }
    Logger::Message() << "calc node qts";
    for (auto& pp : (*em)) {
        if ((pp.first.first == ElementType::Node) && (pp.second->ChangeType()>changetype::Delete)) {

            if (nq.count(pp.first.second)==0) {
                auto o = std::dynamic_pointer_cast<Node>(pp.second);
                int64 qt = quadtree::calculate(o->Lon(),o->Lat(),o->Lon(),o->Lat(),0.05,18);
                qts->expand(pp.first.second,qt);
            }
        }
    }
    Logger::Message() << "calc rel qts";
    std::multimap<int64,int64> rr;
    for (auto& pp : (*em)) {
        if ((pp.first.first == ElementType::Relation) && (pp.second->ChangeType()>changetype::Delete)) {
            auto obj = std::dynamic_pointer_cast<Relation>(pp.second);
            int64 k = pp.second->InternalId();
            if (obj->Members().empty()) {
                qts->expand(k,0);
            } else {
                std::list<Member> missing_members;
                
                for (auto& m : obj->Members()) {
                    if (m.type==ElementType::Relation) {
                        rr.insert(std::make_pair(pp.first.second, m.ref));
                    } else {
                        int64 mk = make_internalid(m.type,m.ref);
                        if (!qts->contains(mk)) {
                            missing_members.push_back(m);
                            
                        } else {
                            auto d = qts->at(mk);
                            qts->expand(k,d);
                        }
                    }
                }
                if (!missing_members.empty()) {
                    Logger::Message msg;
                    msg << "relation " << obj->Id() << " missing " << missing_members.size() << " member qts";
                    if (missing_members.size() < 10) {
                        for (const auto& m: missing_members) {
                            msg << " (" << m.type << ", " << m.ref << "),";
                        }
                    }
                }
                            
            }
        }
    }
    for (size_t z=0; z< 5; z++) {
        for (auto& r: rr) {
            int64 k0 = make_internalid(ElementType::Relation, r.first);
            int64 k1 = make_internalid(ElementType::Relation, r.second);
            if (qts->contains(k1)) {
                qts->expand(k0, qts->at(k1));
            }
        }
    }
    std::list<std::pair<ElementType,int64>> mms;
    for (auto& pp:  (*em)) {
        auto k = pp.second->InternalId();//(pp.first.first<<61) | pp.first.second;
        if (pp.second->ChangeType()==changetype::Normal) {

            if (qts->at(k)==pp.second->Quadtree()) {
                mms.push_back(pp.first);
            } else {
                pp.second->SetQuadtree(qts->at(k));
                pp.second->SetChangeType(changetype::Unchanged);
            }
        } else if (pp.second->ChangeType()>changetype::Remove) {
            pp.second->SetQuadtree(qts->at(k));
        }
    }
    Logger::Message() << "remove " << mms.size() << " unneeded extra nodes";
    for (auto& m: mms) {
        em->erase(m);
    }
    

}

std::vector<PrimitiveBlockPtr> find_change_tiles(
    typeid_element_map_ptr em, std::shared_ptr<QtStore> orig_allocs,
    std::shared_ptr<QtTree> tree, int64 ss, int64 ee) {

    std::map<int64, PrimitiveBlockPtr> tiles;
    auto check_tile = [&tiles,ss,ee](int64 a) {
        if (tiles.count(a)==0) {
            tiles[a]=std::make_shared<PrimitiveBlock>(tiles.size(),0);
            tiles[a]->SetQuadtree(a);
            tiles[a]->SetStartDate(ss);
            tiles[a]->SetEndDate(ee);
        }
    };


    for (auto& pp : (*em)) {
        auto o = pp.second;
        int64 k = o->InternalId();//(pp.first.first<<61) | pp.first.second;
        if (o->ChangeType()>changetype::Remove) {
            int64 a = tree->find_tile(o->Quadtree()).qt;
            check_tile(a);
            tiles[a]->add(o);
            if (orig_allocs->contains(k) && (orig_allocs->at(k) != a)) {
                auto n = o->copy();
                n->SetChangeType(changetype::Remove);
                n->SetQuadtree(0);
                int64 na=orig_allocs->at(k);
                check_tile(na);
                tiles[na]->add(n);
            }
        } else if (orig_allocs->contains(k)) {
            int64 a = orig_allocs->at(k);
            o->SetQuadtree(0);
            check_tile(a);
            tiles[a]->add(o);
        }
    }
    std::vector<PrimitiveBlockPtr> res;
    for (auto& t: tiles) {
        res.push_back(t.second);
    }
    return res;
}



std::pair<int64,int64> find_change_all(const std::string& src, const std::string& prfx, const std::vector<std::string>& fls, int64 st, int64 et, const std::string& outfn) {
    typeid_element_map_ptr objs = std::make_shared<typeid_element_map>();

    gzstream::igzstream src_fl(src.c_str());
    read_xml_change_file_em(&src_fl, objs, true);
    std::shared_ptr<QtStore> qts, orig_allocs;
    std::shared_ptr<QtTree> tree;
    std::tie(orig_allocs,qts,tree) =  add_orig_elements(objs, prfx, fls);
    calc_change_qts(objs, qts);
    auto tiles = find_change_tiles(objs, orig_allocs, tree, st, et);

    
    auto head = std::make_shared<Header>();
    head->SetBBox(bbox{-1800000000,-900000000,1800000000,900000000});

    auto out = make_pbffilewriter_indexedinmem(outfn, head);
    for (auto bl: tiles) {
        auto dd = writePbfBlock(bl, true, true, true, true);
        auto p = prepare_file_block("OSMData",dd);
        out->writeBlock(bl->Quadtree(),p);
    }

    auto xx=out->finish();
    int64 gp = std::get<1>(xx.back())+std::get<2>(xx.back());
    
    return std::make_pair(tiles.size(), gp);
}
}
