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

#ifndef ELEMENTS_BLOCK_HPP
#define ELEMENTS_BLOCK_HPP

#include "oqt/common.hpp"
#include "oqt/elements/element.hpp"

namespace oqt {


class PrimitiveBlock {
    public:
        PrimitiveBlock(int64 i, size_t sz=0) : index(i),quadtree(-1),startdate(0),enddate(0),file_position(0),file_progress(0) {
            objects.reserve(sz);
        }   
    
        int64 Index() { return index; }
        
        
        int64 Quadtree() { return quadtree; }
        void SetQuadtree(int64 qt) { quadtree=qt; }
        
        int64 StartDate() { return startdate; }
        void SetStartDate(int64 sd) { startdate=sd; }
        
        int64 EndDate() { return enddate; }
        void SetEndDate(int64 ed) { enddate=ed; }
        
        int64 FilePosition() { return file_position; }
        void SetFilePosition(int64 fp) { file_position =fp; }
        double FileProgress() { return file_progress; }
        void SetFileProgress(double fp) { file_progress = fp; }
        
        
        std::vector<ElementPtr>& Objects() { return objects; }
        
        size_t size() const { return objects.size(); }
        ElementPtr at(size_t i) const { return objects.at(i); }
        void add(ElementPtr o) {
            if (!o) { throw std::domain_error("tried to add nullptr to PrimitiveBlock"); };
            objects.push_back(o);
        }
        
        void CopyMetadata(std::shared_ptr<PrimitiveBlock> other);
        
        
    private:
        int64 index;
        std::vector<ElementPtr> objects;
        int64 quadtree;
        int64 startdate;
        int64 enddate;
        
        int64 file_position;
        double file_progress;
        
        
};



typedef std::shared_ptr<PrimitiveBlock> PrimitiveBlockPtr;
typedef std::function<void(PrimitiveBlockPtr)> primitiveblock_callback;

}
#endif
