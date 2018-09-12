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

#include "oqt/utils/date.hpp"
#include <ctime>

namespace oqt {


std::string date_str(int64 date) {
    time_t datet = time_t(date);

    tm * datetm = gmtime(&datet);
    std::string res(20,0);
    size_t l=strftime(&res[0],20,"%Y-%m-%dT%H:%M:%S",datetm);
    return res.substr(0,l);
}
int64 read_date(std::string in) {

    std::tm timeinfo={};
    if ((in.size()==20) && (in[19]=='Z')) {
        in=in.substr(0,19);
    }
    if (in.size()==8) {
        if(strptime(in.c_str(), "%Y%m%d", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        }
    } else if (in.size()==10) {

        if(strptime(in.c_str(), "%Y-%m-%d", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        }
    } else if (in.size()==19) {

        if (strptime(in.c_str(), "%Y-%m-%dT%H-%M-%S", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        } else if (
            strptime(in.c_str(), "%Y-%m-%dT%H:%M:%S", &timeinfo)!=NULL) {
            return mktime(&timeinfo);
        }
    }

    return 0;
}

    
}
