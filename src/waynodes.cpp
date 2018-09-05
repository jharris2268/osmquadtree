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

#include "oqt/waynodes.hpp"
#include "oqt/writefile.hpp"
#include "oqt/quadtree.hpp"
#include <algorithm>
#include <exception>
#include <sstream>
#include <iomanip>

#include "oqt/readpbffile.hpp"
namespace oqt {
typedef std::function<void(std::shared_ptr<minimalblock>)> minimalblock_callback;
typedef std::function<void(std::shared_ptr<way_nodes>)> waynodes_callback;


template <class T, int N>
std::string packDeltaIntArr(const T& arr, size_t fr, size_t to) {
    int64 last = 0;
    size_t pos=0;
    std::string result((to-fr)*10,0);
    for (size_t i=fr; i < to; i++) {
        pos = writeVarint(result, pos, std::get<N>(arr.at(i))-last);
        last=std::get<N>(arr.at(i));
    }
    return result.substr(0,pos);
}


template <class T>
keystring_ptr pack_noderefs_alt(int64 k, std::shared_ptr<T> v, size_t first, size_t last, int comp_level) {

    std::string buf((last-first)*20+20, 0);
    size_t pos=0;
    pos = writePbfValue(buf, pos, 1, zigZag(k));
    pos = writePbfData(buf, pos, 2, packDeltaIntArr<T,1>(*v, first, last));
    pos = writePbfData(buf, pos, 3, packDeltaIntArr<T,0>(*v, first, last));
    buf.resize(pos);
    return std::make_shared<keystring>(k, prepareFileBlock("WayNodes", buf, comp_level));
    
}

   


template <class T1, class T2>
bool sort_cmp_first(const std::pair<T1,T2>& l, const std::pair<T1,T2>& r) {
    return l.first < r.first;
}


class way_nodes_impl : public way_nodes {
    typedef std::pair<int64,int64> wn;
    std::vector<wn> waynodes;
    size_t l;
    int64 key_;
    
    public:
        way_nodes_impl(size_t cap) : waynodes(cap), l(0),key_(-1) {};
        way_nodes_impl(size_t cap, int64 k) : waynodes(cap), l(0),key_(k) {};


        virtual ~way_nodes_impl() {}


        bool add(int64 w, int64 n, bool resize) {
            if (l==waynodes.size()) {
                if (resize) {
                    waynodes.resize(waynodes.size()+16384);
                    logger_message() << "resize waynode to " << waynodes.size()+16384;
                } else {
                    throw std::range_error("way_nodes_impl::add with resize=false");
                }
            }
            waynodes[l] = std::make_pair(w,n);
            l++;
            return l==waynodes.size();
        }
        
        
        void reserve(size_t n) {
            if (n > (waynodes.size()-l)) {
                waynodes.resize(l+n);
            }
        }
        
        
        void reset(bool empty) {
            l=0;
            if (empty) {
                std::vector<std::pair<int64,int64>> e;
                waynodes.swap(e);
            }
        };
        virtual size_t size() const { return l; }
        virtual std::pair<int64,int64> at(size_t i) const {
            if (i>l) { throw std::range_error("way_nodes_impl::at"); }
            return waynodes[i];
        }


        void sort_way() {
            if (l==0) { return; }
            std::sort(waynodes.begin(),waynodes.begin()+l, [](const wn& lft, const wn& rht) { return std::tie(lft.first,lft.second) < std::tie(rht.first,rht.second); });
        }

        void sort_node() {
            if (l==0) { return; }
            std::sort(waynodes.begin(),waynodes.begin()+l, [](const wn& lft, const wn& rht) { return std::tie(lft.second,lft.first) < std::tie(rht.second,rht.first); });
        }
        
        int64 key() const { return key_; }
        
        
        std::string str() {
            std::stringstream ss;
            ss << "way_nodes_impl(" << key_ << " " << l << " objs [";
            if (l>0) {
                const auto& f = at(0);
                ss << f.first << ", " << f.second;
                if (l>1) {
                    const auto& ls = at(l-1);
                    ss << " => " << ls.first << ", " << ls.second;
                }
            }
            ss<<"]";
            return ss.str();
        }
        
