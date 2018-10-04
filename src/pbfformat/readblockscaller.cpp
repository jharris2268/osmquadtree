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
 
 
 
#include "oqt/pbfformat/readblockscaller.hpp"

#include "oqt/utils/logger.hpp"
#include "oqt/utils/date.hpp"

#include "picojson.h"
#include <fstream>
    
#include "oqt/pbfformat/writepbffile.hpp"

#include "oqt/pbfformat/readfileblocks.hpp"
        
namespace oqt {
std::pair<std::vector<std::string>,int64> read_filenames(const std::string& prfx, int64 enddate) {
    std::ifstream filelist(prfx+"filelist.json", std::ios::in);
    if (!filelist.good()) {
        throw std::domain_error("?? "+prfx+"filelist.json");
    }


    picojson::value v;
    filelist >> v;
    std::string err = picojson::get_last_error();

    if (! err.empty()) {
      logger_message() << "filelist errors: " << err;
    }
    if (!v.is<picojson::array>()) {
        throw std::domain_error("not an array");
    }
    auto varr = v.get<picojson::array>();
    logger_message() << "filelist: " << varr.size() << "entries";

    std::vector<std::string> result;
    int64 last_date=0;
    
    for (auto& vlv : varr) {

        auto vl = vlv.get<picojson::object>();
        std::string vl_date_in = vl["EndDate"].get<std::string>();
        int64 vl_date=read_date(vl_date_in);
        if (vl_date==0) {
            throw std::domain_error("can't parse "+vl_date_in);
        }
        if ((enddate>0) && (vl_date > enddate)) {
            logger_message() << "skip entry for " << vl_date_in << "(" << vl_date << ">" << enddate;
            continue;
        }
        
        std::string fn = prfx+vl["Filename"].get<std::string>();
        if (vl_date>last_date) {
            last_date=vl_date;
        }
        result.push_back(fn);
    }
    
    return std::make_pair(result, last_date);
}



class ReadBlocksSingle : public ReadBlocksCaller {
    public:
        ReadBlocksSingle(const std::string& fn_, bbox filter_box, const lonlatvec& poly) : fn(fn_) {
            if (!box_empty(filter_box)) {
                auto head = getHeaderBlock(fn);
                if (head && (!head->Index().empty())) {
                    for (const auto& l : head->Index()) {
                        if (overlaps_quadtree(filter_box,std::get<0>(l))) {
                            if (poly.empty() || polygon_box_intersects(poly, quadtree::bbox(std::get<0>(l), 0.05))) {
                            
                            
                                locs.push_back(std::get<1>(l));
                            }
                        }
                    }
                    if (locs.empty()) {
                        throw std::domain_error("no tiles match filter box");
                    }
                }
            }
        }
        void read_primitive(std::vector<primitiveblock_callback> cbs, IdSetPtr filter) {
            read_blocks_split_primitiveblock(fn, cbs, locs, filter, false, 15);
        }
        
        void read_minimal(std::vector<minimalblock_callback> cbs, IdSetPtr filter)  {
            read_blocks_split_minimalblock(fn, cbs, locs, 15);
        }
        
        size_t num_tiles() { return locs.size(); }
    private:
        std::string fn;
        std::vector<int64> locs;
};
        
class ReadBlocksMerged : public ReadBlocksCaller {
    public:
        ReadBlocksMerged(const std::string& prfx, bbox filter_box_, const lonlatvec& poly, int64 enddate_) : enddate(enddate_),filter_box(filter_box_)  {
            std::tie(filenames,enddate) = read_filenames(prfx, enddate);
            if (filenames.empty()) { throw std::domain_error("no filenames!"); }
            bbox top_box;
            bool empty_box = box_empty(filter_box);
            for (size_t file_idx=0; file_idx < filenames.size(); file_idx++) {
                const auto& fn = filenames.at(file_idx);
                auto head = getHeaderBlock(fn);
                if (!head) { throw std::domain_error("file "+fn+" has no header"); }
                if (file_idx==0) { top_box=head->BBox(); }
                if (head->Index().empty()) { throw std::domain_error("file "+fn+" has no tile index"); }
                
                for (const auto& l : head->Index()) {
                    if (file_idx>0) {
                        if (locs.count(std::get<0>(l))>0) {
                            locs[std::get<0>(l)].push_back(std::make_pair(file_idx, std::get<1>(l)));
                        }
                    } else {
                        
                        if (empty_box || overlaps_quadtree(filter_box,std::get<0>(l))) {
                            if (poly.empty() || polygon_box_intersects(poly, quadtree::bbox(std::get<0>(l), 0.05))) {
                        
                                locs[std::get<0>(l)].push_back(std::make_pair(file_idx, std::get<1>(l)));
                            }
                        }
                    }
                }
                
            }
            if (box_empty(filter_box) || bbox_contains(filter_box, top_box)) {
                filter_box = top_box;
            }
            buffer = 1<<14;
            //buffer = 0;
            
        }
        
        void read_primitive(std::vector<primitiveblock_callback> cbs, IdSetPtr filter) {
            logger_message() << "ReadBlocksMerged::read_primitive";
            
            read_blocks_split_merge<PrimitiveBlock>(filenames, cbs, locs, filter, 7, buffer);
        }
        void read_minimal(std::vector<minimalblock_callback> cbs, IdSetPtr filter)  {
            read_blocks_split_merge<minimal::Block>(filenames, cbs, locs, filter, 7, buffer);
        }
        
        
        int64 actual_enddate() { return enddate; }
        bbox actual_filter_box() { return filter_box; }
        size_t num_tiles() { return locs.size(); }
        
    private:
        std::vector<std::string> filenames;
        src_locs_map locs;
        int64 enddate;
        bbox filter_box;
        size_t buffer;
};
        
std::shared_ptr<ReadBlocksCaller> make_read_blocks_caller(
        const std::string& infile_name, 
        bbox& filter_box, const lonlatvec& poly, int64& enddate) {
   
    if (EndsWith(infile_name, ".pbf")) {
        auto hh = getHeaderBlock(infile_name);
        if (box_empty(filter_box) || (!box_empty(hh->BBox()) && bbox_contains(filter_box, hh->BBox()))) {
            filter_box = hh->BBox();
        }
        return std::make_shared<ReadBlocksSingle>(infile_name, filter_box, poly);
    }
    auto rs = std::make_shared<ReadBlocksMerged>(infile_name, filter_box, poly, enddate);
    enddate = rs->actual_enddate();
    filter_box = rs->actual_filter_box();
    return rs;
}

}
