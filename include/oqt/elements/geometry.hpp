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

#ifndef ELEMENTS_GEOMETRY_HPP
#define ELEMENTS_GEOMETRY_HPP

#include "oqt/elements/element.hpp"
#include "oqt/quadtree.hpp"
namespace oqt {
bool isGeometryType(elementtype ty);

class basegeometry : public element {
    public:
        basegeometry(elementtype t, changetype c, int64 i, int64 q, info inf, std::vector<tag> tags, int64 minzoom_) :
            element(t,c,i,q,inf,tags), minzoom(minzoom_) {};
            
    
        virtual elementtype OriginalType() const=0;
        virtual bbox Bounds() const=0;

        virtual std::string Wkb(bool transform, bool srid) const=0;

        virtual int64 MinZoom() const { return minzoom; }
        void SetMinZoom(int64 mz) { minzoom = mz; }
        
        virtual ~basegeometry() {}
        
        virtual std::list<PbfTag> pack_extras() const=0;
        
    private:
        int64 minzoom;
        
        
};

class geometry_packed : public basegeometry {
    public:
        geometry_packed(elementtype t, changetype c, int64 i, int64 q, info inf, std::vector<tag> tags, int64 minzoom_, std::list<PbfTag> geom_messages_) :
            basegeometry(t,c,i,q,inf,tags,minzoom_), geom_messages(geom_messages_), internalid(0) {
            
            internalid = (((uint64) t)<<61ull);
            if (t==6) {
                internalid |= ( ((uint64) i) << 16ull);
                for (const auto& m: geom_messages) {
                    if (m.tag==19) { internalid |= m.value/2; } //part (zigzag int64)
                }
            } else {
                internalid |= i;
            }
                
        }
        virtual uint64 InternalId() const { return internalid; }
        
        
        virtual elementtype OriginalType() const { return Unknown; }
        virtual bbox Bounds() const { return bbox{1,1,0,0}; }
        virtual std::string Wkb(bool transform, bool srid) const { throw std::domain_error("not implemented"); }
        
        std::list<PbfTag> pack_extras() const { return geom_messages; }
        
        virtual element_ptr copy() {
            return std::make_shared<geometry_packed>(Type(),ChangeType(),Id(),Quadtree(),Info(),Tags(),MinZoom(),geom_messages);
        }
        
        virtual ~geometry_packed() {}
        
    private:
        std::list<PbfTag> geom_messages;
        uint64 internalid;
};
}


#endif
