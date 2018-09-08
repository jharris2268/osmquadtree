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

#ifndef SORTING_COMMON_HPP
#define SORTING_COMMON_HPP


#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/calcqts/qttreegroups.hpp"



namespace oqt {

typedef std::pair<int64,std::string> keystring;
typedef std::vector<keystring> keystring_vec;
typedef std::function<void(std::shared_ptr<keystring_vec>)> writevec_callback;


}

#endif //SORTING_COMMON_HPP
