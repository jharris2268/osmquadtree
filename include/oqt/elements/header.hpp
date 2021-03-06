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

#ifndef ELEMENTS_HEADER_HPP
#define ELEMENTS_HEADER_HPP


#include "oqt/common.hpp"
#include "oqt/elements/quadtree.hpp"

namespace oqt {
typedef std::vector<std::tuple<int64,int64,int64>> block_index;
class Header {
    
    public:
        Header() {}
        Header(const bbox& b, const block_index& idx) : box(b), index(idx) {};
    
        std::string Writer() const { return writer; };
        void SetWriter(const std::string& w) { writer=w; };
        
        std::vector<std::string>& Features() { return features; }
        
        bbox BBox() const { return box; }
        void SetBBox(bbox b) { box=b; }
    
        block_index& Index() { return index; }
    
    private:
    
        std::string writer;
        std::vector<std::string> features;
        bbox        box;
        block_index index;
    
};

typedef std::shared_ptr<Header> HeaderPtr;
}
#endif
