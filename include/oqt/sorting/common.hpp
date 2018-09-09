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

#ifndef SORTING_COMMON_HPP
#define SORTING_COMMON_HPP


#include "oqt/pbfformat/writepbffile.hpp"
#include "oqt/calcqts/qttreegroups.hpp"
#include "oqt/elements/block.hpp"


namespace oqt {

typedef std::pair<int64,std::string> keystring;
typedef std::vector<keystring> keystring_vec;
typedef std::function<void(std::shared_ptr<keystring_vec>)> writevec_callback;

class SplitBlocksDetail;
class SplitBlocks {
    typedef std::vector<primitiveblock_ptr> tempsvec;
    
    
    public:
        SplitBlocks(primitiveblock_callback callback_, size_t blocksplit_, size_t writeat_, bool msgs_);
        
        virtual ~SplitBlocks();
        
        virtual size_t find_tile(element_ptr obj)=0;
        virtual size_t max_tile()=0;
        
        
        
        void call(primitiveblock_ptr bl);
    
    private:
        size_t blocksplit;
        
        std::shared_ptr<tempsvec> temps;
        std::function<void(std::shared_ptr<tempsvec>)> call_writetemps;
        std::unique_ptr<SplitBlocksDetail> detail;
}; 
}

#endif //SORTING_COMMON_HPP
