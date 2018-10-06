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

#ifndef UTILS_LOGGER_HPP
#define UTILS_LOGGER_HPP

#include "oqt/common.hpp"

#include "oqt/utils/timing.hpp"


namespace oqt {
    

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
/*
class Logger;
std::shared_ptr<Logger> get_logger();
void set_logger(std::shared_ptr<Logger> nl);*/

class Logger {
    typename std::vector<std::pair<std::string,std::chrono::high_resolution_clock::time_point>> msgs;

    public:
        Logger() { time(""); }
        virtual void message(const std::string& msg)=0;
        virtual void progress(double percent, const std::string& msg)=0;


        static Logger& Get();
        static void Set(std::shared_ptr<Logger> l);


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
                    strm << TmMsg{"TOTAL", ln, msgs.front().second, msgs.back().second};
                } else {
                    strm << TmMsg{msgs[i+1].first, ln, msgs[i].second, msgs[i+1].second};
                    strm << "\n";
                }
            }
            message(strm.str());
        }
        virtual void reset_timing() {
            msgs.clear();
            time("");
        }

        virtual ~Logger() {}







        class Message {
            public:
                Message() {};
                
                Message(Message const&)=delete;
                Message(Message &&)=default;
                
                ~Message() {
                    Logger::Get().message(strm.str());
                }
                
                template <class T>
                Message& operator<<(const T& x) {
                    strm << x;
                    return *this;
                }
                
            private:
                std::stringstream strm;
        };

        class Progress {
            public:
                Progress(double pc_) : pc(pc_) {}
                Progress(Progress const&)=delete;
                Progress(Progress &&)=default;
                ~Progress() {
                    Logger::Get().progress(pc, strm.str());
                }
                
                template <class T>
                Progress& operator<<(const T& x) {
                    strm << x;
                    return *this;
                }
                
            private:
                std::stringstream strm;
                double pc;
        };

};

inline Logger::Message logger_message() { Logger::Message m; return std::move(m); }
inline Logger::Progress logger_progress(double pc) { Logger::Progress m(pc); return std::move(m); }
    
    
    
}
#endif //UTILS_LOGGER_HPP
