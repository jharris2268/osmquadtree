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

#ifndef UTILS_HPP
#define UTILS_HPP




#include "oqt/simplepbf.hpp"


#include "oqt/utils/logger.hpp"

#include "oqt/utils/compress.hpp"
#include "oqt/utils/date.hpp"

#include "oqt/utils/singlequeue.hpp"

#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/splitcallback.hpp"
#include "oqt/utils/invertedcallback.hpp"

#include "oqt/utils/geometry.hpp"
#include "oqt/utils/operatingsystem.hpp"

namespace oqt {
inline bool EndsWith(const std::string& a, const std::string& b) {
    if (b.size() > a.size()) return false;
    return std::equal(a.begin() + a.size() - b.size(), a.end(), b.begin());
}

    




}
#endif //UTILS_HPP
