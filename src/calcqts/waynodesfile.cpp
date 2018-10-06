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
 
#include "oqt/calcqts/waynodesfile.hpp"
#include "oqt/utils/logger.hpp"
#include "oqt/pbfformat/fileblock.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
namespace oqt {
    

class MergeWayNodes {
    public:
        typedef std::function<void(std::shared_ptr<WayNodes>)> callback;
        MergeWayNodes(callback cb_) : cb(cb_), key(-1) {
            
        }
        void finish_temp() {
            if (temp.empty()) { return; }
            
            size_t sz=0;
            for (const auto& t: temp) {
                sz+=t->size();
            }
            
            
            auto curr=make_way_nodes_write(sz, key);
            
            
            for (auto& t: temp) {
                
                for (size_t i=0; i < t->size(); i++) {
                    int64 w = t->way_at(i);
                    int64 n = t->node_at(i);
                    curr->add(w,n, true);
                }
                t.reset();
            }
            curr->sort_node();
            
            cb(curr);
            std::vector<std::shared_ptr<WayNodes>> x;
            temp.swap(x);
        }
            
            
        
        
        void call(std::shared_ptr<WayNodes> wns) {
            if (!wns) {
                finish_temp();
                                
                
                cb(nullptr);
                return;
            }
            if (wns->key() != key) {
                finish_temp();
                key=wns->key();
                
                
            }
            
            temp.push_back(wns);
            
            
            
        }
    private:
        
        
        callback cb;
        
        std::vector<std::shared_ptr<WayNodes>> temp;
        int64 key;
        
        
};
void read_merged_waynodes(const std::string& fn, const std::vector<int64>& locs, std::function<void(std::shared_ptr<WayNodes>)> cb, int64 minway, int64 maxway) {
    
    auto merged_waynodes = std::make_shared<MergeWayNodes>(cb);
    
    auto mwns = [merged_waynodes](std::shared_ptr<WayNodes> s) { merged_waynodes->call(s); };

    auto read_waynode_func = [minway,maxway](std::shared_ptr<FileBlock> fb) -> std::shared_ptr<WayNodes> {
        if (!fb) { 
            return nullptr;
        }
        if (fb->blocktype!="WayNodes") {
            Logger::Message() << "?? " << fb->idx << " " << fb->blocktype;
            throw std::domain_error("not a waynodes blocks");
        }
        auto unc = fb->get_data();//decompress(std::get<2>(*fb),std::get<3>(*fb));
        
        return read_waynodes_block(unc,minway,maxway);
    };
    read_blocks_convfunc<WayNodes>(fn, mwns, locs, 4, read_waynode_func);
}


class WayNodesFileImpl : public WayNodesFile {
    public:
        WayNodesFileImpl(const std::string& fn_, const std::vector<int64>& ll_) : fn(fn_), ll(ll_) {
           
        }
        virtual ~WayNodesFileImpl() {}
        virtual void read_waynodes(std::function<void(std::shared_ptr<WayNodes>)> cb, int64 minway, int64 maxway) {
            read_merged_waynodes(fn, ll, cb, minway, maxway);
        }
        
        virtual void remove_file() {
            std::remove(fn.c_str());
        }
    private:
        std::string fn;
        std::vector<int64> ll;
};

std::shared_ptr<WayNodesFile> make_waynodesfile(const std::string& fn, const std::vector<int64>& ll) {
    return std::make_shared<WayNodesFileImpl>(fn, ll);
}

}
