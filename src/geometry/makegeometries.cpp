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

#include "oqt/geometry/makegeometries.hpp"
#include "oqt/geometry/elements/ring.hpp"
#include "oqt/geometry/elements/point.hpp"
#include "oqt/geometry/elements/linestring.hpp"
#include "oqt/geometry/elements/simplepolygon.hpp"
#include "oqt/geometry/elements/complicatedpolygon.hpp"
#include "oqt/geometry/elements/waywithnodes.hpp"
#include <algorithm>

namespace oqt {
namespace geometry {



int64 get_zorder_value(const Tag& t) {
    if (t.key == "highway") {
        if (t.val == "motorway") { return 380; }
        if (t.val == "trunk") {return 370;}
        if (t.val == "primary") {return 360;}
        if (t.val == "secondary") { return 350;}
        if (t.val == "tertiary") { return 340;}
        if (t.val == "residential") { return 330;}
        if (t.val == "unclassified") { return 330;}
        if (t.val == "road") { return 330;}
        if (t.val == "living_street") { return 320;}
        if (t.val == "pedestrian") { return 310;}
        if (t.val == "raceway") { return 300;}
        if (t.val == "motorway_link") { return 240;}
        if (t.val == "trunk_link") { return 230;}
        if (t.val == "primary_link") { return 220;}
        if (t.val == "secondary_link") { return 210;}
        if (t.val == "tertiary_link") { return 200;}
        if (t.val == "service") { return 150;}
        if (t.val == "track") { return 110;}
        if (t.val == "path") { return 100;}
        if (t.val == "footway") { return 100;}
        if (t.val == "bridleway") { return 100;}
        if (t.val == "cycleway") { return 100;}
        if (t.val == "steps") { return 90;}
        if (t.val == "platform") { return 90;}
        if (t.val == "construction") { return 10;}
        return 0;
    }
    
    if (t.key=="railway") {
        if (t.val == "rail") { return 440; }
        if (t.val == "subway") { return 420; }
        if (t.val == "narrow_gauge") { return 420; }
        if (t.val == "light_rail") { return 420; }
        if (t.val == "funicular") { return 420; }
        if (t.val == "preserved") { return 420; }
        if (t.val == "monorail") { return 420; }
        if (t.val == "miniature") { return 420; }
        if (t.val == "turntable") { return 420; }
        if (t.val == "tram") { return 410; }
        if (t.val == "disused") { return 400; }
        if (t.val == "construction") { return 400; }
        if (t.val == "platform") { return 90; }
        return 0;
    }
    
    if (t.key=="aeroway") {
        if (t.val == "runway") { return 60; }
        if (t.val == "taxiway") { return 50; }
        return 0;
    }
    return 0;
}


int64 calc_zorder(const std::vector<Tag>& tags) {
    int64 result=0;
    for (const auto& t: tags) {
        int64 v = get_zorder_value(t);
        if (v>result) {
            result=v;
        }
    }
    return result;
}

/*
std::pair<std::vector<Tag>,bool> filter_tags(const style_info_map& style, const std::vector<Tag>& tags, bool passnode, bool passway, bool is_ring, const std::string& extra_tags_key) {
    std::vector<Tag> res;
    bool isf=false;
    bool hasarea=false;
    bool notarea=false;
    
    bool all_tags = !extra_tags_key.empty();
    std::vector<Tag> extra_tags;
    
    for (const auto& t : tags) {
        bool added=false;
        if ((t.key=="layer") && all_tags) {
            extra_tags.push_back(Tag{"orig_layer", t.val});
            added=true;
        } else if (style.count(t.key)>0) {

            auto ss=style.find(t.key)->second;
            if ((passnode && ss.IsNode) || (passway && ss.IsWay)) {
                if (ss.ValueList.empty() || (ss.ValueList.count(t.val)>0)) {
                    if (ss.OnlyArea && !is_ring) { continue; }

                    res.push_back(t);
                    added=true;
                    if (!isf && ss.IsFeature) {
                        isf=true;
                    }
                    if (ss.IsArea) {
                        hasarea=true;
                    }
                    if ((t.key=="area") && (t.val=="no")) {
                        notarea=true;
                    }
                }
            }
        }
        if (!added && all_tags) {
            extra_tags.push_back(t);
        }
    }
    
    if (!isf) {
        
        res.clear();
        return std::make_pair(res,false);
    }
    
    if (!extra_tags_key.empty() && !extra_tags.empty()) {
        if (extra_tags_key=="XXX") {
            for (auto t: extra_tags) {
                res.push_back(t);
            }
        } else {
            res.push_back(Tag{extra_tags_key, pack_tags(extra_tags)});
        }
    }
    
    
    if (notarea) {
        hasarea=false;
    }
    
    
    
    return std::make_pair(res,hasarea);
}

std::pair<std::vector<Tag>,int64> filter_node_tags(const style_info_map& style, const std::vector<Tag>& tags, const std::string& extra_tags_key) {
    auto tgs = filter_tags(style,tags,true,false,false,extra_tags_key).first;
    int64 layer=0;
    for (const auto& t: tags) {
        if (t.key=="layer") {
            try {
                layer = std::stoll(t.val);
            } catch (...) {
                //pass
            }
        }
    }
    return std::make_pair(tgs,layer);
}

std::tuple<std::vector<Tag>, bool,int64, int64> filter_way_tags(const style_info_map& style, const std::vector<Tag>& tags, bool is_ring, bool is_bp, const std::string& extra_tags_key) {
    std::vector<Tag> res; bool hasarea;
    
    std::tie(res,hasarea) = filter_tags(style, tags, false, true,is_ring,extra_tags_key);
    if (hasarea || is_bp) { hasarea=is_ring; }

    int64 zorder=0;
    zorder=calc_zorder(tags);
    
    int64 layer=0;
    for (const auto& t: tags) {
        if (t.key=="layer") {
            try {
                layer = std::stoll(t.val);
            } catch (...) {
                //pass
            }
        }
    }
    
    return std::make_tuple(res,hasarea,zorder,layer);
}




void copy_metadata(PrimitiveBlockPtr into, PrimitiveBlockPtr from) {
    into->SetQuadtree(from->Quadtree());
    into->SetStartDate(from->StartDate());
    into->SetEndDate(from->EndDate());
    into->SetFilePosition(from->FilePosition());
    into->SetFileProgress(from->FileProgress());
}


class MakeGeometry {
    public:
        MakeGeometry(const style_info_map& style_, const bbox& box_) : 
            style(style_), box(box_) {
        
            for (const auto& st: style) {
                if (st.second.IsOtherTags) {
                    extra_tag_key=st.first;
                }
            }
        }
        
        
        PrimitiveBlockPtr process_block(PrimitiveBlockPtr in) {
            
            auto result = std::make_shared<PrimitiveBlock>(in->Index(),0);
            copy_metadata(result, in);

            for (auto e : in->Objects()) {
                if(e->Type()==ElementType::Node) {
                    auto pt = process_node(std::dynamic_pointer_cast<Node>(e));
                    if (pt) {
                        result->add(pt);
                    }
                } else if (e->Type()==ElementType::WayWithNodes) {
                    auto geom = process_way(std::dynamic_pointer_cast<WayWithNodes>(e));
                    if (geom) {
                        result->add(geom);
                    }
                } else if (e->Type()==ElementType::Relation) {
                    //pass;
                } else {
                    result->add(e);
                }
            }
            return result;
            
        }
                
        ElementPtr process_node(std::shared_ptr<Node> nd) {
            if (!nd) { return nullptr; }
            if (!contains_point(box, nd->Lon(),nd->Lat())) {
                return nullptr;
            }
            
            std::vector<Tag> tgs; int64 ly;
            
            std::tie(tgs,ly) = filter_node_tags(style, nd->Tags(), extra_tag_key);
            if (tgs.empty()) {
                return nullptr;
            }
            
            return std::make_shared<Point>(nd, tgs,ly,-1);
        }
    
        ElementPtr process_way(std::shared_ptr<WayWithNodes> w) {
            if (!w) { return nullptr; }
            if (w->Refs().size()<2) { return nullptr; }
            if (!overlaps(box, w->Bounds())) { return nullptr; }
            
            std::vector<Tag> tgs; bool ispoly; int64 zo, ly;
            std::tie(tgs,ispoly,zo,ly) = filter_way_tags(style, w->Tags(), w->IsRing(), false, extra_tag_key);
            if (!tgs.empty()) {
                if (ispoly) {
                    return std::make_shared<SimplePolygon>(w, tgs,zo,ly,-1);
                } else {
                    return std::make_shared<Linestring>(w, tgs,zo,ly,-1);
                }
            }
            return nullptr;
        }
        
    private:
        style_info_map style;
        bbox box;
        std::string extra_tag_key;
};





PrimitiveBlockPtr make_geometries(const style_info_map& style, const bbox& box, PrimitiveBlockPtr in) {

    auto result = std::make_shared<PrimitiveBlock>(in->Index(),0);
    copy_metadata(result, in);
    
    
    std::string extra_tag_key;
    for (const auto& st: style) {
        if (st.second.IsOtherTags) {
            extra_tag_key=st.first;
        }
    }
    
    //bool all_tags = style.count("*")!=0;

    for (auto e : in->Objects()) {
        if(e->Type()==ElementType::Node) {

            std::vector<Tag> tgs; int64 ly;
        
            std::tie(tgs,ly) = filter_node_tags(style, e->Tags(), extra_tag_key);
            if (!tgs.empty()) {
                auto n = std::dynamic_pointer_cast<Node>(e);
                if (contains_point(box, n->Lon(),n->Lat())) {
                    result->add(std::make_shared<Point>(n, tgs,ly,-1));
                }
            }
        } else if (e->Type()==ElementType::WayWithNodes) {
            auto w = std::dynamic_pointer_cast<WayWithNodes>(e);
            if (w->Refs().size()<2) {
                continue;
            }

            if (!overlaps(box, w->Bounds())) {
                continue;
            }
            std::vector<Tag> tgs; bool ispoly; int64 zo, ly;
            std::tie(tgs,ispoly,zo,ly) = filter_way_tags(style, w->Tags(), w->IsRing(), false, extra_tag_key);
            if (!tgs.empty()) {
                if (ispoly) {
                    result->add(std::make_shared<SimplePolygon>(w, tgs,zo,ly,-1));
                } else {
                    result->add(std::make_shared<Linestring>(w, tgs,zo,ly,-1));
                }
            }
        } else if (e->Type()==ElementType::Relation) {
             //pass
        } else {

            result->add(e);
        }
    }


    return result;
}

*/




std::tuple<bool, std::vector<Tag>, int64> filter_tags(
    const std::set<std::string>& feature_keys,
    const std::set<std::string>& other_keys,
    bool all_other_keys,
    const std::vector<Tag> in_tags) {
        
    
        
    bool has_feature=false;
    std::vector<Tag> out_tags;
    int64 layer=0;
        
    if (in_tags.empty()) {
        return std::make_tuple(has_feature, out_tags, layer);
    }
    
    for (const auto& tg: in_tags) {
        
        if (feature_keys.count(tg.key)>0) {
            has_feature=true;
            out_tags.push_back(tg);
        } else if (all_other_keys || (other_keys.count(tg.key)>0)) {
            out_tags.push_back(tg);
        }
        if (tg.key=="layer") {
            try {
                layer = std::stoll(tg.val);
            } catch (...) {
                //pass
            }
        }
    }
    
    return std::make_tuple(has_feature, out_tags, layer);
}


bool check_polygon_tags(const std::map<std::string, PolygonTag>& polygon_tags, const std::vector<Tag>& tags) {
    
    
    for (const auto& tg: tags) {
        
        auto it = polygon_tags.find(tg.key);
        if (it != polygon_tags.end()) {
            
            if (it->second.type == PolygonTag::Type::All) {
                return true;
            } else if (it->second.type == PolygonTag::Type::Include) {
                if (it->second.values.count(tg.val)>0) {
                    return true;
                }
            } else if (it->second.type == PolygonTag::Type::Exclude) {
                if (it->second.values.count(tg.val)==0) {
                    return true;
                }
            }
        }
        
    }
    return false;
}
            
            
            
        
      


PrimitiveBlockPtr make_geometries(
    const std::set<std::string>& feature_keys,
    const std::map<std::string, PolygonTag>& polygon_tags,
    const std::set<std::string>& other_keys,
    bool all_other_keys,
    const bbox& box,
    PrimitiveBlockPtr in,
    std::function<bool(ElementPtr)> check_feat) {
        
    if (!in) { return in; }
    if (in->Objects().empty()) { return in; }
    
    auto result = std::make_shared<PrimitiveBlock>(in->Index(), in->size());
    result->CopyMetadata(in);
    
    for (auto obj: in->Objects()) {
        if (check_feat) {
            if (!check_feat(obj)) {
                continue;
            }
        }
        if (obj->Type()==ElementType::Node) {
            
            bool passes; std::vector<Tag> tags; int64 layer;
            std::tie(passes, tags, layer) = filter_tags(feature_keys, other_keys, all_other_keys, obj->Tags());
            if (passes) {
                auto n = std::dynamic_pointer_cast<Node>(obj);
                if (contains_point(box, n->Lon(),n->Lat())) {
                    result->add(std::make_shared<Point>(n, tags,layer,-1));
                }
            }
        } else if (obj->Type() == ElementType::WayWithNodes) {
            
            bool passes; std::vector<Tag> tags; int64 layer;
            std::tie(passes, tags, layer) = filter_tags(feature_keys, other_keys, all_other_keys, obj->Tags());
            
            if (passes) {
                auto w = std::dynamic_pointer_cast<WayWithNodes>(obj);
                if (w->Refs().size()<2) {
                    continue;
                }

                if (!overlaps(box, w->Bounds())) {
                    continue;
                }
                bool is_poly = (w->IsRing() && check_polygon_tags(polygon_tags, tags));
            
                if (is_poly) {
                    result->add(std::make_shared<SimplePolygon>(w, tags,0,layer,-1));
                } else {
                    int64 z_order = calc_zorder(tags);
                    result->add(std::make_shared<Linestring>(w, tags, z_order, layer, -1));
                }
            }
        } else if (obj->Type()==ElementType::Relation) {
             //pass
        } else {

            result->add(obj);
        }
    }


    return result;
}

        
        
        
        
    
        




bbox get_bound(ElementPtr o) {
    
    auto gg = std::dynamic_pointer_cast<BaseGeometry>(o);
    if (gg) {
        return gg->Bounds();
    }

    return bbox{0,0,0,0};
}


size_t recalculate_quadtree(PrimitiveBlockPtr block, uint64 maxdepth, double buf) {
    if (!block || block->Objects().empty()) { return 0; }
    size_t c=0;
    for (auto o : block->Objects()) {
        auto b = get_bound(o);
        int64 q = quadtree::calculate(b.minx,b.miny,b.maxx,b.maxy,buf,maxdepth);
        if (q!=o->Quadtree()) {
            c++;
            o->SetQuadtree(q);
        }
    }
    return c;
}






class GeometryProcess : public BlockHandler {
    public:
        /*GeometryProcess(const style_info_map& style_, const bbox& box_, bool recalc_, std::shared_ptr<FindMinZoom> fmz_)
            : style(style_), box(box_),makegeometry(makegeometry_), recalc(recalc_), fmz(fmz_) {}*/
            