        int64 way_at(size_t i) { return waynodes[i].first; }
        int64 node_at(size_t i) { return waynodes[i].second; }


};

keystring_ptr pack_noderefs_try2(std::shared_ptr<way_nodes_impl> tile) {
    size_t sz=tile->size();
    
    size_t node_len = packedDeltaLength_func([&tile](size_t i) { return tile->node_at(i); }, sz);
    size_t way_len = packedDeltaLength_func([&tile](size_t i) { return tile->way_at(i); }, sz);
    
    size_t len = pbfValueLength(1, zigZag(tile->key()))
        + pbfDataLength(2, node_len)
        + pbfDataLength(3, way_len)
        + pbfValueLength(4, sz);
    
    std::string buf(len,0);
    size_t pos=0;
    
    pos = writePbfValue(buf, pos, 1, zigZag(tile->key()));
    
    pos = writePbfDataHeader(buf, pos, 2, node_len);
    pos = writePackedDeltaInPlace_func(buf, pos, [&tile](size_t i) { return tile->node_at(i); }, sz);
    
    pos = writePbfDataHeader(buf, pos, 3, way_len);
    pos = writePackedDeltaInPlace_func(buf, pos, [&tile](size_t i) { return tile->way_at(i); }, sz);
    
    pos = writePbfValue(buf, pos, 4, sz);
    
    if (pos!=len) {
        logger_message() << "pack_noderefs_try2 failed (key=" << tile->key() << ", sz=" << sz << " node_len=" << node_len << ", way_len=" << way_len << ", len=" << len << " != pos=" << pos;
        throw std::domain_error("failed");
    }
    
    return std::make_shared<keystring>(tile->key(), prepareFileBlock("WayNodes", buf, 1));
    
}
    
void unpack_noderefs(const std::string& data, int64 key, way_nodes_impl& waynodes, int64 minway, int64 maxway) {
    size_t pos=0;
    PbfTag tag = readPbfTag(data,pos);
    std::string nn,ww;

    for ( ; tag.tag>0; tag=readPbfTag(data,pos)) {
        if (tag.tag==1) {
            if (unZigZag(tag.value)!=key) {
                throw std::domain_error("wrong key");
            }
        } else if (tag.tag==2) {
            nn = tag.data;
        } else if (tag.tag==3) {
            ww = tag.data;

        } else if (tag.tag==4) {
            waynodes.reserve(tag.value);
        }
    }

    size_t np=0;
    size_t wp=0;
    int64 nd=0,wy=0;

    while (np < nn.size()) {
        nd += readVarint(nn,np);
        wy += readVarint(ww,wp);
        if ((wy>=minway) && ((maxway==0) || (wy<maxway))) {
            waynodes.add(wy,nd,true);
        }
   }
}

std::shared_ptr<way_nodes> read_waynodes_block(const std::string& data, int64 minway, int64 maxway) {
    size_t pos=0;
    PbfTag tag = readPbfTag(data,pos);
    std::string nn,ww;
    int64 ky=-1;
    size_t sz=0;
    for ( ; tag.tag>0; tag=readPbfTag(data,pos)) {
        if (tag.tag==1) {
            ky = unZigZag(tag.value);
        } else if (tag.tag==2) {
            nn = tag.data;
        } else if (tag.tag==3) {
            ww = tag.data;

        } else if (tag.tag==4) {
            sz=tag.value;
        }
    }

    size_t np=0;
    size_t wp=0;
    int64 nd=0,wy=0;
    
    auto res = std::make_shared<way_nodes_impl>(sz,ky);
    
    while (np < nn.size()) {
        nd += readVarint(nn,np);
        wy += readVarint(ww,wp);
        if ((wy>=minway) && ((maxway==0) || (wy<maxway))) {
            res->add(wy,nd,true);
        }
   }
   return res;
}


class WayNodesCommon {
    public:
        WayNodesCommon(
            minimalblock_callback relations_, size_t block_size_,
            int64 split_at_) :
            relations(relations_),
            block_size(block_size_), split_at(split_at_) {}
        
        virtual ~WayNodesCommon() {}
        
        virtual void finish_tile(size_t i) {};
        std::shared_ptr<way_nodes_impl> tile_at(size_t k) {
            if (k>=waynodes.size()) { return nullptr; }
            return waynodes[k];
        }
        void reset_tile(size_t k) {
            if (k<waynodes.size()) { waynodes[k]->reset(false); }
        }
        void empty_tiles() {
            for (auto& t: waynodes) { t.reset(); }
            std::vector<std::shared_ptr<way_nodes_impl>> x;
            waynodes.swap(x);
        }
        size_t num_tiles() { return waynodes.size(); }
        
