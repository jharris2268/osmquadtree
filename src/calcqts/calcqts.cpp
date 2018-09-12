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

#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/pbfformat/readminimal.hpp"
#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/pbfformat/writeblock.hpp"

#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/calcqts/writewaynodes.hpp"
#include "oqt/calcqts/qtstore.hpp"
#include "oqt/calcqts/qtstoresplit.hpp"
#include "oqt/calcqts/calcqts.hpp"
#include "oqt/calcqts/writeqts.hpp"

#include "oqt/calcqts/calculaterelations.hpp"
#include "oqt/calcqts/waynodes.hpp"
#include "oqt/calcqts/waynodesfile.hpp"

#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/pbf/packedint.hpp"
#include "oqt/utils/invertedcallback.hpp"

#include <algorithm>
#include <chrono>
namespace oqt {
    

class WaynodesQts {
    public:
        typedef std::function<void(std::shared_ptr<qtstore>)> callback;
        
        WaynodesQts(callback cb_, std::shared_ptr<qtstore> qts_)
            : qts(qts_), cb(cb_), nmissing(0) {}
        
        void call(std::shared_ptr<way_nodes> curr) {
            if (!curr) {
                logger_message() << "had " << nmissing << " missing ways";
                cb(nullptr);
                return;
            }
            if (curr->size()==0) {
                return;
            }
            
            
            int64 mn = curr->key() * (1<<20);
            int64 mx = mn + (1<<20);
            
            
            auto out = make_qtstore_arr(mn, mx, curr->key());
            
            for (size_t i=0; i < curr->size(); i++) {
                
                int64 way_ref = curr->way_at(i);
                auto q = qts->at(way_ref);
                if (q==-1) {
                    logger_message() << "missing way " << way_ref;
                    nmissing+=1;
                } else {
                    int64 node_ref = curr->node_at(i);
                    out->expand(node_ref, q);
                }
            }
            
            cb(out);               
            
        }
        
    private:
        
        std::shared_ptr<qtstore> qts;
        callback cb;
        size_t nmissing;
};



void write_qts_file(const std::string& qtsfn, const std::string& nodes_fn, size_t numchan,
    const std::vector<int64>& node_locs, std::shared_ptr<qtstore_split> way_qts,
    std::shared_ptr<WayNodesFile> wns, std::shared_ptr<calculate_relations> rels, double buffer, size_t max_depth) {
    
    
    auto node_qts = inverted_callback<qtstore>::make([wns,way_qts](std::function<void(std::shared_ptr<qtstore>)> cb) {
        auto nn = std::make_shared<WaynodesQts>(cb, way_qts);
        auto nncb = threaded_callback<way_nodes>::make([nn](std::shared_ptr<way_nodes> x) { nn->call(x); });
        wns->read_waynodes(nncb, 0, 0);
    });
    
    auto out = make_collectqts(qtsfn, numchan, 8000);
    
    
    auto nq = node_qts();
    int64 nql = nq->ref_range().second;
    
    auto calc_node_qts = threaded_callback<minimalblock>::make([rels,out,buffer,max_depth](std::shared_ptr<minimalblock> nds) {
        if (!nds) { return; }
        for (auto& n : nds->nodes) {
            if (n.quadtree==-1) {
                n.quadtree = quadtree::calculate(n.lon,n.lat,n.lon,n.lat,buffer, max_depth);
            }
            out->add(0,n.id,n.quadtree);
        }
        rels->add_nodes(nds);
    });
            
            
    
    auto add_nodeway_qts = [&nq,&nql,node_qts,&calc_node_qts](std::shared_ptr<minimalblock> nds) {
        if (!nds) {

            calc_node_qts(nullptr);
            return;
        }
        logger_progress(nds->file_progress) << "calculate node quadtrees";
        for (auto& n : nds->nodes) {

            while (nq && (n.id >= nql)) {
                nq=node_qts();
                if (nq) {
                    nql = nq->ref_range().second;
                } else {
                    nql = -1;
                }
            }
            int64 q=-1;
            if (nq) { q = nq->at(n.id); }
            n.quadtree=q;
        }
        calc_node_qts(nds);
    };
    read_blocks_minimalblock(nodes_fn, add_nodeway_qts, node_locs, 4, 1 | 48);
    
    
    auto lg=get_logger();
    
    lg->time("calculate node qts");            
    rels->add_ways(way_qts);
    lg->time("added rel way qts");
    for (size_t ti = 0; ti < way_qts->num_tiles(); ti++) {
        auto wq = way_qts->tile(ti);
        if (!wq) { continue; }
        
        int64 f = wq->first();
        logger_progress(100.0*ti/way_qts->num_tiles()) << "write ways tile " << wq->key() << " first " << f;
        
        while (f>0 ) {
            out->add(1, f, wq->at(f));
            f= wq->next(f);
        }
    }
    lg->time("wrote way qts");
    
    rels->finish_alt([out](int64 i, int64 q) { out->add(2,i,q); });
    
    
    out->finish();
    
    lg->time("wrote relation qts");
    
    
}



int run_calcqts(const std::string& origfn, const std::string& qtsfn, size_t numchan, bool splitways, bool resort, double buffer, size_t max_depth) {
    
    auto lg=get_logger();
    std::string waynodes_fn = qtsfn+"-waynodes";
    
    std::shared_ptr<calculate_relations> rels;
    std::shared_ptr<WayNodesFile> wns;
     
    std::string nodes_fn;
    std::vector<int64> node_locs;
    std::tie(wns,rels,nodes_fn,node_locs) = write_waynodes(origfn, waynodes_fn, 1/*numchan*/, resort, lg);    
    
    
    
    
    logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
   // logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
    //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
    
    std::shared_ptr<qtstore_split> way_qts = make_qtstore_split(1<<20,true);
    
    
    
    if (splitways) {
        int64 midway = 320<<20;
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, midway); //find_way_quadtrees_test
        
        logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        //logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
        //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, midway, 0);
        
        logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        //logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
        //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        
    } else {
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, 0);
        logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
        //logger_message() << "trim ? " << (trim_memory() ? "yes" : "no");
        //logger_message() << "[RSS = " << getmemval(getpid())/1024/1024 << "mb]";
    }
    
    
    write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth);
    return 1;
}



}
    
    