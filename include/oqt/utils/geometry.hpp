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

#ifndef UTILS_GEOMETRY_HPP
#define UTILS_GEOMETRY_HPP

#include "oqt/common.hpp"
#include "oqt/utils/bbox.hpp"
#include <cmath>



namespace oqt {




inline int64 coordinate_as_integer(double v) {
    if (v>0) {
        return (v*10000000)+0.5;
    }
    return (v*10000000)-0.5;
}

inline double coordinate_as_float(int64 v) {
    return ((double)v) * 0.0000001;
}
inline double latitude_mercator(double y, double scale=90.0) {
    return log(tan(M_PI*(1.0+y/90.0)/4.0)) * scale / M_PI;
}

inline double latitude_un_mercator(double d, double scale=90.0) {
    return (atan(exp(d*M_PI/scale))*4/M_PI - 1.0) * 90.0;
}

static const double earth_width = 20037508.342789244;




struct LonLat {
    LonLat() : lon(0), lat(0) {}
    LonLat(int64 lon_,int64 lat_) : lon(lon_), lat(lat_) {}
    int64 lon, lat;
};


bool point_in_poly(const std::vector<LonLat>& poly, const LonLat& test);

bool segment_intersects(const LonLat& p1, const LonLat& p2, const LonLat& q1, const LonLat& q2);
bool line_intersects(const std::vector<LonLat>& line1, const std::vector<LonLat>& line2);
bool line_box_intersects(const std::vector<LonLat>& line, const bbox& box);
bool polygon_box_intersects(const std::vector<LonLat>& line, const bbox& box);
}
#endif
