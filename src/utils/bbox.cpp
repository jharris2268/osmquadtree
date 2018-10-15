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

#include "oqt/utils/bbox.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
namespace oqt {




void expand_point(bbox& box, int64 x, int64 y) {
    if (x<box.minx) { box.minx=x; }
    if (y<box.miny) { box.miny=y; }
    if (x>box.maxx) { box.maxx=x; }
    if (y>box.maxy) { box.maxy=y; }
}


std::ostream& operator<<(std::ostream& os, const bbox& bb) {
    os  << "box{" << std::setw(10) << bb.minx
        << ", "   << std::setw(10) << bb.miny
        << ", "   << std::setw(10) << bb.maxx
        << ", "   << std::setw(10) << bb.maxy
        << "}";
    return os;
}


/*
const double earth_half_circum = 20037508.3428;


//Transform lon lat into spherical mercartor coordinate
func Mercator(ln float64, lt float64) (float64, float64) {
    return ln * earth_half_circum / 180.0, merc(lt) * earth_half_circum / 90.0
}

//Transform spherical mercartor to lon lat
func UnMercator(x, y float64) (float64, float64) {
    return x * 180.0 / earth_half_circum, unMerc(y * 90.0 / earth_half_circum)
}
*/

bool contains_point(const bbox& bb, int64 ln, int64 lt) {
    return (ln >= bb.minx) && (lt>=bb.miny) && (ln <= bb.maxx) && (lt <= bb.maxy);
}
bool overlaps(const bbox& l, const bbox& r) {
    if (l.minx > r.maxx) { return false; }
    if (l.miny > r.maxy) { return false; }
    if (r.minx > l.maxx) { return false; }
    if (r.miny > l.maxy) { return false; }
    return true;
}



bool box_empty(const bbox& b) {
    return (b.minx>b.maxx) || (b.miny>b.maxy);
}


bool box_planet(const bbox& b) {
    if (b.minx > -1800000000) { return false; }
    if (b.miny > -900000000) { return false; }
    if (b.maxx < 1800000000) { return false; }
    if (b.maxy < 900000000) { return false; }
    return true;
}

bool bbox_contains(const bbox& l, const bbox& r) {
    return (l.minx <= r.minx) && (l.miny <= r.miny) && (l.maxx>=r.maxx) && (l.maxy >= r.maxy);
}

void bbox_expand(bbox& l, const bbox& r) {
    if (r.minx< l.minx) { l.minx=r.minx; }
    if (r.miny< l.miny) { l.miny=r.miny; }
    if (r.maxx> l.maxx) { l.maxx=r.maxx; }
    if (r.maxy> l.maxy) { l.maxy=r.maxy; }
}


}
