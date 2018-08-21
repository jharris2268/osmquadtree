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

#include "oqt/quadtree.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
namespace oqt {

std::ostream& operator<<(std::ostream& os, const bbox& bb) {
    os  << "box{" << std::setw(10) << bb.minx
        << ", "   << std::setw(10) << bb.miny
        << ", "   << std::setw(10) << bb.maxx
        << ", "   << std::setw(10) << bb.maxy
        << "}";
    return os;
}

namespace quadtree {

int64 findQuad(double mx, double my, double Mx, double My, double bf) {
    if ((mx < (-1-bf)) || (my < (-1-bf)) || (Mx > (1+bf)) || (My > (1+bf))) {
        return -1;
    }

    if ((Mx <= 0) && (my >= 0)) {
        return 0;
    } else if ((mx >= 0) && (my >= 0)) {
        return 1;
    } else if ((Mx <= 0) && (My <= 0)) {
        return 2;
    } else if ((mx >= 0) && (My <= 0)) {
        return 3;

    } else if ((Mx < bf) && (fabs(Mx) < fabs(mx)) && (my > -bf) && (fabs(My) >= fabs(my))) {
        return 0;
    } else if ((mx > -bf) && (fabs(Mx) >= fabs(mx)) && (my > -bf) && (fabs(My) >= fabs(my))) {
        return 1;
    } else if ((Mx < bf) && (fabs(Mx) < fabs(mx)) && (My < bf) && (fabs(My) < fabs(my))) {
        return 2;
    } else if ((mx > -bf) && (fabs(Mx) >= fabs(mx)) && (My < bf) && (fabs(My) < fabs(my))) {
        return 3;
    }
    return -1;
}

int64 makeQuadTree_(double mx, double my, double Mx, double My, double bf, uint64 mxl, uint64 cl)  {

    if (mxl == 0) {
        return 0;
    }

    int64 q = findQuad(mx, my, Mx, My, bf);
    if (q == -1) {
        return 0;
    }
    if ((q == 0) || (q == 2)) {
        mx += 0.5;
        Mx += 0.5;
    } else {
        mx -= 0.5;
        Mx -= 0.5;
    }
    if ((q == 2) || (q == 3)) {
        my += 0.5;
        My += 0.5;
    } else {
        my -= 0.5;
        My -= 0.5;
    }
    return (q << (61 - 2*cl)) + 1 + makeQuadTree_(2*mx, 2*my, 2*Mx, 2*My, bf, mxl-1, cl+1);
}

int64 makeQuadTreeFloat(double mx, double my, double Mx, double My, double bf, uint64 mxl) {
    if ((mx > Mx) || (my > My)) {
        return -1;
    }
    if (Mx == mx) {
        Mx += 0.0000001;
    }
    if (My == my) {
        My += 0.0000001;
    }
    double mym = merc(my) / 90.0;
    double Mym = merc(My) / 90.0;
    double mxm = mx / 180.0;
    double Mxm = Mx / 180.0;

    return makeQuadTree_(mxm, mym, Mxm, Mym, bf, mxl, 0);
}


std::string string(int64 qt) {
    if (qt<=-1) {
        return "NULL";
    }

    uint64 l = qt & 31;
    std::string r(l,0);

    for (uint64 i=0; i < l; i++) {
        char v = (qt >> (61 - 2*i)) & 3;
        r[i]=(v+'A');
    }

    return r;
}

oqt::bbox bbox(int64 qt, double buffer) {

    double mx=-180.0, my=-90., Mx=180., My=90.;

    uint64 l = qt & 31;

    for (uint64 i=0; i < l; i++) {
        int64 v = (qt >> (61 - 2*i)) & 3;

        if ((v==0) || (v==2)) {
            Mx -= (Mx - mx) / 2;
        } else {
            mx += (Mx - mx) / 2;
        }
        if ((v==2) || (v==3)) {
            My -= (My - my) / 2;
        } else {
            my += (My - my) / 2;
        }

    }

    my = unMerc(my);
    My = unMerc(My);

    if (buffer > 0.0) {
        double xx = (Mx - mx) * buffer;
        double yy = (My - my) * buffer;
        mx -= xx;
        my -= yy;
        Mx += xx;
        My += yy;
    }

    return oqt::bbox{toInt(mx), toInt(my), toInt(Mx), toInt(My)};

}

xyz tuple(int64 qt) {
    uint64 z = qt & 31;
    int64 x=0, y=0;
    for (uint64 i = 0; i < z; i++) {
        x <<= 1;
        y <<= 1;
        int64 t = (qt >> (61-2*i)) & 3;
        if ((t & 1) == 1) {
            x |= 1;
        }
        if ((t & 2) == 2) {
            y |= 1;
        }
    }

    return xyz{x, y, (int64) z};
}


int64 round(int64 qt, uint64 level) {
    if ((qt&31) < level) {
        return qt;
    }
    qt >>= (63 - 2*level);
    qt <<= (63 - 2*level);
    return qt + level;
}

int64 common(int64 qt, int64 other) {
    if (qt == -1) {
        return other;
    } else if (other == -1) {
        return qt;
    } else if (qt == other) {
        return qt;
    }

    uint64 d = qt & 31;
    if ((other&31) < d) {
        d = other & 31;
    }
    int64 p = 0;

    for (uint64 i=0; i < d; i++) {
        int64 q = round(qt,i+1);
        if (q != round(other,i+1)) {
            return p;
        }
        p = q;
    }

    return p;
}

int64 calculate(int64 minx, int64 miny, int64 maxx, int64 maxy,
    double buffer, uint64 maxLevel) {
    return makeQuadTreeFloat(
        toFloat(minx), toFloat(miny),
        toFloat(maxx), toFloat(maxy),
        buffer, maxLevel);
}

int64 from_tuple(int64 x, int64 y, int64 z) {
    int64 ans =0;
    int64 scale = 1;
    for (size_t i=0; i < (size_t) z; i++) {
        ans += (  ((x>>i)&1)  | (((y>>i)&1)<<1))  * scale;
        scale *= 4;
    }

    ans <<= (63 - (2 * uint(z)));
    ans |= z;
    return ans;
}

int64 from_string(const std::string& str) {

    int64 ans = 0;
    size_t i=0;
    for (auto itr = str.begin(); itr < str.end(); ++itr) {
        int64 p = -1;

        switch (*itr) {
            case 'A': p=0; break;
            case 'B': p=1; break;
            case 'C': p=2; break;
            case 'D': p=3; break;
            default: return 0;
        }
        ans |= p<<(61-2*i );
        i++;
    }
    ans |= str.size();

    return ans;

}
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

bool overlaps_quadtree(const bbox& l, int64 qt) {
    bbox r = quadtree::bbox(qt,0.05);
    return overlaps(l,r);
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
