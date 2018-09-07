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

#include "XmlInspector.hpp"
#include "oqt/utils.hpp"
#include "oqt/update/xmlchange.hpp"
#include "oqt/elements/node.hpp"
#include "oqt/elements/way.hpp"
#include "oqt/elements/relation.hpp"


namespace oqt {
namespace xmlchangedetail {
typedef Xml::Inspector<Xml::Encoding::Utf8Writer> Inspector;

int64 read_int(const std::string& s) {
    return atoll(s.c_str());
}

int64 read_lonlat(const std::string& s) {
    double f = atof(s.c_str());
    if (f<0) {
        return (int64) (f*10000000 - 0.5);
    }
    return f*10000000 + 0.5;
}

void add_tag(Inspector& inspector, tagvector& tags) {
    if (inspector.GetAttributesCount()!=2) { throw std::domain_error("expected tag to have two attributes"); }
    tag t;
    for (int i=0; i<2; i++) {
        if (inspector.GetAttributeAt(i).Name=="k") {
            t.key = inspector.GetAttributeAt(i).Value;
        } else if (inspector.GetAttributeAt(i).Name=="v") {
            t.val = inspector.GetAttributeAt(i).Value;
        }

    }
    tags.push_back(t);

    if (inspector.GetInspected() == Xml::Inspected::EmptyElementTag) {
        return;
    }

    while (inspector.Inspect()) {
        if ((inspector.GetInspected() == Xml::Inspected::EndTag) && (inspector.GetName()=="tag")) {
            return;
        }
    }

}

inline bool check_info(const Inspector::AttributeType& attr, info& inf) {
    if (attr.Name == "version") { inf.version = read_int(attr.Value); return true; }
    if (attr.Name == "changeset") { inf.changeset = read_int(attr.Value); return true; }
    if (attr.Name == "timestamp") { inf.timestamp = read_date(attr.Value); return true; }
    if (attr.Name == "uid") { inf.user_id = read_int(attr.Value); return true; }
    if (attr.Name == "user") { inf.user = attr.Value; return true; }
    return false;
}

void read_node(Inspector& inspector, changetype mode, std::function<void(std::shared_ptr<element>)>& add, bool allow_missing_users) {
    int64 id=0,lon=0,lat=0;
    info inf; inf.visible=true;
    inf.user=""; inf.user_id=-1;
    tagvector tags;

    int nattr=inspector.GetAttributesCount();
    if ((!allow_missing_users) && (nattr!=8)) {
        throw std::domain_error("expected tag to have 8 attributes");
    }
    for (int i=0; i < nattr; i++) {
        auto attr = inspector.GetAttributeAt(i);
        if (attr.Name == "id") { id = read_int(attr.Value); }
        else if (check_info(attr, inf)) { /*pass*/ }
        else if (attr.Name == "lon") { lon = read_lonlat(attr.Value); }
        else if (attr.Name == "lat") { lat = read_lonlat(attr.Value); }
        else { throw std::domain_error("unexpected attribute "+attr.Name); }
    }
    if (inspector.GetInspected() == Xml::Inspected::EmptyElementTag) {
        add(std::make_shared<node>(mode,id,-1,inf,tags,lon,lat));
        return;
    }
    while (inspector.Inspect()) {
        auto ii = inspector.GetInspected();
        if ((ii == Xml::Inspected::StartTag) ||  (ii==Xml::Inspected::EmptyElementTag)){
            if (inspector.GetName()=="tag") {
                add_tag(inspector,tags);
            }
            else { throw std::domain_error("unexpected element "+inspector.GetName()); }
        }
        if ((ii == Xml::Inspected::EndTag) &&
            (inspector.GetName()=="node")) {

            add(std::make_shared<node>(mode,id,-1,inf,tags,lon,lat));
            return;
        }
    }

}

void add_ref(Inspector& inspector, std::vector<int64>& refs) {
    if (inspector.GetAttributesCount()!=1) {
        throw std::domain_error("expected nd to have 1 attribute");
    }
    if (inspector.GetAttributeAt(0).Name!="ref") {
        throw std::domain_error("expected nd to have 1 attribute, called ref");
    }
    refs.push_back(read_int(inspector.GetAttributeAt(0).Value));
    if (inspector.GetInspected() == Xml::Inspected::EmptyElementTag) {
        return;
    }
    while (inspector.Inspect()) {
        if ((inspector.GetInspected() == Xml::Inspected::EndTag) &&
            (inspector.GetName()=="nd")) {
                return;
        }
    }
}


void read_way(Inspector& inspector, changetype mode, std::function<void(std::shared_ptr<element>)>& add, bool allow_missing_users) {
    int64 id=0;
    info inf; inf.visible=true;
    inf.user=""; inf.user_id=-1;
    tagvector tags;
    std::vector<int64> refs;
    int nattr=inspector.GetAttributesCount();
    if ((!allow_missing_users) && (nattr!=6)) {
        throw std::domain_error("expected tag to have 6 attributes");
    }
    for (int i=0; i < nattr; i++) {
        auto attr = inspector.GetAttributeAt(i);
        if (attr.Name == "id") { id = read_int(attr.Value); }
        else if (check_info(attr, inf)) { /*pass*/ }
        else { throw std::domain_error("unexpected attribute "+attr.Name); }
    }
    if (inspector.GetInspected() == Xml::Inspected::EmptyElementTag) {
        add(std::make_shared<way>(mode,id,-1,inf,tags,refs));
        return;
    }

    while (inspector.Inspect()) {
        auto ii = inspector.GetInspected();
        if ((ii == Xml::Inspected::StartTag) ||  (ii==Xml::Inspected::EmptyElementTag)){
            if (inspector.GetName()=="tag") {
                add_tag(inspector,tags);
            } else if (inspector.GetName()=="nd") {
                add_ref(inspector, refs);
            } else { throw std::domain_error("unexpected element "+inspector.GetName()); }
        }
        if ((ii == Xml::Inspected::EndTag) &&
            (inspector.GetName()=="way")) {

            add(std::make_shared<way>(mode,id,-1,inf,tags,refs));
            return;
        }
    }
}

elementtype read_member_type(const std::string s) {
    if (s=="node") { return Node; }
    if (s=="way") { return Way; }
    if (s=="relation") { return Relation; }
    throw std::domain_error("unexpected member type");
    return WayWithNodes;
    //return -1;
}

void add_member(Inspector& inspector, std::vector<member>& mems) {
    if (inspector.GetAttributesCount()!=3) {
        throw std::domain_error("expected nd to have 1 attribute");
    }
    member m;
    for (int i=0; i < 3; i++) {
        auto attr = inspector.GetAttributeAt(i);
        if (attr.Name=="ref") { m.ref = read_int(attr.Value); }
        else if (attr.Name=="type") { m.type = read_member_type(attr.Value); }
        else if (attr.Name=="role") { m.role = attr.Value; }
        else { throw std::domain_error("unexpected attribute "+attr.Name); }
    }
    mems.push_back(m);
    if (inspector.GetInspected() == Xml::Inspected::EmptyElementTag) {
        return;
    }
    while (inspector.Inspect()) {
        if ((inspector.GetInspected() == Xml::Inspected::EndTag) &&
            (inspector.GetName()=="member")) {
                return;
        }
    }
}

void read_relation(Inspector& inspector, changetype mode, std::function<void(std::shared_ptr<element>)>& add, bool allow_missing_users) {
    int64 id=0;
    info inf; inf.visible=true;
    inf.user=""; inf.user_id=-1;
    tagvector tags;
    std::vector<member> membs;

    int nattr=inspector.GetAttributesCount();
    if ((!allow_missing_users) && (nattr!=6)) {
        throw std::domain_error("expected tag to have 6 attributes");
    }
    
    for (int i=0; i < nattr; i++) {
        auto attr = inspector.GetAttributeAt(i);
        if (attr.Name == "id") { id = read_int(attr.Value); }
        else if (check_info(attr, inf))  { /*pass*/ }
        else { throw std::domain_error("unexpected attribute "+attr.Name); }
    }

    if (inspector.GetInspected() == Xml::Inspected::EmptyElementTag) {
        add(std::make_shared<relation>(mode,id,-1,inf,tags,membs));
        return;
    }

    while (inspector.Inspect()) {
        auto ii = inspector.GetInspected();
        if ((ii == Xml::Inspected::StartTag) ||  (ii==Xml::Inspected::EmptyElementTag)){
            if (inspector.GetName()=="tag") {
                add_tag(inspector,tags);
            } else if (inspector.GetName()=="member") {
                add_member(inspector, membs);
            } else { throw std::domain_error("unexpected element "+inspector.GetName()); }
        }
        if ((ii == Xml::Inspected::EndTag) &&
            (inspector.GetName()=="relation")) {

            add(std::make_shared<relation>(mode,id,-1,inf,tags,membs));
            return;
        }
    }


}

void add_all(Xml::Inspector<Xml::Encoding::Utf8Writer>& inspector, std::function<void(std::shared_ptr<element>)> add, bool allow_missing_users) {
    changetype mode=Normal;
    while (inspector.Inspect()) {
       switch (inspector.GetInspected())
        {
            case Xml::Inspected::StartTag:
                if (inspector.GetName()=="osmChange") {
                    //pass
                } else if (inspector.GetName()=="delete") {
                    mode=Delete;
                } else if (inspector.GetName()=="modify") {
                    mode=Modify;
                } else if (inspector.GetName()=="create") {
                    mode=Create;
                } else if (inspector.GetName()=="node") {
                    read_node(inspector, mode,add, allow_missing_users);
                } else if (inspector.GetName()=="way") {
                    read_way(inspector, mode,add, allow_missing_users);
                } else if (inspector.GetName()=="relation") {
                    read_relation(inspector, mode,add, allow_missing_users);
                } else {
                    logger_message() << "StartTag name: " << inspector.GetName();
                }
                break;
            case Xml::Inspected::EmptyElementTag:
                if (inspector.GetName()=="node") {
                    read_node(inspector, mode,add, allow_missing_users);
                } else if (inspector.GetName()=="way") {
                    read_way(inspector, mode,add, allow_missing_users);
                } else if (inspector.GetName()=="relation") {
                    read_relation(inspector, mode,add, allow_missing_users);
                } else {
                    logger_message() << "EmptyElementTag name: " << inspector.GetName();
                }
                break;
            case Xml::Inspected::EndTag:
                if (inspector.GetName()=="osmChange") {
                    //pass
                } else if (inspector.GetName()=="delete") {
                    mode=Normal;
                } else if (inspector.GetName()=="modify") {
                    mode=Normal;
                } else if (inspector.GetName()=="create") {
                    mode=Normal;
                } else {
                    logger_message() << "EndTag name: " << inspector.GetName();
                }
                break;
            default:
                break;
        }
    }
}

std::shared_ptr<primitiveblock> read_xml_change_detail(Xml::Inspector<Xml::Encoding::Utf8Writer>& inspector) {
    auto pb = std::make_shared<primitiveblock>(0,0);
    auto add = [pb](std::shared_ptr<element> e)->void {
        pb->objects.push_back(e);
    };
    add_all(inspector,add, true);
    return pb;
}

void read_xml_change_detail(Xml::Inspector<Xml::Encoding::Utf8Writer>& inspector, typeid_element_map_ptr em, bool allow_missing_users) {

    auto add = [em](std::shared_ptr<element> e)->void {
        auto k = std::make_pair(e->Type(),e->Id());
        if (em->count(k)==0) {
            (*em)[k]=e;
            return;
        }
        auto o=(*em)[k];
        if (e->Info().version > o->Info().version) {
            (*em)[k]=e;
        }
    };

    add_all(inspector, add, allow_missing_users);

}

}

std::shared_ptr<primitiveblock> read_xml_change(const std::string& data) {
    Xml::Inspector<Xml::Encoding::Utf8Writer> inspector(data.begin(), data.end());
    return xmlchangedetail::read_xml_change_detail(inspector);
}

std::shared_ptr<primitiveblock> read_xml_change_file(std::istream* fl) {

    Xml::Inspector<Xml::Encoding::Utf8Writer> inspector(fl);
    return xmlchangedetail::read_xml_change_detail(inspector);
}

void read_xml_change_file_em(std::istream* fl, typeid_element_map_ptr em, bool allow_missing_users) {

    Xml::Inspector<Xml::Encoding::Utf8Writer> inspector(fl);
    xmlchangedetail::read_xml_change_detail(inspector,em, allow_missing_users);
}
void read_xml_change_em(const std::string& data, typeid_element_map_ptr em, bool allow_missing_users) {

    Xml::Inspector<Xml::Encoding::Utf8Writer> inspector(data.begin(),data.end());
    xmlchangedetail::read_xml_change_detail(inspector,em, allow_missing_users);
}
}
