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

#include "oqt/elements/minimalblock.hpp"
#include "oqt/pbfformat/readfileblocks.hpp"
#include "oqt/utils/date.hpp"

#include "oqt/count.hpp"
#include <algorithm>


namespace oqt {
std::shared_ptr<Count> run_count(const std::string& fn, size_t numchan, bool tiles, bool geom, size_t objflags) {
    
    
    auto& lg=Logger::Get();
    
    std::ifstream infile(fn, std::ifstream::in | std::ifstream::binary);
    if (!infile.good()) {
        Logger::Message() << "failed to open " << fn;
        return nullptr;
    }
    bool change = EndsWith(fn, "pbfc");
    
    auto result = std::make_shared<Count>(0,tiles,geom,change);
    
    
    size_t step = 14000000;
    size_t nexttot = step/2;
    
    
    auto cb = [result,&nexttot,&step](minimal::BlockPtr mb) {
        if (!mb) {
            Logger::Progress(100) << "\r" << result->short_str();
            return;
        }
        size_t total = std::get<3>(result->summary());
        if (total > nexttot) {
            Logger::Progress(mb->file_progress) << result->short_str();
            nexttot = total + step;
        }
        result->add(mb->index, mb);
    };
    

    if (numchan==0) {

        read_blocks_nothread_minimalblock(fn, cb, {}, objflags);
        lg.time("count");
        return result;

    }

    
    read_blocks_minimalblock(fn, cb, {}, numchan, objflags);
    
    lg.time("count");
    return result;
}
}