        void call(std::shared_ptr<minimalblock> block) {
            if (!block) {
                
                for (size_t k=0; k < waynodes.size(); k++) {
                    if (waynodes[k]) {
                        finish_tile(k);
                    }
                }
                    
            
                relations(nullptr);
                
                return;
            }
            for (const auto& w : block->ways) {
                int64 ndref=0;
                size_t pos=0;
                while ( pos < w.refs_data.size()) {
                    ndref += readVarint(w.refs_data,pos);
                    int64 ki = ndref/split_at;
                    if (ki<0) { throw std::domain_error("???"); }
                    size_t k = ki;
                    
                    
                    if (k>=waynodes.size()) {
                        waynodes.resize(k+1);
                    }
                    if (!waynodes[k]) {
                        waynodes[k] = std::make_shared<way_nodes_impl>(block_size==0?16384:block_size,k);
                    }
                    
                    if (waynodes[k]->add(w.id,ndref,block_size==0)) {
                        finish_tile(k);
                        
                        
                    }

                }
            }
            
            if (block->relations.size()>0) {
                relations(block);
            }
            
        }
    private:
        
        minimalblock_callback relations;
        
        size_t block_size;
        int64 split_at;
        
       std::vector<std::shared_ptr<way_nodes_impl>> waynodes;
};
            
        
            



class WriteWayNodes : public WayNodesCommon {
    public:
        WriteWayNodes(
            write_file_callback writer_, minimalblock_callback relations,
            size_t block_size, int64 split_at, int comp_level_) :
            
            WayNodesCommon(relations, block_size, split_at),
            writer(writer_),comp_level(comp_level_) {}
        
        virtual ~WriteWayNodes() {
            logger_message() << "~WriteWayNodes";
        }
        
        virtual void call(std::shared_ptr<minimalblock> block) {
            WayNodesCommon::call(block);
            if (!block) {
                writer(nullptr);
                empty_tiles();
            }
        }
        
        
        
        virtual void finish_tile(size_t k) {
            auto tile = tile_at(k);
            if (tile) {
                tile->sort_way();
                //writer(pack_noderefs_alt(k, tile, 0, tile->size(),comp_level));
                writer(pack_noderefs_try2(tile));
                reset_tile(k);
            }
        }
        
    private:
        write_file_callback writer;
        int comp_level;
        
};

minimalblock_callback make_write_waynodes_callback(
    write_file_callback writer, minimalblock_callback relations,
            size_t block_size, int64 split_at, int comp_level) {

    auto wwn = std::make_shared<WriteWayNodes>(writer,relations,block_size,split_at,comp_level);
    return [wwn](std::shared_ptr<minimalblock> mb) {
        wwn->call(mb);
    };
}




class calculate_relations_impl : public calculate_relations {
    typedef int32_t relation_id_type;
    typedef std::pair<relation_id_type,relation_id_type> id_pair;
    typedef std::vector<id_pair> id_pair_vec;

    typedef std::multimap<relation_id_type,relation_id_type> id_pair_map;


    std::shared_ptr<qtstore> rel_qts;

    std::map<int64,id_pair_vec> rel_nodes;
    bool nodes_sorted;
    
    id_pair_vec rel_ways;
    id_pair_vec rel_rels;
    std::vector<relation_id_type> empty_rels;
    bool objs_sorted;
    
    size_t node_rshift;
    size_t node_mask;
    
    std::vector<bool> node_check;
    
