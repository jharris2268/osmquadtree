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

#ifndef COUNT_HPP
#define COUNT_HPP

#include "oqt/elements/minimalblock.hpp"
#include "oqt/elements/element.hpp"
#include "oqt/elements/block.hpp"

#include <map>

namespace oqt {
class CountElement {

    public:
        int64 min_id, max_id, num_objects, min_timestamp, max_timestamp;

        CountElement() : min_id(0), max_id(0), num_objects(0),
            min_timestamp(0), max_timestamp(0) {}

        int64 len() const;
        
        
        void add(ElementPtr obj);
        
        void add (const minimal::Element& obj);

        void expand(const CountElement& other);

        std::string str() const;
};

class CountNode : public CountElement {


    public:
        int64 min_lon, min_lat, max_lon, max_lat;

        CountNode() : CountElement(),
            min_lon(1800000000), min_lat(900000000),
            max_lon(-1800000000), max_lat(-900000000) {}

        void add(ElementPtr obj);
        
        void add (const minimal::Node& obj);

        void expand(const CountNode& other);

        std::string str() const;
};

class CountWay : public CountElement {

    public:
        int64 num_refs, min_ref, max_ref, max_num_refs;

        CountWay() : CountElement(),
            num_refs(0), min_ref(0),
            max_ref(0), max_num_refs(0) {}

        void add (ElementPtr obj);
        
        
        void add (const minimal::Way& obj);
            
        void expand(const CountWay& other);

        std::string str() const;
};
class CountRelation : public CountElement {

    public:
        int64 num_nodes, num_ways, num_rels, num_empties, max_len;

        CountRelation() : CountElement(),
            num_nodes(0), num_ways(0), num_rels(0),
            num_empties(0), max_len(0) {}


        void add (ElementPtr obj);
        
        void add (const minimal::Relation& obj);
        void expand(const CountRelation& other);

        std::string str() const ;
};



struct BlockSummary {
    size_t idx;
    int64 quadtree, len;
    size_t num_nodes, num_ways, num_relations, num_geometries;
};


class Count {
    
    public:

        Count(int index_, bool tiles_, bool geom_, bool change_) : index(index_),storetiles(tiles_), geom(geom_), change(change_), numblocks(0),progress(0),uncomp(0),total(0) {
            
                
        }

        void add(size_t i, minimal::BlockPtr block);
        
        void add(size_t i, PrimitiveBlockPtr block);


        void expand(const Count& other);
        bool prog(int t);

        std::string short_str();

        std::string long_str();
        
        
        const std::map<changetype, CountNode>& nodes() const { return nn; }
        const std::map<changetype, CountWay>& ways() const { return ww; }
        const std::map<changetype, CountRelation>& relations() const { return rr; }
        const std::map<std::pair<ElementType,changetype>, CountElement>& geometries() const { return gg; }
        
        const std::vector<BlockSummary>& blocks() const { return tiles; }
        
        
        std::tuple<int64,double,int64,size_t> summary() const { return std::make_tuple(numblocks,progress,uncomp,total); }
        
        
    private:
        int index;
        bool storetiles;
        bool geom;
        bool change;
        
        int64 numblocks;
        double progress;
        int64 uncomp;
        
        size_t total;
        std::map<changetype, CountNode> nn;
        std::map<changetype, CountWay> ww;
        std::map<changetype, CountRelation> rr;
        std::map<std::pair<ElementType, changetype>, CountElement> gg;
        
        std::vector<BlockSummary> tiles;
};
enum class ReadBlockFlags;
std::shared_ptr<Count> run_count(const std::string& fn, size_t numchan, bool tiles, bool geom, ReadBlockFlags objflags, bool use_minimal);
}
#endif //COUNT_HPP
