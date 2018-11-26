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

#include "oqt/geometry/postgiswriter.hpp"
#include "oqt/geometry/utils.hpp"
#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"
#include "oqt/utils/logger.hpp"

#include "oqt/utils/pbf/protobuf.hpp"
#include "oqt/utils/pbf/fixedint.hpp"

#include <sstream>
#include <iostream>
#include <map>
#include <postgresql/libpq-fe.h>
#include "picojson.h"

namespace oqt {
namespace geometry {

const char quote = '\x01';
const char delim = '\x02';

void quotestring(std::ostream& strm, const std::string& val) {
    strm << quote;
    for (auto c : val) {
        if (c=='\n') {
            strm << "\\n";
        } else {
            /*if (c=='"') {
                strm << '"';
            }*/
            strm << c;
        }
    }
    strm << quote;
}

std::ostream& writestring(std::ostream& strm, const std::string& val) {
    if (!val.empty()) {
        quotestring(strm,val);
    }
    
    return strm;
}


void json_quotestring(std::ostream& strm, const std::string& val) {
    strm << '"';
    for (auto c : val) {
        if (c=='\n') {
            strm << "\\\n";
        } else if (c=='"') {
            strm << '\\' << '"';
        } else if (c=='\t') {
            strm << "\\\t";
        } else if (c=='\r') {
            strm << "\\\r";
        } else if (c=='\\') {
            strm << "\\\\";
        } else {
            strm << c;
        }
    }
    strm << '"';
}

void hstore_quotestring(std::ostream& strm, const std::string& val) {
    strm << '"';
    for (auto c : val) {
        if (c=='\n') {
            strm << '\\' << 'n';
        } else if (c=='"') {
            strm << '\\' << '"';
        } else if (c=='\t') {
            strm << '\\' << 't';
        } else if (c=='\r') {
            strm << '\\' << 'r';
        } else if (c=='\\') {
            strm << '\\' << '\\';
        } else {
            strm << c;
        }
    }
    strm << '"';
}


std::string pack_jsontags(const tagvector& tags) {
    std::stringstream strm;
    strm << "{";
    bool isf=true;
    for (const auto& t: tags) {
        if (!isf) { strm << ", "; }
        json_quotestring(strm,t.key);
        strm << ": ";
        json_quotestring(strm,t.val);
        isf=false;
    }
    strm << "}";
    return strm.str();
}

std::string pack_hstoretags(const tagvector& tags) {
    std::stringstream strm;
    
    bool isf=true;
    for (const auto& t: tags) {
        if (!isf) { strm << ", "; }
        hstore_quotestring(strm, t.key);
        
        //strm << picojson::value(t.key).serialize();
        
        strm << "=>";
        hstore_quotestring(strm, t.val);
        //strm << picojson::value(t.val).serialize();
        
        isf=false;
    }
    return strm.str();
}

std::string pack_jsontags_picojson(const tagvector& tags) {
    picojson::object p;
    for (const auto& t: tags) {
        p[t.key] = picojson::value(t.val);
    }
    return picojson::value(p).serialize();
}
    
    

size_t _write_data(std::string& data, size_t pos, const std::string& v) {
    std::copy(v.begin(),v.end(),data.begin()+pos);
    pos+=v.size();
    return pos;
}

std::string pack_hstoretags_binary(const tagvector& tags) {
    size_t len=4 + tags.size()*8;
    for (const auto& tg: tags) {
        len += tg.key.size();
        len += tg.val.size();
    }
    
    std::string out(len,'\0');
    size_t pos=0;
    pos = write_int32(out,pos,tags.size());
    
    for (const auto& tg: tags) {
        pos = write_int32(out,pos,tg.key.size());
        pos = _write_data(out,pos,tg.key);
        pos = write_int32(out,pos,tg.val.size());
        pos = _write_data(out,pos,tg.val);
    }
    return out;
}
    

std::string prep_tags(std::stringstream& strm, const std::map<std::string,size_t>& tags, ElementPtr obj, bool other_tags, bool asjson) {
    std::vector<std::string> tt(tags.size(),"");
    
    tagvector others;
    
    if (!obj->Tags().empty()) {
        for (const auto& tg:obj->Tags()) {
            auto it=tags.find(tg.key);
            if (it!=tags.end()) {
                if (it->second >= tags.size()) {
                    Logger::Message() << "tag out of bounds?? " << tg.key << " " << tg.val << "=>" << it->second << "/" << tt.size();
                    throw std::domain_error("tag out of bounds");
                }
                tt.at(it->second)=tg.val;
            } else if (other_tags) {
                others.push_back(tg);
            }
        }
    }
    
    
    for (const auto& t:tt) {
        writestring(strm,t) << delim;
        
    }
    if (other_tags && !others.empty()) {
        if (asjson) {
            return pack_jsontags_picojson(others);
        } else {
            return pack_hstoretags(others);
        }
    }
    return std::string();            
}

std::pair<std::string,std::string> prep_tags_binary(const std::map<std::string,size_t>& tags, ElementPtr obj, bool other_tags) {
    std::vector<std::string> tt(tags.size(),"");
    
    tagvector others;
    size_t len=tags.size()*4;
    
    if (!obj->Tags().empty()) {
        for (const auto& tg:obj->Tags()) {
            auto it=tags.find(tg.key);
            if (it!=tags.end()) {
                if (it->second >= tags.size()) {
                    Logger::Message() << "tag out of bounds?? " << tg.key << " " << tg.val << "=>" << it->second << "/" << tt.size();
                    throw std::domain_error("tag out of bounds");
                }
                tt.at(it->second)=tg.val;
                len+=tg.val.size();
            } else if (other_tags) {
                others.push_back(tg);
            }
        }
    }
    
    std::string out(len,0);
    size_t pos=0;
    for (const auto& t:tt) {
        if (t.empty()) {
            pos=write_int32(out,pos,-1);
        } else {
            pos=write_int32(out,pos,t.size());
            pos=_write_data(out,pos,t);
        }
    }
    
    std::string hstore;
    if (other_tags && !others.empty()) {
        hstore=pack_hstoretags_binary(others);
    }
    return std::make_pair(out,hstore);
}
        


std::string as_hex(const std::string& str) {
    std::stringstream ss;
    for (unsigned char c : str) {
        ss << std::hex << std::setfill('0') << std::setw(2) << (size_t) c;
    }

    return ss.str();

}

CsvRows::CsvRows(bool is_binary_) : _is_binary(is_binary_) {
    data.reserve(1024*1024);
    if (_is_binary) {
        data += std::string("PGCOPY\n\xff\r\n\x00\x00\x00\x00\x00\x00\x00\x00\x00",19);
    }
}
    
void CsvRows::add(const std::string row) {
    if ((row.size() + data.size()) > data.capacity()) {
        data.reserve(data.capacity()+1024*1024);
    }    
    
    if (poses.empty()) { poses.push_back(data.size()); }
    
    //pos = _write_data(data, pos, row);
    data += row;
    poses.push_back(data.size());
}

std::string CsvRows::at(int i) const {
    if (i<0) { i += size(); }
    if (i >= size()) { throw std::range_error("out of range"); }
    size_t p = poses[i];
    size_t q = poses[i+1];
    return data.substr(p,q-p);
}

void CsvRows::finish() {
    if (_is_binary) {
        data += "\xff\xff";        
    }
    
}

int CsvRows::size() const {
    if (poses.empty()) { return 0; }
    return poses.size()-1;
}





class PackCsvBlocksImpl : public PackCsvBlocks {
    public:
        PackCsvBlocksImpl(
            const PackCsvBlocks::tagspec& tags, bool with_header_, bool asjson_) : with_header(with_header_), minzoom(false), other_tags(false), asjson(asjson_),layerint(false) {
                
            std::stringstream point_ss, line_ss, poly_ss;
            if (with_header) {
                point_ss << "osm_id" << delim << "block_quadtree" << delim << "quadtree" << delim;
                line_ss << "osm_id" << delim << "block_quadtree" << delim << "quadtree" << delim;
                poly_ss << "osm_id" << delim << "part" << delim << "block_quadtree" << delim << "quadtree" << delim;
            }
                
            for (const auto& t: tags) {
                if (std::get<0>(t)=="minzoom") {
                    minzoom=true;
                } else if ( (std::get<0>(t)=="*") || (std::get<0>(t)=="XXX") ){
                    other_tags=true;
                } else if ( (std::get<0>(t)=="layer") ) {
                    layerint = true;
                } else {
                
                    if (std::get<1>(t)) {
                        point_tags.insert(std::make_pair(std::get<0>(t),point_tags.size()));
                        if (with_header) {
                            point_ss << std::get<0>(t) << delim;
                        }
                    }
                    if (std::get<2>(t)) {
                        line_tags.insert(std::make_pair(std::get<0>(t),line_tags.size()));
                        if (with_header) {
                            line_ss << std::get<0>(t) << delim;
                        }
                    }
                    if (std::get<3>(t)) {
                        poly_tags.insert(std::make_pair(std::get<0>(t),poly_tags.size()));
                        if (with_header) {
                            poly_ss << std::get<0>(t) << delim;
                        }
                    }
                }
            }
            
            if (with_header) {
                if (other_tags) {
                    point_ss << "tags" << delim;
                    line_ss << "tags" << delim;
                    poly_ss << "tags" << delim;
                }
                if (layerint) {
                    point_ss << "layer" << delim;
                    line_ss << "layer" << delim;
                    poly_ss << "layer" << delim;
                }
                
                
                if (minzoom) {
                    point_ss << "minzoom" << delim;
                    line_ss << "minzoom" << delim;
                    poly_ss << "minzoom" << delim;
                }
                line_ss << "z_order" << delim;
                poly_ss << "z_order" << delim;
                
                line_ss << "length" << delim;
                poly_ss << "way_area" << delim;
                
                point_ss << "way\n";
                line_ss << "way\n";
                poly_ss << "way\n";
                
            
                point_header = point_ss.str();
                line_header = line_ss.str();
                poly_header = poly_ss.str();
            }
            
        }

