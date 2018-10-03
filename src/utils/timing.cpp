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

#include "oqt/utils/timing.hpp"
#include <iostream>
#include <iomanip>

namespace oqt {


TimeSingle::TimeSingle() : st(std::chrono::high_resolution_clock::now()) {}
double TimeSingle::since() const {
    auto et = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(et-st).count()*0.001;
}

void TimeSingle::reset() {
    st=std::chrono::high_resolution_clock::now();
}
double TimeSingle::since_reset() {
    auto et = std::chrono::high_resolution_clock::now();
    auto rs = std::chrono::duration_cast<std::chrono::milliseconds>(et-st).count()*0.001;
    st=et;
    return rs;
}
    

std::ostream& operator<<(std::ostream& os, const TmStr& t) {
    os << std::fixed << std::setprecision(t.precision) << std::setw(t.width) << t.time << "s";
    return os;
}

std::ostream& operator<<(std::ostream& os, const TimeSingle& ts) {
    os << TmStr{ts.since(), 6, 1};
    return os;
}


std::ostream& operator<<(std::ostream& os, const TmMsg& t) {
    double ta = std::chrono::duration_cast<std::chrono::milliseconds>(t.et-t.st).count()*0.001;
    os << std::left << std::setw(t.ln+1) << (t.msg+":");
    os << std::right << std::setw(8) << std::fixed << std::setprecision(1) << ta << "s. ";
    return os;
}
    
    
    
    
}
