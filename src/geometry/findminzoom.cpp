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
 
#include "oqt/geometry/findminzoom.hpp"

#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include <map>

namespace oqt {
namespace geometry {


double res_zoom(double res) {
    if (std::abs(res)<0.001) { return 20; }
    return log(earth_width*2/res/256) / log(2.0);
}
        

int64 area_zoom(double area, double minarea) {
    double zm = res_zoom(std::sqrt(area/minarea));
    return (int64) zm;
}
int64 length_zoom(double length, double minlen) {
    return res_zoom(length/minlen);
}

       
class FindMinZoomOneTag : public FindMinZoom {
    public:
        typedef std::map<std::tuple<int64,std::string,std::string>,std::pair<int64,int64>> tagmap;
        
        FindMinZoomOneTag(const tag_spec& spec, double ml_, double ma_) :
            minlen(ml_), minarea(ma_) {
            
            for (const auto& t : spec) {
                int64 a,d; std::string b,c;
                std::tie(a,b,c,d) = t;
                tm.insert(std::make_pair(std::make_tuple(a,b,c),std::make_pair(d,0)));
                keys.insert(b);
            }
        }
        
        virtual ~FindMinZoomOneTag() {}
        
        
        virtual bool check_feature(ElementPtr ele, int64 min_max_zoom_level) {
            int mz = tags_zoom(ele);
            if (mz<0) {
                return false;
            }
            return mz <= min_max_zoom_level;
        }
        
        void check_tag(tagmap::iterator& curr_it, int64 ty, const Tag& t) {
        
            auto it = tm.find(std::make_tuple(ty,t.key,t.val));
            if (it!=tm.end()) {
                if ((curr_it==tm.end()) || (it->second.first < curr_it->second.first)) {
                    curr_it=it;
                }
            } else {
                it = tm.find(std::make_tuple(ty,t.key,"*"));
                if (it!=tm.end()) {
                    if ((curr_it==tm.end()) || (it->second.first < curr_it->second.first)) {
                        curr_it=it;
                    }
                }
            }
        }
        
        
        int64 tags_zoom(ElementPtr ele) {
            int64 ty = -1;
            if ((ele->Type()==ElementType::Node) || (ele->Type()==ElementType::Point)) {
                ty = 0;
            } else if (ele->Type()==ElementType::Linestring) {
                ty = 1;
            } else if ((ele->Type()==ElementType::SimplePolygon) || (ele->Type()==ElementType::ComplicatedPolygon)) {
                ty = 2;
            } else if (ele->Type()==ElementType::WayWithNodes) { 
                ty = 3;
            }
            
            
            if (ty==-1) { return -1; }
            
            auto curr_it = tm.end();
            
            for (const auto& t: ele->Tags()) {
                if (keys.count(t.key)>0) {
                    if (ty==3) {
                        check_tag(curr_it, 1,t);                        
                        check_tag(curr_it, 2,t);
                    } else {
                        check_tag(curr_it, ty,t);
                    }
                }
            }
                    
            if (curr_it==tm.end()) { return -1; }
            curr_it->second.second++;
            return curr_it->second.first;
        }
        
        virtual int64 calculate(ElementPtr ele) {
            //int64 minzoom=100;
            int64 minzoom = tags_zoom(ele);
            if (minzoom < 0) { return minzoom; }
        
            int64 area_minzoom = areaminzoom(ele);
            if (area_minzoom > minzoom) {
                return area_minzoom;
            }
            return minzoom;
        }
        
        int64 areaminzoom(ElementPtr ele) {
        
            
            if ((ele->Type()==ElementType::Linestring) && (minlen>0)) {
                auto ls=std::dynamic_pointer_cast<geometry::Linestring>(ele);
                if (!ls) { throw std::domain_error("not a Linestring"); }
                double len = ls->Length();
                return length_zoom(len, minlen);
            }
            if ((ele->Type()==ElementType::SimplePolygon) && (minarea>0)) {
                auto sp = std::dynamic_pointer_cast<geometry::SimplePolygon>(ele);
                if (!sp) { throw std::domain_error("not a SimplePolygon"); }
                double area = sp->Area();
                return area_zoom(area, minarea);
            }
            if ((ele->Type()==ElementType::ComplicatedPolygon) && (minarea>0)) {
                auto cp=std::dynamic_pointer_cast<geometry::ComplicatedPolygon>(ele);
                if (!cp) { throw std::domain_error("not a ComplicatedPolygon"); }
                double area = cp->Area();
                return area_zoom(area, minarea);
            }
            return 0;
        }
        const tagmap& categories() { return tm; }
        
    private:
        tagmap tm;
        double minlen;
        double minarea;
        std::set<std::string> keys;
};


std::shared_ptr<FindMinZoom> make_findminzoom_onetag(const tag_spec& spec, double minlen, double minarea) {
    return std::make_shared<FindMinZoomOneTag>(spec, minlen, minarea);
}

}
}

   
