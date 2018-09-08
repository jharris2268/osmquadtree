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

#ifndef ELEMENTS_PACKEDOBJ_HPP
#define ELEMENTS_PACKEDOBJ_HPP

#include "oqt/common.hpp"
#include "oqt/elements/rawelement.hpp"
namespace oqt {
/*class packedobj : public rawelement {
    
    
    public:
        packedobj(uint64 id_, int64 qt_, const std::string& data_, changetype ct_) :
            id(id_), qt(qt_), data(data_), ct(ct_) {}

        std::string pack() const;

        uint64 InternalId() const { return id; }

        elementtype Type() const { return (elementtype) (id>>61); };
        int64 Id() const { return id&0x1fffffffffffffff; };
        int64 Quadtree() const { return qt; };
        void SetQuadtree(int64 qt_) { qt=qt_; }
        changetype ChangeType() const { return ct; }
        void SetChangeType(changetype ct_) { ct=ct_; }

        const std::string& Data() const { return data; }

        uint64 approx_size() const {
            return 30+data.size();
        }
        
        std::shared_ptr<packedobj> copy() const {
            return std::make_shared<packedobj>(id,qt,data,ct);
        }
        
        virtual ~packedobj() {}
    
    private:
        uint64 id;
        int64 qt;
        std::string data;
        changetype ct;
};

typedef std::shared_ptr<packedobj> packedobj_ptr;
//typedef std::unique_ptr<packedobj> packedobj_ptr;

packedobj_ptr make_packedobj_ptr(uint64 id, int64 quadtree, const std::string& data, changetype ct);
packedobj_ptr read_packedobj(const std::string& data);

inline bool packedobj_cmp(const packedobj_ptr& l, const packedobj_ptr& r) {
    if ((!l) || (!r)) { throw std::domain_error("packedobj_cmp nullptr!!");}
    return l->InternalId() < r->InternalId();
}




std::tuple<info,tagvector,std::list<PbfTag>> unpack_packedobj_common(const std::string& data);
*/

}

#endif

