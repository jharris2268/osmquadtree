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

#ifndef UTILS_TIMING_HPP
#define UTILS_TIMING_HPP

#include "oqt/common.hpp"
#include "oqt/utils/string.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>

namespace oqt {



class TimeSingle {
    
    public:
        TimeSingle();
        double since() const;
        void reset();
        double since_reset();
    
    private:
        std::chrono::high_resolution_clock::time_point st;
        
};



std::ostream& operator<<(std::ostream& os, const TimeSingle& ts);



struct TmMsg {
    const std::string& msg;
    size_t ln;
    const std::chrono::high_resolution_clock::time_point& st;
    const std::chrono::high_resolution_clock::time_point& et;
};

std::ostream& operator<<(std::ostream& os, const TmMsg& t);


}






#endif //UTILS_TIMER_HPP