    public:
        calculate_relations_impl() : rel_qts(make_qtstore_map()),nodes_sorted(false), node_rshift(30) {
            node_mask = (1ull<<node_rshift) - 1;
            logger_message() << "calculate_relations_impl: node_rshift = " << node_rshift << ", node_mask = " << node_mask;
            
        }

        
        virtual void add_relations_data(std::shared_ptr<minimalblock> data) {
            if (!data) { return; }
            if (data->relations.empty()) { return; }
            
            
            for (const auto& r: data->relations) {
                if (r.refs_data.empty()) {
                    empty_rels.push_back(r.id);
                }
                auto rfs = readPackedDelta(r.refs_data);
                auto tys = readPackedInt(r.tys_data);
                for (size_t i=0; i < tys.size(); i++) {
                    int64 rf=rfs[i];
                    if (tys[i]==0) {
                        
                        int64 s = rf>>node_rshift;
                    
                        id_pair_vec& objs = rel_nodes[s];
                        if (objs.size()==objs.capacity()) {
                            objs.reserve(objs.capacity()+(1<<(node_rshift-12)));
                        }
                        objs.push_back(std::make_pair(rf&node_mask, r.id));
                    } else {
                        id_pair_vec& objs = ((tys[i]==1) ? rel_ways : rel_rels);


                        if (objs.size()==objs.capacity()) {
                            objs.reserve(objs.capacity()+(1<<18));
                        }
                        objs.push_back(std::make_pair(rf, r.id));
                    }
                }
                
            }
        }



        
        virtual void add_nodes(std::shared_ptr<minimalblock> mb) {
            auto cmpf = [](const id_pair& l, const id_pair& r) { return l.first < r.first; };
            if (!nodes_sorted) {
                logger_message() << "sorting nodes [RSS = " << std::fixed << std::setprecision(1) << getmemval(getpid())/1024.0 << "]";
                if (rel_nodes.empty()) {
                    logger_message() << "no nodes...";
                } else {
                    int64 mx = rel_nodes.rbegin()->first;
                    
                    node_check.resize((mx+1) << node_rshift);
                    for (auto& rn : rel_nodes) {
                        std::sort(rn.second.begin(), rn.second.end(), cmpf);
                        int64 k = rn.first << node_rshift;
                        for (const auto& n: rn.second) {
                            node_check[k+n.first]=true;
                        }
                        
                    }
                    
                    logger_message() << "node_check.size() = " << node_check.size() << " [RSS = " << std::fixed << std::setprecision(1) << getmemval(getpid())/1024.0 << "]";
                }
                nodes_sorted=true;
            }
            if (!mb) { return; }
            if (mb->nodes.size()==0) { return; }
            
            
            for (const auto& n : mb->nodes) {
                int64 rf = n.id;
                
                if ((((size_t) rf)>=node_check.size()) || (!node_check[rf])) {
                    continue;
                }
                
                int64 qt=n.quadtree;
                int64 rf0 = rf>>node_rshift;
                relation_id_type rf1 = rf&node_mask;
                if (!rel_nodes.count(rf0)) {
                    
                    continue;
                }
                const id_pair_vec& tile = rel_nodes[rf0];
                
                id_pair_vec::const_iterator it,itEnd;
                std::tie(it,itEnd) = std::equal_range(tile.begin(), tile.end(), std::make_pair(rf1,0), cmpf);
                
                if ((it<tile.end()) && (it<itEnd)) {
                    
                        
                    
                    for ( ; it<itEnd; ++it) {
                        if (it->first != rf1) {
                            logger_message() << "??? rel_node " << (int64) it->first + (rf>>node_rshift) << " " << it->second << " matched by " << rf;
                            continue;
                        }
                        rel_qts->expand(it->second, qt);
                        
                        
                        
                    }
                }
            }
        }
        

        virtual void add_ways(std::shared_ptr<qtstore> qts) {
            for (const auto& wq : rel_ways) {
                int64 q=qts->at(wq.first);
                if (q>=0) {
                    rel_qts->expand(wq.second, q);
                }
               
                
                
            }
            id_pair_vec e;
            rel_ways.swap(e);
        }
        
        void finish_alt(std::function<void(int64,int64)> func) {
            for (auto r : empty_rels) {
                rel_qts->expand(r,0);
            }

            for (size_t i=0; i < 5; i++) {
                for (auto rr : rel_rels) {
                    if (rel_qts->contains(rr.first)) {
                        rel_qts->expand(rr.second, rel_qts->at(rr.first));
                    }
                }
            }
            for (auto rr : rel_rels) {
                if (!rel_qts->contains(rr.second)) {
                    logger_message() << "no child rels ??" << rr.first << " " << rr.second;
                    rel_qts->expand(rr.second, 0);
                }
            }
            /*for (auto r : empty_rels) {
                rel_qts->expand(r,0);
            }*/


            
            int64 rf = rel_qts->first();
            for ( ; rf > 0; rf=rel_qts->next(rf)) {
                func(rf, rel_qts->at(rf));
            }
        }
        
        
        

