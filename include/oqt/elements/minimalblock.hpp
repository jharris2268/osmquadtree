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

#ifndef ELEMENTS_MINIMALBLOCK_HPP
#define ELEMENTS_MINIMALBLOCK_HPP

#include "oqt/common.hpp"

namespace oqt {
    
namespace minimal {


struct Element {
    Element() : ty(0),id(0),changetype(0),version(0),timestamp(0),quadtree(0) {}
    
    size_t ty : 8;
    size_t id : 56;
    
    size_t changetype : 4;
    size_t version : 24;
    size_t timestamp : 36;
    int64 quadtree;
};
    
struct Node : Element {
    
    Node() : Element(), lon(0), lat(0) {}
    
    int32 lon;
    int32 lat;
};
    
struct Way : Element {
    
    Way() : Element(), refs_data("") {}
    
    std::string refs_data;
};

struct Relation : Element {
    
    Relation() : Element(), tys_data(""), refs_data("") {}
    
    std::string tys_data;
    std::string refs_data;
};

struct Geometry : Element {};


struct Block {
    int64 index;
    int64 quadtree;
    
    std::vector<Node> nodes;
    std::vector<Way> ways;
    std::vector<Relation> relations;
    std::vector<Geometry> geometries;
    
    int64 file_position;
    
    bool has_nodes;
    
    int64 uncompressed_size;
    double file_progress;
    
};

typedef std::shared_ptr<Block> BlockPtr;

}


typedef std::function<void(minimal::BlockPtr)> minimalblock_callback;
}
#endif
