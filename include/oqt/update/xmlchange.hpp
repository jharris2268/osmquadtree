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

#ifndef UPDATE_XMLCHANGE_HPP
#define UPDATE_XMLCHANGE_HPP

#include "oqt/elements/block.hpp"
#include <map>

namespace oqt {
PrimitiveBlockPtr read_xml_change(const std::string& data);
PrimitiveBlockPtr read_xml_change_file(std::istream* strm);


typedef std::map<std::pair<ElementType,int64>, ElementPtr> typeid_element_map;
typedef std::shared_ptr<typeid_element_map> typeid_element_map_ptr;

void read_xml_change_file_em(std::istream* fl, typeid_element_map_ptr em, bool allow_missing_users);
void read_xml_change_em(const std::string& data, typeid_element_map_ptr em, bool allow_missing_users);
}
#endif