        virtual ~PackCsvBlocksImpl() {}

        void add_to_csvblock(PrimitiveBlockPtr bl, std::shared_ptr<CsvBlock> res) {
            if (bl->size()==0) {
                return;
            }
            for (auto o : bl->Objects()) {

                if (o->Type()==ElementType::Point) {
                    std::stringstream ss;
                    ss << std::dec << o->Id() << delim << bl->Quadtree() << delim << o->Quadtree() << delim;
                    auto pt = std::dynamic_pointer_cast<Point>(o);
                    auto other = prep_tags(ss,point_tags,o, other_tags, asjson);
                    
                    
                    if (other_tags) {
                        if (!other.empty()) {
                            ss << quote << other << quote;
                        }
                        ss <<delim;
                    }
                    if (layerint) {
                        if (pt->Layer()!=0) {
                            ss << pt->Layer();
                        }
                        ss<<delim;
                    }
                    
                    if (minzoom) {
                        if ((pt->MinZoom()>=0) && (pt->MinZoom()<100)) {
                            ss << std::dec << pt->MinZoom();
                        }
                        ss << delim;
                    }
                    
                    ss << as_hex(pt->Wkb(true, true));
                    ss << "\n";
                    if (with_header && (res->points.size()==0)) {
                        res->points.add(point_header);
                    }
                    
                    res->points.add(ss.str());

                } else if (o->Type()==ElementType::Linestring) {
                    std::stringstream ss;
                    ss << std::dec << o->Id() << delim << bl->Quadtree() << delim << o->Quadtree() << delim;
                    auto ln = std::dynamic_pointer_cast<Linestring>(o);
                    auto other = prep_tags(ss,line_tags,o, other_tags, asjson);
                    
                    if (other_tags) {
                        if (!other.empty()) {
                            quotestring(ss,other);
                        }
                        ss << delim;
                    }
                    if (layerint) {
                        if (ln->Layer()!=0) {
                            ss << ln->Layer();
                        }
                        ss<<delim;
                    }
                    
                    if (minzoom) {
                        if ((ln->MinZoom()>=0) && (ln->MinZoom()<100)) {
                            ss << std::dec << ln->MinZoom();
                        }
                        ss << delim;
                    }
                    ss << std::dec << ln->ZOrder() << delim;
                    ss << std::fixed << std::setprecision(1) << ln->Length() << delim;
                    ss << as_hex(ln->Wkb(true, true));
                    ss << "\n";
                    if (with_header && (res->lines.size()==0)) {
                        res->lines.add(point_header);
                    }
                    res->lines.add(ss.str());
                } else if (o->Type()==ElementType::SimplePolygon) {
                    std::stringstream ss;
                    ss << std::dec << o->Id() << delim << delim << bl->Quadtree() << delim << o->Quadtree() << delim;
                    
                    auto py = std::dynamic_pointer_cast<SimplePolygon>(o);
                    auto other = prep_tags(ss,poly_tags,o, other_tags, asjson);
                    
                    if (other_tags) {
                        if (!other.empty()) {
                            quotestring(ss,other);
                        }
                        ss << delim;
                    }
                    if (layerint) {
                        if (py->Layer()!=0) {
                            ss << py->Layer();
                        }
                        ss<<delim;
                    }
                    
                    if (minzoom) {
                        if ((py->MinZoom()>=0) && (py->MinZoom()<100)) {
                            ss << std::dec << py->MinZoom();
                        }
                        ss << delim;
                    }
                    ss << std::dec << py->ZOrder() << delim << std::fixed << std::setprecision(1) << py->Area() << delim;
                    ss << as_hex(py->Wkb(true, true));
                    ss << "\n";
                    if (with_header && (res->polygons.size()==0)) {
                        res->polygons.add(point_header);
                    }
                    res->polygons.add(ss.str());
                } else if (o->Type()==ElementType::ComplicatedPolygon) {
                    std::stringstream ss;
                    auto py = std::dynamic_pointer_cast<ComplicatedPolygon>(o);
                    ss << std::dec << (-1ll * o->Id()) << delim << py->Part() << delim << bl->Quadtree() << delim << o->Quadtree() << delim;
                    auto other = prep_tags(ss,poly_tags,o, other_tags, asjson);
                    if (other_tags) {
                        if (!other.empty()) {
                            quotestring(ss,other);
                        }
                        ss << delim;
                    }
                    if (layerint) {
                        if (py->Layer()!=0) {
                            ss << py->Layer();
                        }
                        ss<<delim;
                    }
                    
                    if (minzoom) {
                        if ((py->MinZoom()>=0) && (py->MinZoom()<100)) {
                            ss << std::dec << py->MinZoom();
                        }
                        ss << delim;
                    }
                    ss << std::dec << py->ZOrder() << delim << std::fixed << std::setprecision(1) << py->Area() << delim;
                    ss << as_hex(py->Wkb(true, true));
                    ss << "\n";
                    if (with_header && (res->polygons.size()==0)) {
                        res->polygons.add(point_header);
                    }
                    res->polygons.add(ss.str());
                }
            }
        }
       
        
        std::shared_ptr<CsvBlock> call(PrimitiveBlockPtr block) {
            if (!block) { return nullptr; }
            auto res = std::make_shared<CsvBlock>(false);
            add_to_csvblock(block,res);
            res->points.finish();
            res->lines.finish();
            res->polygons.finish();
            return res;
        }
        

