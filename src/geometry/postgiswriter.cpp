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

#include <sstream>
#include <iostream>
#include <map>
#include <postgresql/libpq-fe.h>

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
            strm << "\\n";
        } else if (c=='"') {
            strm << '\\' << '"';
        } else if (c=='\t') {
            strm << '\\' << 't';
        } else if (c=='\r') {
            strm << '\\' << 'r';
        } else if (c=='\\') {
            strm << "\\\\";
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
        json_quotestring(strm, t.key);
        strm << "=>";
        json_quotestring(strm, t.val);
        isf=false;
    }
    return strm.str();
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
            return pack_jsontags(others);
        } else {
            return pack_hstoretags(others);
        }
    }
    return std::string();            
}

std::string as_hex(const std::string& str) {
    std::stringstream ss;
    for (unsigned char c : str) {
        ss << std::hex << std::setfill('0') << std::setw(2) << (size_t) c;
    }

    return ss.str();

}


void csv_rows::add(const std::string row) {
    if (poses.empty()) { poses.push_back(0); }

    data+=row;
    data+="\n";
    poses.push_back(data.size());
}

std::string csv_rows::at(int i) {
    if (i<0) { i += size(); }
    if (i >= size()) { throw std::range_error("out of range"); }
    size_t p = poses[i];
    size_t q = poses[i+1];
    return data.substr(p,q-p);
}

int csv_rows::size() {
    if (poses.empty()) { return 0; }
    return poses.size()-1;
}





class pack_csvblocks_impl : public pack_csvblocks {
    public:
        pack_csvblocks_impl(
            const pack_csvblocks::tagspec& tags, bool with_header_, bool asjson_) : with_header(with_header_), minzoom(false), other_tags(false), asjson(asjson_),layerint(false) {
                
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
                
                point_ss << "way";
                line_ss << "way";
                poly_ss << "way";
                
            
                point_header = point_ss.str();
                line_header = line_ss.str();
                poly_header = poly_ss.str();
            }
            
        }

        void add_to_csvblock(PrimitiveBlockPtr bl, std::shared_ptr<csv_block> res) {
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
                            quotestring(ss,other);
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
                    
                    if (with_header && (res->polygons.size()==0)) {
                        res->polygons.add(point_header);
                    }
                    res->polygons.add(ss.str());
                }
            }
        }
        virtual ~pack_csvblocks_impl() {}
        
        std::shared_ptr<csv_block> call(PrimitiveBlockPtr block) {
            if (!block) { return nullptr; }
            auto res = std::make_shared<csv_block>();
            add_to_csvblock(block,res);
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

std::shared_ptr<pack_csvblocks> make_pack_csvblocks(const pack_csvblocks::tagspec& tags, bool with_header) {
    return std::make_shared<pack_csvblocks_impl>(tags, with_header,false);
}


class PostgisWriterImpl : public PostgisWriter {
    public:
        PostgisWriterImpl(
            const std::string& connection_string_,
            const std::string& table_prfx_,
            bool with_header_)
             : connection_string(connection_string_), table_prfx(table_prfx_), with_header(with_header_), init(false), ii(0) {
            
            
            
        }
        
        virtual ~PostgisWriterImpl() {}
        
        
        virtual void finish() {
            if (init) {
                auto res = PQexec(conn,"commit");
                PQclear(res);
                PQfinish(conn);
            }
        }
        
        virtual void call(std::shared_ptr<csv_block> bl) {
            if (bl->points.size()>0) {
                copy_func(table_prfx+"point", bl->points.data);
            }
            if (bl->lines.size()>0) {
                copy_func(table_prfx+"line", bl->lines.data);
            }
            if (bl->polygons.size()>0) {
                copy_func(table_prfx+"polygon", bl->polygons.data);
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
            
            
            std::string sql="COPY "+tab+" FROM STDIN csv QUOTE e'\x01' DELIMITER e'\x02'";
            if (with_header) {
                sql += " HEADER";
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
        PGconn* conn;
        bool init;
        size_t ii;
};

std::shared_ptr<PostgisWriter> make_postgiswriter(
    const std::string& connection_string,
    const std::string& table_prfx,
    bool with_header) {
    
    return std::make_shared<PostgisWriterImpl>(connection_string, table_prfx, with_header);
}
    
        
std::function<void(std::shared_ptr<csv_block>)> make_postgiswriter_callback(
            const std::string& connection_string,
            const std::string& table_prfx,
            bool with_header) {
            
    auto pw = make_postgiswriter(connection_string,table_prfx, with_header);
    return [pw](std::shared_ptr<csv_block> bl) {
        if (!bl) {
            Logger::Message() << "PostgisWriter done";
            pw->finish();
        } else {
            pw->call(bl);
        }
    };
}
}}
