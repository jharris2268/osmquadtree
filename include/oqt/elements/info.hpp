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

#ifndef ELEMENTS_INFO_HPP
#define ELEMENTS_INFO_HPP


#include "oqt/common.hpp"

#include <string>
namespace oqt {
struct info {
    info() : version(0),timestamp(0),changeset(0),user_id(0),user(""),visible(false) {}
    info(int64 vs, int64 ts, int64 cs, int64 ui, std::string us, bool vis=true) :
        version(vs),timestamp(ts),changeset(cs),user_id(ui),user(us),visible(vis) {}

    int64 version;
    int64 timestamp;
    int64 changeset;
    int64 user_id;
    std::string user;
    bool visible;
};
}
#endif