    private:

        std::map<std::string,size_t> point_tags;
        std::map<std::string,size_t> line_tags;
        std::map<std::string,size_t> poly_tags;
        bool with_header;
        bool minzoom;
        bool other_tags;
        bool asjson;
        bool layerint;
        std::string point_header;
        std::string line_header;
        std::string poly_header;
};



class PackCsvBlocksBinaryImpl : public PackCsvBlocks {
    public:
        PackCsvBlocksBinaryImpl(
            const PackCsvBlocks::tagspec& tags) : minzoom(false), other_tags(false),layerint(false) {
                
             
            for (const auto& t: tags) {
                if (std::get<0>(t)=="minzoom") {
                    minzoom=true;
                } else if ( (std::get<0>(t)=="*") || (std::get<0>(t)=="XXX") ){
                    other_tags=true;
                } else if ( (std::get<0>(t)=="layer") ) {
                    layerint = true;
                } else {
                
                    if (std::get<1>(t)) {
                        point_tags.insert(std::make_pair(std::get<0>(t),point_tags.size()));
                        
                    }
                    if (std::get<2>(t)) {
                        line_tags.insert(std::make_pair(std::get<0>(t),line_tags.size()));
                        
                    }
                    if (std::get<3>(t)) {
                        poly_tags.insert(std::make_pair(std::get<0>(t),poly_tags.size()));
                        
                    }
                }
            }
            
            
            
        }

