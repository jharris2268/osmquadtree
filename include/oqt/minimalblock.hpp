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

#ifndef MINIMALBLOCK
#define MINIMALBLOCK

#include "oqt/simplepbf.hpp"
#include <deque>
namespace oqt {
struct minimalnode {
    int64 id;
    //int64 timestamp;
    size_t changetype : 4;
    size_t version : 24;
    size_t timestamp : 36;
    int64 quadtree;
    
    int32 lon;
    int32 lat;
};
    
struct minimalway {
    int64 id;
    //int64 timestamp;
    size_t changetype : 4;
    size_t version : 24;
    size_t timestamp : 36;
    int64 quadtree;
    
    //std::vector<int64> refs;
    std::string refs_data;
};

struct minimalrelation {
    int64 id;
    //int64 timestamp;
    size_t changetype : 4;
    size_t version : 24;
    size_t timestamp : 36;
    int64 quadtree;
    
    std::string tys_data;
    std::string refs_data;
};

struct minimalgeometry {
    size_t ty : 8;
    size_t id : 56;
    
    size_t changetype : 4;
    size_t version : 24;
    size_t timestamp : 36;
    int64 quadtree;
};

struct minimalblock {
    int64 index;
    int64 quadtree;
    
    std::vector<minimalnode> nodes;
    std::vector<minimalway> ways;
    std::vector<minimalrelation> relations;
    std::vector<minimalgeometry> geometries;
    
    int64 file_position;
    
    bool has_nodes;
    
    int64 uncompressed_size;
    double file_progress;
    
};


std::shared_ptr<minimalblock> readMinimalBlock(int64 index, const std::string& data, size_t objflags=7);

typedef std::vector<std::pair<uint64,int64> > qtvec;
std::shared_ptr<qtvec> readQtVecBlock(const std::string& data, size_t objflags=7);
}
#endif
