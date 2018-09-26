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

#ifndef ELEMENTS_BASEELEMENT_HPP
#define ELEMENTS_BASEELEMENT_HPP

#include "oqt/common.hpp"
#include <ostream>
namespace oqt {
    
/*! used in osm change files: see
 * https://wiki.openstreetmap.org/wiki/OsmChange*/
enum struct changetype {
    Normal=0, //!< For normal osm files
    Delete, //!< Delete from file
    Remove, //!< Remove from this block
    Unchanged, //!< Add to this block
    Modify, //!< New version of object
    Create //!< New object
};
std::ostream& operator<<(std::ostream& strm, changetype c);

/*! Allows user to cast (with std::dynamic_pointer_cast) BaseElement
 * or Element to correct type*/
enum struct ElementType {
    Node=0,
    Way,
    Relation,
    Point,
    Linestring,
    SimplePolygon,
    ComplicatedPolygon,
    WayWithNodes,
    Unknown
};
std::ostream& operator<<(std::ostream& strm, ElementType e);


//! Base class for all osm Elements
class BaseElement {
    public:
        //! Allows for simple sorting of Elements
        virtual uint64      InternalId() const =0;
        
        virtual ElementType Type() const =0;
        virtual int64       Id() const =0;
        
        //! Allows for grouping of Elements by location
        virtual int64       Quadtree() const =0;
        virtual changetype  ChangeType() const =0;
        
        virtual ~BaseElement() {}
};
}
#endif