        virtual ~PackCsvBlocksBinaryImpl() {}

        
        std::string pack_point(std::shared_ptr<Point> pt, int64 tileqt) {
            std::string header(38,'\0');
            size_t pos=0;
            
            size_t numfields=3 + point_tags.size() + (other_tags?1:0)+(layerint?1:0)+(minzoom?1:0)+ 1;
            pos=write_int16(header, pos, numfields);

            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,pt->Id());
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,tileqt);
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,pt->Quadtree());
            
            std::string tags,othertags;
            std::tie(tags,othertags) = prep_tags_binary(point_tags, pt, other_tags);
            
            std::string wkb=pt->Wkb(true,true);
            
            std::string tail(4+othertags.size()+8+8+4, '\0');
            
            pos=0;
            if (other_tags) {
                if (othertags.size()>0) {
                    pos=write_int32(tail,pos,othertags.size());
                    pos=_write_data(tail,pos,othertags);
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            if (layerint) {
                if (pt->Layer()!=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,pt->Layer());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            if (minzoom) {
                if (pt->MinZoom()>=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,pt->MinZoom());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            pos=write_int32(tail,pos,wkb.size());
            tail.resize(pos);
            
            return header + tags + tail + wkb;
        }
        
        std::string pack_linestring(std::shared_ptr<Linestring> ln, int64 tileqt) {
            std::string header(38,'\0');
            size_t pos=0;
            
            size_t numfields=3 + line_tags.size() + (other_tags?1:0)+(layerint?1:0)+(minzoom?1:0)+ 3;
            pos=write_int16(header, pos, numfields);

            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,ln->Id());
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,tileqt);
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,ln->Quadtree());
            
            std::string tags,othertags;
            std::tie(tags,othertags) = prep_tags_binary(line_tags, ln, other_tags);
            
            std::string wkb=ln->Wkb(true,true);
            
            std::string tail(4+othertags.size()+8+8+4+8+12, '\0');
            
            pos=0;
            if (other_tags) {
                if (othertags.size()>0) {
                    pos=write_int32(tail,pos,othertags.size());
                    pos=_write_data(tail,pos,othertags);
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            if (layerint) {
                if (ln->Layer()!=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,ln->Layer());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            if (minzoom) {
                if (ln->MinZoom()>=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,ln->MinZoom());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            pos=write_int32(tail,pos,4);
            pos=write_int32(tail,pos,ln->ZOrder());
            
            pos=write_int32(tail,pos,8);
            pos=write_double(tail,pos,round(ln->Length()*10)/10);
            
            pos=write_int32(tail,pos,wkb.size());
            tail.resize(pos);
            
            return header + tags + tail + wkb;
        }
        
        std::string pack_simplepolygon(std::shared_ptr<SimplePolygon> py, int64 tileqt) {
            std::string header(42,'\0');
            size_t pos=0;
            
            size_t numfields=3 + poly_tags.size() + (other_tags?1:0)+(layerint?1:0)+(minzoom?1:0)+ 4;
            pos=write_int16(header, pos, numfields);

            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,py->Id());
            
            pos=write_int32(header,pos,-1);
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,tileqt);
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,py->Quadtree());
            
            std::string tags,othertags;
            std::tie(tags,othertags) = prep_tags_binary(poly_tags, py, other_tags);
            
            std::string wkb=py->Wkb(true,true);
            
            std::string tail(4+othertags.size()+8+8+4+8+12, '\0');
            
            pos=0;
            if (other_tags) {
                if (othertags.size()>0) {
                    pos=write_int32(tail,pos,othertags.size());
                    pos=_write_data(tail,pos,othertags);
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            if (layerint) {
                if (py->Layer()!=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,py->Layer());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            if (minzoom) {
                if (py->MinZoom()>=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,py->MinZoom());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            pos=write_int32(tail,pos,4);
            pos=write_int32(tail,pos,py->ZOrder());
            
            pos=write_int32(tail,pos,8);
            pos=write_double(tail,pos,round(10*py->Area())/10.0);
            
            pos=write_int32(tail,pos,wkb.size());
            tail.resize(pos);
            
            return header + tags + tail + wkb;
        }
        
        std::string pack_complicatedpolygon(std::shared_ptr<ComplicatedPolygon> py, int64 tileqt) {
            std::string header(46,'\0');
            size_t pos=0;
            
            size_t numfields=3 + poly_tags.size() + (other_tags?1:0)+(layerint?1:0)+(minzoom?1:0)+ 4;
            pos=write_int16(header, pos, numfields);

            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,py->Id()*-1);
            
            pos=write_int32(header,pos,4);
            pos=write_int32(header,pos,py->Part());
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,tileqt);
            
            pos=write_int32(header,pos,8);
            pos=write_int64(header,pos,py->Quadtree());
            
            std::string tags,othertags;
            std::tie(tags,othertags) = prep_tags_binary(poly_tags, py, other_tags);
            
            std::string wkb=py->Wkb(true,true);
            
            std::string tail(4+othertags.size()+8+8+4+8+12, '\0');
            
            pos=0;
            if (other_tags) {
                if (othertags.size()>0) {
                    pos=write_int32(tail,pos,othertags.size());
                    pos=_write_data(tail,pos,othertags);
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            if (layerint) {
                if (py->Layer()!=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,py->Layer());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            if (minzoom) {
                if (py->MinZoom()>=0) {
                    pos=write_int32(tail,pos,4);
                    pos=write_int32(tail,pos,py->MinZoom());
                } else {
                    pos=write_int32(tail,pos,-1);
                }
            }
            
            pos=write_int32(tail,pos,4);
            pos=write_int32(tail,pos,py->ZOrder());
            
            pos=write_int32(tail,pos,8);
            pos=write_double(tail,pos,round(10*py->Area())/10.0);
            
            pos=write_int32(tail,pos,wkb.size());
            tail.resize(pos);
            
            return header + tags + tail + wkb;
        }

        void add_to_csvblock(PrimitiveBlockPtr bl, std::shared_ptr<CsvBlock> res) {
            if (bl->size()==0) {
                return;
            }
            for (auto o : bl->Objects()) {

                if (o->Type()==ElementType::Point) {
                    
                    auto pt=std::dynamic_pointer_cast<Point>(o);
                    res->points.add(pack_point(pt,bl->Quadtree()));

                } else if (o->Type()==ElementType::Linestring) {
                    auto ln = std::dynamic_pointer_cast<Linestring>(o);
                    res->lines.add(pack_linestring(ln,bl->Quadtree()));
                } else if (o->Type()==ElementType::SimplePolygon) {
                    auto py = std::dynamic_pointer_cast<SimplePolygon>(o);
                    res->polygons.add(pack_simplepolygon(py,bl->Quadtree()));
                } else if (o->Type()==ElementType::ComplicatedPolygon) {
                    auto py = std::dynamic_pointer_cast<ComplicatedPolygon>(o);
                    res->polygons.add(pack_complicatedpolygon(py,bl->Quadtree()));
                }
            }
        }
       
        
        std::shared_ptr<CsvBlock> call(PrimitiveBlockPtr block) {
            if (!block) { return nullptr; }
            auto res = std::make_shared<CsvBlock>(true);
            add_to_csvblock(block,res);
            res->points.finish();
            res->lines.finish();
            res->polygons.finish();
            return res;
        }
        

    private:

        std::map<std::string,size_t> point_tags;
        std::map<std::string,size_t> line_tags;
        std::map<std::string,size_t> poly_tags;
        bool with_header;
        bool minzoom;
        bool other_tags;
        bool asjson;
        bool layerint;
        std::string point_header;
        std::string line_header;
        std::string poly_header;
};

std::shared_ptr<PackCsvBlocks> make_pack_csvblocks(const PackCsvBlocks::tagspec& tags, bool with_header, bool binary_format) {
    if (binary_format) {
        //throw std::domain_error("not implmented");
        return std::make_shared<PackCsvBlocksBinaryImpl>(tags);
    }
    return std::make_shared<PackCsvBlocksImpl>(tags, with_header,false);
}


class PostgisWriterImpl : public PostgisWriter {
    public:
        PostgisWriterImpl(
            const std::string& connection_string_,
            const std::string& table_prfx_,
            bool with_header_, bool as_binary_)
             : connection_string(connection_string_), table_prfx(table_prfx_), with_header(with_header_), as_binary(as_binary_), init(false), ii(0) {
            
            
            
        }
        
        virtual ~PostgisWriterImpl() {}
        
        
        virtual void finish() {
            if (init) {
                auto res = PQexec(conn,"commit");
                PQclear(res);
                PQfinish(conn);
            }
        }
        
        virtual void call(std::shared_ptr<CsvBlock> bl) {
            if (bl->points.size()>0) {
                copy_func(table_prfx+"point", bl->points.data_blob());
            }
            if (bl->lines.size()>0) {
                copy_func(table_prfx+"line", bl->lines.data_blob());
            }
            if (bl->polygons.size()>0) {
                copy_func(table_prfx+"polygon", bl->polygons.data_blob());
            }
            ii++;
            if ((ii % 17731)==0) {
                auto res = PQexec(conn,"commit");
                int r = PQresultStatus(res);
                PQclear(res);
                if (r!=PGRES_COMMAND_OK){
                    Logger::Message() << "postgiswriter: commit failed " << PQerrorMessage(conn);
                    PQfinish(conn);
                    throw std::domain_error("failed");
                }
                res = PQexec(conn,"begin");
                PQclear(res);
            }
        }
        
        
        
    private:
        size_t copy_func(const std::string& tab, const std::string& data) {
            if (!init) {
                conn = PQconnectdb(connection_string.c_str());
                if (!conn) {
                    Logger::Message() << "connection to postgresql failed [" << connection_string << "]";
                    throw std::domain_error("connection to postgressql failed");
                }
                auto res = PQexec(conn,"begin");
                if (PQresultStatus(res)!=PGRES_COMMAND_OK) {
                    Logger::Message() << "begin failed?? " <<  PQerrorMessage(conn);
                    PQclear(res);
                    PQfinish(conn);
                    throw std::domain_error("begin failed");
                    return 0;
                }
                PQclear(res);
                init=true;
            }
            
            
            std::string sql="COPY "+tab+" FROM STDIN";
            
            if (as_binary) {
                sql += " (FORMAT binary)";
            } else {
                sql += " csv QUOTE e'\x01' DELIMITER e'\x02'";
                if (with_header) {
                    sql += " HEADER";
                }
            }
            
            auto res = PQexec(conn,sql.c_str());

            if (PQresultStatus(res) != PGRES_COPY_IN) {
                Logger::Message() << "PQresultStatus != PGRES_COPY_IN [" << PQresultStatus(res) << "] " <<  PQerrorMessage(conn);
                
                PQclear(res);
                PQfinish(conn);
                init=false;
                throw std::domain_error("PQresultStatus != PGRES_COPY_IN");
                return 0;
            }

            PQputCopyData(conn,data.data(),data.size());

            int r = PQputCopyEnd(conn,nullptr);
            PQclear(res);
            if (r!=PGRES_COMMAND_OK) {
                Logger::Message() << "\n*****\ncopy failed [" << sql << "]" << PQerrorMessage(conn) << "\n" ;
                    
                return 0;
            }
            return 1;
        }
        std::string connection_string;        
        std::string table_prfx;      
        bool with_header;  
        bool as_binary;
        PGconn* conn;
        bool init;
        size_t ii;
};

std::shared_ptr<PostgisWriter> make_postgiswriter(
    const std::string& connection_string,
    const std::string& table_prfx,
    bool with_header, bool as_binary) {
    
    return std::make_shared<PostgisWriterImpl>(connection_string, table_prfx, with_header, as_binary);
}
    
        
std::function<void(std::shared_ptr<CsvBlock>)> make_postgiswriter_callback(
            const std::string& connection_string,
            const std::string& table_prfx,
            bool with_header, bool as_binary) {
            
    auto pw = make_postgiswriter(connection_string,table_prfx, with_header, as_binary);
    return [pw](std::shared_ptr<CsvBlock> bl) {
        if (!bl) {
            Logger::Message() << "PostgisWriter done";
            pw->finish();
        } else {
            pw->call(bl);
        }
    };
}
}}
