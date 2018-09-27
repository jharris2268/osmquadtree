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

#ifndef POSTGISWRITER_HPP
#define POSTGISWRITER_HPP

#include "oqt/elements/block.hpp"
namespace oqt {
namespace geometry {



struct csv_rows {
    std::string data;
    std::vector<size_t> poses;
    
    void add(const std::string row);
    std::string at(int i);
    int size();
};

struct csv_block {
    csv_rows points;
    csv_rows lines;
    csv_rows polygons;
};

class pack_csvblocks {
    public:
        typedef std::vector<std::tuple<std::string,bool,bool,bool>> tagspec;
        virtual std::shared_ptr<csv_block> call(PrimitiveBlockPtr bl) = 0;
        virtual ~pack_csvblocks() {}
};

std::shared_ptr<pack_csvblocks> make_pack_csvblocks(const pack_csvblocks::tagspec& tags, bool with_header);

class PostgisWriter {
    public:
        
        virtual void finish()=0;
        virtual void call(std::shared_ptr<csv_block> bl)=0;
        virtual ~PostgisWriter() {}
};

std::shared_ptr<PostgisWriter> make_postgiswriter(
    const std::string& connection_string,
    const std::string& table_prfx,
    bool with_header);

std::function<void(std::shared_ptr<csv_block>)> make_postgiswriter_callback(
    const std::string& connection_string,
    const std::string& table_prfx,
    bool with_header);

}}  
#endif 
