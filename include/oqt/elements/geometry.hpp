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
#include "oqt/elements/quadtree.hpp"

#include "oqt/utils/pbf/protobuf.hpp"

namespace oqt {
bool is_geometry_type(ElementType ty);

class BaseGeometry : public Element {
    public:
        BaseGeometry(ElementType t, changetype c, int64 i, int64 q, ElementInfo inf, std::vector<Tag> tags, int64 minzoom_) :
            Element(t,c,i,q,inf,tags), minzoom(minzoom_) {};
            
    
        virtual ElementType OriginalType() const=0;
        virtual bbox Bounds() const=0;

        virtual std::string Wkb(bool transform, bool srid) const=0;

        virtual int64 MinZoom() const { return minzoom; }
        void SetMinZoom(int64 mz) { minzoom = mz; }
        
        virtual ~BaseGeometry() {}
        
        virtual std::list<PbfTag> pack_extras() const=0;
        
    private:
        int64 minzoom;
        
        
};

class GeometryPacked : public BaseGeometry {
    public:
        GeometryPacked(ElementType t, changetype c, int64 i, int64 q, ElementInfo inf, std::vector<Tag>, int64 minzoom_, std::list<PbfTag> geom_messages_);
        virtual uint64 InternalId() const;
        
        
        virtual ElementType OriginalType() const;
        virtual bbox Bounds() const;
        virtual std::string Wkb(bool transform, bool srid) const;
        virtual std::list<PbfTag> pack_extras() const;
        
        virtual ElementPtr copy();
        
        virtual ~GeometryPacked() {}
        
    private:
        std::list<PbfTag> geom_messages;
        uint64 internalid;
};
}


#endif
