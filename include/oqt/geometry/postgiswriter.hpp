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

#ifndef GEOMETRY_POSTGISWRITER_HPP
#define GEOMETRY_POSTGISWRITER_HPP

#include "oqt/elements/block.hpp"
namespace oqt {
namespace geometry {



class CsvRows {
    
    public:
        CsvRows(bool is_binary_);
        
        bool is_binary() { return _is_binary; }
        
        void finish();
        void add(const std::string row);
        std::string at(int i) const;
        int size() const;
        
        const std::string data_blob() const { return data; }
        
    private:
        bool _is_binary;
        
        std::string data;
        std::vector<size_t> poses;
};

struct CsvBlock {
    
    CsvBlock(bool is_binary) : points(is_binary), lines(is_binary), polygons(is_binary) {}
    
    CsvRows points;
    CsvRows lines;
    CsvRows polygons;
};

class PackCsvBlocks {
    public:
        typedef std::vector<std::tuple<std::string,bool,bool,bool>> tagspec;
        virtual std::shared_ptr<CsvBlock> call(PrimitiveBlockPtr bl) = 0;
        virtual ~PackCsvBlocks() {}
};
std::string pack_hstoretags(const tagvector& tags);
std::string pack_jsontags_picojson(const tagvector& tags);

std::shared_ptr<PackCsvBlocks> make_pack_csvblocks(const PackCsvBlocks::tagspec& tags, bool with_header, bool binary_format);

class PostgisWriter {
    public:
        
        virtual void finish()=0;
        virtual void call(std::shared_ptr<CsvBlock> bl)=0;
        virtual ~PostgisWriter() {}
};

std::shared_ptr<PostgisWriter> make_postgiswriter(
    const std::string& connection_string,
    const std::string& table_prfx,
    bool with_header, bool binary_format);

std::function<void(std::shared_ptr<CsvBlock>)> make_postgiswriter_callback(
    const std::string& connection_string,
    const std::string& table_prfx,
    bool with_header, bool binary_format);

}}  
#endif 
