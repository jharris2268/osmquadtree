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

#ifndef PBFFORMAT_READBLOCK_HPP
#define PBFFORMAT_READBLOCK_HPP

#include "oqt/common.hpp"
#include "oqt/pbfformat/idset.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/elements/header.hpp"
#include "oqt/elements/quadtree.hpp"
#include "oqt/utils/pbf/protobuf.hpp"
namespace oqt {
    
enum class ReadBlockFlags {
    Empty = 0,
    SkipNodes = 1,
    SkipWays = 2,
    SkipRelations = 4,
    SkipGeometries = 8,
    SkipStrings = 16,
    SkipInfo = 32,
    UseAlternative = 64
};
inline ReadBlockFlags operator |(ReadBlockFlags lhs, ReadBlockFlags rhs)  
{
    return static_cast<ReadBlockFlags> (
        static_cast<std::underlying_type<ReadBlockFlags>::type>(lhs) |
        static_cast<std::underlying_type<ReadBlockFlags>::type>(rhs)
    );
}

inline bool has_flag(ReadBlockFlags lhs, ReadBlockFlags rhs) {
    auto l = static_cast<std::underlying_type<ReadBlockFlags>::type>(lhs);
    auto r = static_cast<std::underlying_type<ReadBlockFlags>::type>(rhs);
    return (l&r) == r;
}

/*inline ReadBlockFlags operator &(ReadBlockFlags lhs, ReadBlockFlags rhs)  
{
    return static_cast<ReadBlockFlags> (
        static_cast<std::underlying_type<ReadBlockFlags>::type>(lhs) &
        static_cast<std::underlying_type<ReadBlockFlags>::type>(rhs)
    );
}*/

typedef std::function<ElementPtr(ElementType, const std::string&, const std::vector<std::string>&, changetype)> read_geometry_func;
PrimitiveBlockPtr read_primitive_block(int64 idx, const std::string& data, bool change,
    ReadBlockFlags flags=ReadBlockFlags::Empty, IdSetPtr ids=IdSetPtr(),
    read_geometry_func readGeometry = read_geometry_func());

std::tuple<int64,ElementInfo,std::vector<Tag>,int64,std::list<PbfTag> >
    read_common(ElementType ty, const std::string& data, const std::vector<std::string>& stringtable, IdSetPtr ids);

std::vector<std::string> read_string_table(const std::string& data);
int64 read_quadtree(const std::string& data);


HeaderPtr read_header_block(const std::string& data, int64 fl, const std::string& filename);
void read_filelocs_json(HeaderPtr head, int64 fl, const std::string& filename, const std::string& filelocssuffix);


PrimitiveBlockPtr read_primitive_block_new(int64 idx, const std::string& data, bool change,
    ReadBlockFlags flags=ReadBlockFlags::Empty, IdSetPtr ids=IdSetPtr(),
    read_geometry_func readGeometry = read_geometry_func());

void read_primitive_block_new_into(
    PrimitiveBlockPtr primblock,
    const std::string& data, bool change,
    ReadBlockFlags flags=ReadBlockFlags::Empty, IdSetPtr ids=IdSetPtr(),
    read_geometry_func readGeometry = read_geometry_func());

}
#endif //PBFFORMAT_READBLOCK_HPP