        GeometryProcess(
            const std::set<std::string>& feature_keys_,
            const std::map<std::string, PolygonTag>& polygon_tags_,
            const std::set<std::string>& other_keys_,
            bool all_other_keys_,
            const bbox& box_,
            bool recalc_,
            std::shared_ptr<FindMinZoom> fmz_,
            int64 max_min_zoom_level_)
            
            
            :feature_keys(feature_keys_), polygon_tags(polygon_tags_),
            other_keys(other_keys_), all_other_keys(all_other_keys_),
            box(box_), recalc(recalc_), fmz(fmz_), max_min_zoom_level(max_min_zoom_level_) {}
            

        virtual primblock_vec process(primblock_ptr bl) {
            std::function<bool(ElementPtr)> check_feat;
            if (max_min_zoom_level>0) {
                check_feat = [this](ElementPtr e) { return this->fmz->check_feature(e, this->max_min_zoom_level); };
            }
            auto res = make_geometries(feature_keys, polygon_tags, other_keys, all_other_keys, box, bl, check_feat);
            
            if (recalc) {
                recalculate_quadtree(res, 18, fmz ? 0 : 0.05);
            }
            if (fmz) {
                calculate_minzoom(res, fmz,max_min_zoom_level);
            }
            return primblock_vec(1, res);
        }
        virtual ~GeometryProcess() {}
    
    
    private:
        std::set<std::string> feature_keys;
        std::map<std::string, PolygonTag> polygon_tags;
        std::set<std::string> other_keys;
        bool all_other_keys;
        bbox box;
        bool recalc;
        std::shared_ptr<FindMinZoom> fmz;
        int64 max_min_zoom_level;
};


std::shared_ptr<BlockHandler> make_geometryprocess(
    const std::set<std::string>& feature_keys,
    const std::map<std::string, PolygonTag>& polygon_tags,
    const std::set<std::string>& other_keys,
    bool all_other_keys,
    const bbox& box,
    bool recalc,
    std::shared_ptr<FindMinZoom> fmz,
    int64 max_min_zoom_level){
        
    return std::make_shared<GeometryProcess>(feature_keys, polygon_tags, other_keys, all_other_keys, box, recalc, fmz, max_min_zoom_level);
    
}

    

void calculate_minzoom(PrimitiveBlockPtr block, std::shared_ptr<FindMinZoom> minzoom, int64 max_min_zoom_level) {
    std::vector<ElementPtr> out;
    
    for (auto ele: block->Objects()) {
        int64 mz = minzoom->calculate(ele);
        
        if (mz>=0) {
            if ((max_min_zoom_level > 0) && (mz > max_min_zoom_level)) {
                continue;
            }
            
            if ((ele->Quadtree()&31) > mz) {
                ele->SetQuadtree(quadtree::round(ele->Quadtree(),mz));
            }
            if (ele->Type()==ElementType::Point) { std::dynamic_pointer_cast<Point>(ele)->SetMinZoom(mz); }
            if (ele->Type()==ElementType::Linestring) { std::dynamic_pointer_cast<Linestring>(ele)->SetMinZoom(mz); }
            if (ele->Type()==ElementType::SimplePolygon) { std::dynamic_pointer_cast<SimplePolygon>(ele)->SetMinZoom(mz); }
            if (ele->Type()==ElementType::ComplicatedPolygon) { std::dynamic_pointer_cast<ComplicatedPolygon>(ele)->SetMinZoom(mz); }
            if (max_min_zoom_level > 0) {
                out.push_back(ele);
            }
        }
    }
    if (max_min_zoom_level > 0) {
        block->Objects().swap(out);
    }
}

}}
