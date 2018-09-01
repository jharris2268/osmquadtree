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

#ifndef PBFFORMAT_OBJSIDSET_HPP
#define PBFFORMAT_OBJSIDSET_HPP

#include "oqt/pbfformat/idset.hpp"
#include "oqt/elements/block.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"

#include <set>
namespace oqt {


class objs_idset : public idset {
    public:
        objs_idset() {}
        virtual ~objs_idset() {}
        virtual bool contains(elementtype ty, int64 id) const;
        
        void add(elementtype t, int64 i);
        void add_node(std::shared_ptr<node> nn);

        void add_way(std::shared_ptr<way> ww);

        void add_relation(std::shared_ptr<relation> rr);
        
        void add_all(std::shared_ptr<primitiveblock> pb);
    
    
        const std::set<int64>& nodes() { return nodes_; }
        const std::set<int64>& ways() { return ways_; }
        const std::set<int64>& relations() { return relations_; }
    
    private:

        std::set<int64> nodes_;
        std::set<int64> ways_;
        std::set<int64> relations_;
};

}


#endif //PBFFORMAT_OBJSIDSET_HPP
