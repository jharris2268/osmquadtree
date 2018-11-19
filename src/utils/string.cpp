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

#include "oqt/utils/string.hpp"
#include <iostream>
#include <iomanip>

namespace oqt {

bool ends_with(const std::string& a, const std::string& b) {
    if (b.size() > a.size()) return false;
    return std::equal(a.begin() + a.size() - b.size(), a.end(), b.begin());
}    


std::ostream& operator<<(std::ostream& os, const Percent& p) {
    if (p.precision==0) {
        os << std::setw(4) << (int) p.val << "%";
        return os;
    }
    os << std::fixed << std::setprecision(p.precision) << std::setw(p.precision + 5) << p.val << "%";
    return os;
}

std::ostream& operator<<(std::ostream& os, const TmStr& t) {
    os << std::fixed << std::setprecision(t.precision) << std::setw(t.width) << t.time << "s";
    return os;
}
std::ostream& operator<<(std::ostream& os, const Mb& p) {
    os << std::fixed << std::setprecision(p.precision) << std::setw(p.precision+8) << (p.val/1024.0/1024) << "mb";
    return os;
}

}
