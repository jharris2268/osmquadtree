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

#ifndef ELEMENTS_ELEMENT_HPP
#define ELEMENTS_ELEMENT_HPP

#include "oqt/common.hpp"

#include "oqt/elements/baseelement.hpp"
#include "oqt/elements/info.hpp"
#include "oqt/elements/tag.hpp"

#include <list>
namespace oqt {

class Element;
typedef std::shared_ptr<Element> ElementPtr;


//! Element extends BaseElement
class Element : public BaseElement {
    public:
        struct data {
            ElementType type;
            changetype change;
            int64 id;
            int64 quadtree;
            ElementInfo info;
            std::vector<Tag> tags;
        };
    
    
    protected:
        //Element(ElementType t, changetype c, int64 i, int64 q, ElementInfo info, std::vector<Tag> tags);
               
        
    public:
        virtual uint64 InternalId() const=0;
        virtual ElementType Type() const=0;
        virtual changetype ChangeType() const=0;
        virtual void SetChangeType(changetype ct)=0;
        virtual int64 Id() const=0;
        virtual const ElementInfo& Info() const=0;
        virtual const std::vector<Tag>& Tags() const=0;
        virtual int64 Quadtree() const=0;
        virtual void SetQuadtree(int64 qt)=0;

        /*! Add tag
         * \param k tag key.
         * \param v tag value
         * \param replaceifpresent remove existing tags with key k */
        virtual void AddTag(const std::string& k, const std::string& v, bool replaceifpresent=true)=0;
        
        //! Remove tag \param k tag key
        virtual bool RemoveTag(const std::string& k)=0;
        
        //! Replace tags \param new_tags new tags
        virtual void SetTags(const std::vector<Tag>& new_tags)=0;

        virtual ~Element() {}
        virtual ElementPtr copy()=0;

        //! Remove null characters (\\ufeff) from tag keys, tag values, user and relation members
        friend bool fix_tags(Element& ele);

    protected:
        virtual std::vector<Tag>& EditTags()=0;

    private:
        //data data_;
    /*
        ElementType type_;
        changetype changetype_;
        int64 id_, quadtree_ ;
        ElementInfo info_;
        std::vector<Tag> tags_;*/
};

template <class Data>
class ElementImpl : public Element {
    
    protected:
        Data data_;
        ElementImpl(Data&& data__) : data_(std::move(data__)) {}
        
    public:
        
        virtual ~ElementImpl() {}
        virtual ElementPtr copy()=0;
        
        virtual uint64 InternalId() const { return internal_id(data_.type, data_.id); };
        virtual ElementType Type() const { return data_.type; };
        virtual changetype ChangeType() const { return data_.change; };
        virtual void SetChangeType(changetype ct) { data_.change = ct; };
        virtual int64 Id() const { return data_.id; };
        virtual const ElementInfo& Info() const { return data_.info; };
        virtual const std::vector<Tag>& Tags() const { return data_.tags; };
        virtual int64 Quadtree() const { return data_.quadtree; };
        virtual void SetQuadtree(int64 qt) { data_.quadtree = qt; };

        /*! Add tag
         * \param k tag key.
         * \param v tag value
         * \param replaceifpresent remove existing tags with key k */
        virtual void AddTag(const std::string& k, const std::string& v, bool replaceifpresent=true) {
            add_tag(data_.tags, k, v, replaceifpresent);
        }
        
        //! Remove tag \param k tag key
        virtual bool RemoveTag(const std::string& k) {
            return remove_tag(data_.tags, k);
        }
        
        //! Replace tags \param new_tags new tags
        virtual void SetTags(const std::vector<Tag>& new_tags) {
            data_.tags = new_tags;
        }
    protected:
        virtual std::vector<Tag>& EditTags() { return data_.tags; }
};

int64 internal_id(ElementType et, int64 id);
void add_tag(std::vector<Tag>& tags_, const std::string& k, const std::string& v, bool replaceifpresent);
bool remove_tag(std::vector<Tag>& tags_, const std::string& k);


inline bool element_cmp(const ElementPtr& l, const ElementPtr& r) {
    return l->InternalId()<r->InternalId();
}

bool fix_tags(Element& ele);
//! Remove null characters (\ufeff) from \param str
std::string fix_str(const std::string& str);
}
#endif
