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

#include "oqt/utils/geometry.hpp"


namespace oqt {
    


bool point_in_poly(const std::vector<lonlat>& verts, const lonlat& test) {
    /*
       from http://www.ecse.rpi.edu/~wrf/Research/Short_Notes/pnpoly.html

       Copyright (c) 1970-2003, Wm. Randolph Franklin

       Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

           1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimers.
           2. Redistributions in binary form must reproduce the above copyright notice in the documentation and/or other materials provided with the distribution.
           3. The name of W. Randolph Franklin may not be used to endorse or promote products derived from this Software without specific prior written permission.

       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

       int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
       {
         int i, j, c = 0;
         for (i = 0, j = nvert-1; i < nvert; j = i++) {
           if ( ((verty[i]>testy) != (verty[j]>testy)) &&
         (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
              c = !c;
         }
         return c;
       }*/
    if (verts.size()<3) { return false; }
/*
def pnpoly(vertx, verty, testx, testy):
    j, c = len(vertx)-1,False
    for i in range(len(vertx)):
        if ( ((verty[i]>testy) != (verty[j]>testy)) and (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) ):
            c = not c
        j=i
    return c
*/
    size_t j = verts.size()-1;
    bool c= false;
    double testlon = test.lon, testlat = test.lat;
    for (size_t i=0; i < verts.size(); i++) {

        double loni = verts.at(i).lon, lati = verts.at(i).lat;
        double lonj = verts.at(j).lon, latj = verts.at(j).lat;

        if ( ((lati>testlat)!=(latj>testlat)) &&
            (testlon < (lonj - loni) * (testlat- lati) / (latj  - lati) + loni)) {
            c = !c;
        }

        j = i;
    }
    return c;
}

int segment_side(const lonlat& p1, const lonlat& p2, const lonlat& q) {
    int64 side = (q.lon - p1.lon) * (p2.lat - p1.lat) - (p2.lon - p1.lon) * (q.lat - p1.lat);
    if (side>0) { return 1; }
    if (side<0) { return -1; }
    return 0;
}
    

bool segment_intersects(const lonlat& p1, const lonlat& p2, const lonlat& q1, const lonlat& q2) {
    int pq1 = segment_side(p1,p2,q1);
    int pq2 = segment_side(p1,p2,q2);
    if (pq1==pq2) { return false; }
    
    int qp1 = segment_side(q1,q2,p1);
    int qp2 = segment_side(q1,q2,p2);
    if (qp1==qp2) { return false; }
    return true;
}
    
    
bool line_intersects(const lonlatvec& line1, const lonlatvec& line2) {
    if ((line1.size()<2) || (line2.size()<2)) { return false; }
    
    for (size_t i=0; i < (line1.size()-1); i++) {
        for (size_t j=0; i < (line2.size()-1); j++) {
            if (segment_intersects(line1[i],line1[i+1],line2[i],line2[j+1])) {
                return true;
            }
        }
    }
    return false;
}
    
    
bool line_box_intersects(const lonlatvec& line, const bbox& box) {
    if (line.size()<2) { return false; }
    
    lonlat a{box.minx,box.miny};
    lonlat b{box.maxx,box.miny};
    lonlat c{box.maxx,box.maxy};
    lonlat d{box.minx,box.maxy};
 
    
    for (size_t i=0; i < (line.size()-1); i++) {
        if (segment_intersects(line[i],line[i+1],a,b)) {
            return true;
        }
        if (segment_intersects(line[i],line[i+1],b,c)) {
            return true;
        }
        if (segment_intersects(line[i],line[i+1],c,d)) {
            return true;
        }
        if (segment_intersects(line[i],line[i+1],d,a)) {
            return true;
        }
        
        
    }
    return false;
}

bool polygon_box_intersects(const lonlatvec& line, const bbox& box) {
    if (line.size()<3) { return false; }
    if (box_empty(box)) { return false; }
    
    if (line_box_intersects(line,box)) { return true; }
    
    if (contains_point(box, line[0].lon, line[0].lat)) {
        return true;
    }
    
    if (point_in_poly(line, lonlat{box.minx,box.miny})) {
        return true;
    }
    
    return false;
}


    
}
