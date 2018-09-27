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

#ifndef ADDWAYNODES_HPP
#define ADDWAYNODES_HPP

#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/geometry.hpp"

#include "oqt/elements/block.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"

namespace oqt {
namespace geometry {



struct xy {
    xy() : x(0), y(0) {}
    xy(double x_, double y_) : x(x_), y(y_) {}
    double x, y;
};



xy forward_transform(int64 ln, int64 lt);
lonlat inverse_transform(double x, double y);



class way_withnodes : public Element {
    public:
        way_withnodes(std::shared_ptr<Way> wy, const lonlatvec& lonlats_)
            : Element(ElementType::WayWithNodes, changetype::Normal, wy->Id(), wy->Quadtree(), wy->Info(), wy->Tags()), refs(wy->Refs()), lonlats(lonlats_) {
            for (auto& l : lonlats) {
                bounds.expand_point(l.lon,l.lat);
            }
        }
        way_withnodes(int64 id, int64 qt, const info& inf, const tagvector& tgs, const refvector& refs_, const lonlatvec& lonlats_, const bbox& bounds_)
            : Element(ElementType::WayWithNodes,changetype::Normal, id,qt,inf,tgs), refs(refs_), lonlats(lonlats_), bounds(bounds_) {}


        const refvector& Refs() { return refs; }

        const lonlatvec& LonLats() { return lonlats; }
        const bbox& Bounds() { return bounds; }
        bool IsRing() {
            return (Refs().size()>3) && (Refs().front()==Refs().back());
        }

        virtual std::list<PbfTag> pack_extras() const;
        virtual ~way_withnodes() {}
        virtual ElementPtr copy() { return std::make_shared<way_withnodes>(Id(),Quadtree(),Info(),Tags(),refs,lonlats,bounds); }
    private:
        way_withnodes(const way_withnodes&)=delete;
        way_withnodes operator=(const way_withnodes&)=delete;

        refvector refs;
        lonlatvec lonlats;
        bbox bounds;

};


class lonlatstore {
    public:
        virtual void add_tile(PrimitiveBlockPtr block)=0;
        virtual lonlatvec get_lonlats(std::shared_ptr<Way> way)=0;
        virtual void finish()=0;
        virtual ~lonlatstore() {}
};

std::shared_ptr<lonlatstore> make_lonlatstore();

PrimitiveBlockPtr add_waynodes(std::shared_ptr<lonlatstore> lls, PrimitiveBlockPtr bl);
/*
void add_waynodes_process(
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> in,
    std::vector<std::shared_ptr<single_queue<primitiveblock>>> out,
    std::shared_ptr<lonlatstore> lls);
*/


typedef std::function<void(PrimitiveBlockPtr)> block_callback;
block_callback make_addwaynodes_cb(block_callback cb);
}}

#endif //ADDWAYNODES_HPP
