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



#include <zlib.h>
#include "oqt/simplepbf.hpp"

#include <sstream>
#include <chrono>
#include <iomanip>


#include "oqt/utils/compress.hpp"
#include "oqt/utils/date.hpp"

#include "oqt/utils/singlequeue.hpp"

#include "oqt/utils/threadedcallback.hpp"
#include "oqt/utils/multithreadedcallback.hpp"
#include "oqt/utils/splitcallback.hpp"
#include "oqt/utils/invertedcallback.hpp"


namespace oqt {
inline bool EndsWith(const std::string& a, const std::string& b) {
    if (b.size() > a.size()) return false;
    return std::equal(a.begin() + a.size() - b.size(), a.end(), b.begin());
}

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

struct Percent {
    double val;
    int precision;
};

inline std::ostream& operator<<(std::ostream& os, const Percent& p) {
    if (p.precision==0) {
        os << std::setw(4) << (int) p.val << "%";
        return os;
    }
    os << std::fixed << std::setprecision(p.precision) << std::setw(p.precision + 5) << p.val << "%";
    return os;
}

class logger {
    typename std::vector<std::pair<std::string,std::chrono::high_resolution_clock::time_point>> msgs;

    public:
        logger() { time(""); }
        virtual void message(const std::string& msg)=0;
        virtual void progress(double percent, const std::string& msg)=0;


        virtual void time(const std::string& msg) {
            msgs.push_back(std::make_pair(msg, std::chrono::high_resolution_clock::now()));
        }
        virtual void timing_messages() {
            size_t ln=5;
            for (auto& m : msgs) {
                if (m.first.size()>ln) { ln=m.first.size(); }
            }
            std::stringstream strm;
            for (size_t i=0; i < msgs.size(); i++) {
                if (i==(msgs.size()-1)) {
                    printtime(strm, "TOTAL", ln, msgs.front().second, msgs.back().second);
                } else {
                    printtime(strm, msgs[i+1].first, ln, msgs[i].second, msgs[i+1].second);
                    strm << "\n";
                }
            }
            message(strm.str());
        }
        virtual void reset_timing() {
            msgs.clear();
            time("");
        }

        virtual ~logger() {}
};

std::shared_ptr<logger> make_default_logger();

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


template <class ArgType, class FinishReturnType>
class CallFinish {
    public:
        virtual void call(std::shared_ptr<ArgType>)=0;
        virtual FinishReturnType finish()=0;
        virtual ~CallFinish() {}
};

        
int64 file_size(const std::string& fn);

struct lonlat {
    lonlat() : lon(0), lat(0) {}
    lonlat(int64 lon_,int64 lat_) : lon(lon_), lat(lat_) {}
    int64 lon, lat;
};
typedef std::vector<lonlat> lonlatvec;

bool point_in_poly(const lonlatvec& poly, const lonlat& test);

bool segment_intersects(const lonlat& p1, const lonlat& p2, const lonlat& q1, const lonlat& q2);
bool line_intersects(const lonlatvec& line1, const lonlatvec& line2);
bool line_box_intersects(const lonlatvec& line, const bbox& box);
bool polygon_box_intersects(const lonlatvec& line, const bbox& box);

void checkstats();
int64 getmemval(size_t pid);
std::string getmem(size_t pid);

size_t write_uint32(std::string& data, size_t pos, uint32_t i);
size_t write_uint32_le(std::string& data, size_t pos, uint32_t i);
size_t write_double(std::string& data, size_t pos, double d);
size_t write_double_le(std::string& data, size_t pos, double d);

uint32_t read_uint32_le(std::string& data, size_t pos);

bool trim_memory();


std::shared_ptr<logger> get_logger();
void set_logger(std::shared_ptr<logger> nl);

class logger_message {
    public:
        logger_message() {}
        
        ~logger_message() {
            get_logger()->message(strm.str());
        }
        
        template <class T>
        logger_message& operator<<(const T& x) {
            strm << x;
            return *this;
        }
        
    private:
        std::stringstream strm;
};

class logger_progress {
    public:
        logger_progress(double pc_) : pc(pc_) {}
        
        ~logger_progress() {
            get_logger()->progress(pc, strm.str());
        }
        
        template <class T>
        logger_progress& operator<<(const T& x) {
            strm << x;
            return *this;
        }
        
    private:
        std::stringstream strm;
        double pc;
};
}
#endif //UTILS_HPP