        virtual std::string str() {
            size_t rnn=0,rnc=0;
            for (auto& tl : rel_nodes) {
                //if (tl.second) {
                    rnn += tl.second.size();
                    rnc += tl.second.capacity();
              //}
            }
            /*size_t rww=0, rwc=0;
            for (auto& tl : rel_ways) {
                if (tl.second) {
                    rww += tl.second->second.size();
                    rwc += tl.second->second.capacity();
                }
            }*/

            std::stringstream ss;
            ss  << "have: " << rel_qts->size() << " rel qts\n"
                //<< "      " << rel_nodes.size() << " rel_nodes \n"//tiles\n"
                << "      " << rnn << " rel_nodes [cap=" << rnc << "]\n"
                //<< "      " << rel_nodes.size() << " rel_nodes [cap=" << rel_nodes.capacity() << "]\n"
                //<< "      " << rel_ways.size() << " rel_ways \n"//tiles\n"
                //<< "      " << rww << " rel_ways [cap=" << rwc << "]\n"
                << "      " << rel_ways.size() << " rel_ways [cap=" << rel_ways.capacity() << "]\n"
                << "      " << rel_rels.size() << " rel_rels \n"//[cap=" << rel_rels.capacity() << "]\n"
                << "      " << empty_rels.size() << " empty_rels";
            return ss.str();
        }


};

std::shared_ptr<calculate_relations> make_calculate_relations() {
    return std::make_shared<calculate_relations_impl>();
}

 

class MergeWayNodes {
    public:
        typedef std::function<void(std::shared_ptr<way_nodes>)> callback;
        MergeWayNodes(callback cb_) : cb(cb_), key(-1) {
            
            //curr = std::make_shared<way_nodes_impl>(16384,0);
            
        }
        void finish_temp() {
            size_t sz=0;
            for (const auto& t: temp) {
                sz+=t->size();
            }
            auto curr=std::make_shared<way_nodes_impl>(sz, key);
            int64 w,n;
            
            for (auto& t: temp) {
                for (size_t i=0; i < t->size(); i++) {
                    std::tie(w,n)=t->at(i);
                    curr->add(w,n, true);
                }
                t.reset();
            }
            curr->sort_node();
            cb(curr);
            std::vector<std::shared_ptr<way_nodes>> x;
            temp.swap(x);
        }
            
            
        
        
        void call(std::shared_ptr<way_nodes> wns) {
            if (!wns) {
                finish_temp();
                                
                /*if (curr->size()>0) {
                    curr->sort_node();
                    cb(curr);
                }*/
                cb(nullptr);
                return;
            }
            if (wns->key() != key) {
                finish_temp();
                key=wns->key();
                
                /*if (curr->size()>0) {
                    curr->sort_node();
                    
                    cb(curr);
                }
                curr = std::make_shared<way_nodes_impl>(16384,wns->key());
                */
            }
            
            temp.push_back(wns);
            
            /*for (size_t i=0; i < wns->size(); i++) {
                const auto& w = wns->at(i);
                curr->add(w.first,w.second,true);
            }*/
            
        }
    private:
        
        
        callback cb;
        
        std::vector<std::shared_ptr<way_nodes>> temp;
        int64 key;
        
        //std::shared_ptr<way_nodes_impl> curr;
};
void read_merged_waynodes(const std::string& fn, const std::vector<int64>& locs, std::function<void(std::shared_ptr<way_nodes>)> cb, int64 minway, int64 maxway) {
    
    auto merged_waynodes = std::make_shared<MergeWayNodes>(cb);
    
    auto mwns = [merged_waynodes](std::shared_ptr<way_nodes> s) { merged_waynodes->call(s); };

    auto read_waynode_func = [minway,maxway](std::shared_ptr<FileBlock> fb) -> std::shared_ptr<way_nodes> {
        if (!fb) { 
            return nullptr;
        }
        if (fb->blocktype!="WayNodes") {
            logger_message() << "?? " << fb->idx << " " << fb->blocktype;
            throw std::domain_error("not a waynodes blocks");
        }
        auto unc = fb->get_data();//decompress(std::get<2>(*fb),std::get<3>(*fb));
        
        return read_waynodes_block(unc,minway,maxway);
    };
    read_blocks_convfunc<way_nodes>(fn, mwns, locs, 4, read_waynode_func);
}


class WayNodesFileImpl : public WayNodesFile {
    public:
        WayNodesFileImpl(const std::string& fn_, const std::vector<int64>& ll_) : fn(fn_), ll(ll_) {
           
        }
        virtual ~WayNodesFileImpl() {}
        virtual void read_waynodes(std::function<void(std::shared_ptr<way_nodes>)> cb, int64 minway, int64 maxway) {
            read_merged_waynodes(fn, ll, cb, minway, maxway);
        }
        
        virtual void remove_file() {
            std::remove(fn.c_str());
        }
    private:
        std::string fn;
        std::vector<int64> ll;
};
        
class WayNodesFilePrep {
    public:
        WayNodesFilePrep(const std::string& fn_) : fn(fn_) {}
        
