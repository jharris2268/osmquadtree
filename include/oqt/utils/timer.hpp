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

#ifndef UTILS_TIMER_HPP
#define UTILS_TIMER_HPP

#include "oqt/common.hpp"

#include <sstream>
#include <chrono>
#include <iomanip>

namespace oqt {


template <class OFS, class T>
OFS& printtime(OFS& strm, const std::string& msg, size_t ln, const T& st, const T& et) {
    double ta = std::chrono::duration_cast<std::chrono::milliseconds>(et-st).count()*0.001;
    strm << std::left << std::setw(ln+1) << (msg+":");
    strm << std::right << std::setw(8) << std::fixed << std::setprecision(1) << ta << "s. ";
    return strm;
}

struct time_single {
    std::chrono::high_resolution_clock::time_point st;
    
    time_single() : st(std::chrono::high_resolution_clock::now()) {}
    double since() const {
        auto et = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(et-st).count()*0.001;
    }
    
    void reset() {
        st=std::chrono::high_resolution_clock::now();
    }
    double since_reset() {
        auto et = std::chrono::high_resolution_clock::now();
        auto rs = std::chrono::duration_cast<std::chrono::milliseconds>(et-st).count()*0.001;
        st=et;
        return rs;
    }
        
};

struct TmStr {
    double time;
    int width;
    int precision;
};
inline std::ostream& operator<<(std::ostream& os, const TmStr& t) {
    os << std::fixed << std::setprecision(t.precision) << std::setw(t.width) << t.time << "s";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const time_single& ts) {
    os << TmStr{ts.since(), 6, 1};
    return os;
}


class timing {

    typename std::vector<std::pair<std::string,std::chrono::high_resolution_clock::time_point>> msgs;
    size_t ln;

    public:
        timing() : ln(0) {
            msg("");
        }
        void msg(std::string m) {
            msgs.push_back(std::make_pair(m, std::chrono::high_resolution_clock::now()));
            if (m.size()>ln) { ln = m.size(); }
        }

        template <class OFS>
        OFS& print(OFS& strm, size_t i) {
            if (i==(msgs.size()-1)) { return printtime(strm, "TOTAL", ln, msgs.front().second, msgs.back().second); }
            return printtime(strm, msgs[i+1].first, ln, msgs[i].second, msgs[i+1].second);

        }

        template <class OFS>
        OFS& print_all(OFS& strm) {
            for (size_t i=0; i < size(); i++) {
                print(strm,i) << std::endl;
            }
            return strm;
        }

        size_t size() { return msgs.size(); }
};



}






#endif //UTILS_TIMER_HPP