        std::shared_ptr<WayNodesFile> make_waynodesfile() {
            logger_message() << "make_waynodesfile fn=" << fn << ", ll.size()=" << ll.size();
            return std::make_shared<WayNodesFileImpl>(fn, ll);
        }
        
        
        //std::vector<minimalblock_callback> make_writewaynodes(std::shared_ptr<calculate_relations> rels, bool resort, size_t numchan) {
        minimalblock_callback make_writewaynodes(std::shared_ptr<calculate_relations> rels, bool sortinmem, size_t numchan) {
            if (sortinmem) {
                waynodes_writer_obj = make_pbffilewriter_indexedinmem(fn, nullptr);
            } else {
                waynodes_writer_obj = make_pbffilewriter(fn,nullptr,true);
            }
                
            //auto add_relations = threaded_callback<way_nodes>::make([rels](std::shared_ptr<way_nodes> r) {
            minimalblock_callback add_relations = [rels](std::shared_ptr<minimalblock> r) {
                if (r) {
                    rels->add_relations_data(r);
                }
            };
                        
            auto wwo = waynodes_writer_obj;
            typedef std::function<void(keystring_ptr)> keystring_callback;
            keystring_callback write_waynodes = [wwo](keystring_ptr k) {
                if (k) {
                    wwo->writeBlock(k);
                }
            };
            
            
            if (numchan!=0) {
                add_relations = threaded_callback<minimalblock>::make(add_relations,numchan);
                write_waynodes = threaded_callback<keystring>::make(write_waynodes,numchan);
            }
            
            size_t block_size = 1<<12;//(sortinmem ? 12 : 16);
            int64 split_at = 1<<20;
            int comp_level=1;
            
            std::vector<minimalblock_callback> pack_waynodes;
            if (numchan==0) {
                pack_waynodes.push_back(make_write_waynodes_callback(write_waynodes, add_relations, block_size,split_at,comp_level));
            } else {
            
                for (size_t i=0; i < numchan; i++) {
                    pack_waynodes.push_back(threaded_callback<minimalblock>::make(
                        
                        make_write_waynodes_callback(write_waynodes, add_relations, block_size,split_at,comp_level)
                    )
                    );
                }
            }
            
            node_blocks.reserve(800000);
            return [pack_waynodes,this](std::shared_ptr<minimalblock> bl) {
                if (!bl) {
                    for (auto pw: pack_waynodes) { pw(bl); }
                } else {
                    
                    logger_progress(bl->file_progress) << "writing way nodes";
                    
                    if (bl->has_nodes) { node_blocks.push_back(bl->file_position); }
                    pack_waynodes[bl->index % pack_waynodes.size()](bl);
                }
            };
            
            
        }
        
        std::pair<std::string,std::vector<int64>> finish_writewaynodes() {
            if (!waynodes_writer_obj) {
                throw std::domain_error("waynodes not written???");
            }
            block_index idx = waynodes_writer_obj->finish();
            ll.reserve(idx.size());
            
            sort_block_index(idx);
            for (const auto& l: idx) {
                ll.push_back(std::get<1>(l));
            }
            waynodes_writer_obj.reset();
            return std::make_pair(nodefn,std::move(node_blocks));
        }
        
        
        
    private:
        std::string fn;
        bool writelocs;
        std::string nodefn;
        std::vector<int64> ll;
        std::vector<int64> node_blocks;
        std::shared_ptr<PbfFileWriter> waynodes_writer_obj;
};


std::tuple<std::shared_ptr<WayNodesFile>,std::shared_ptr<calculate_relations>,std::string,std::vector<int64>>
    write_waynodes(const std::string& orig_fn, const std::string& waynodes_fn, size_t numchan, bool sortinmem, std::shared_ptr<logger> lg) {
    
    auto rels = make_calculate_relations();
    auto waynodes = std::make_shared<WayNodesFilePrep>(waynodes_fn);
        
    auto pack_waynodes= waynodes->make_writewaynodes(rels, sortinmem, numchan);
        
    
    if (numchan!=0) {
        read_blocks_minimalblock(orig_fn, pack_waynodes, {}, 4, 6 | 48);
    } else {
        read_blocks_nothread_minimalblock(orig_fn, pack_waynodes, {}, 6 | 48);
    }
        
    
    logger_message() << rels->str();
    lg->time("wrote waynodes");
    
    auto res = waynodes->finish_writewaynodes();
    logger_message() << "have " << res.second.size() << " blocks with nodes";
    
    lg->time("finished waynodes");    
    
    return std::make_tuple(waynodes->make_waynodesfile(),rels,orig_fn,res.second);
}
}
   
