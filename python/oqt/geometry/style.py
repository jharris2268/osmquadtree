#-----------------------------------------------------------------------
#
# This file is part of osmquadtree
#
# Copyright (C) 2018 James Harris
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#-----------------------------------------------------------------------

from __future__ import print_function

import json
from . import _geometry
import oqt.elements


class KeySpec:
    __slots__ = ['IsNode','IsWay','IsFeature','IsArea','OnlyArea']
    def __init__(self, **kw):
        
        for s in self.__slots__:
            if s in kw:
                setattr(self, s, kw[s])
            else:
                setattr(self, s, False)
    
    def to_json(self):
        return dict((s, getattr(self, s)) for s in self.__slots__)
    
    def to_cpp(self):
        res =  _geometry.StyleInfo(IsFeature=self.IsFeature, IsArea=self.IsArea, IsWay=self.IsWay, IsNode=self.IsNode, IsOtherTags=False)
        res.OnlyArea=self.OnlyArea
        
        return res
    
    @staticmethod
    def from_json(jj):
        return KeySpec(**jj)
    
    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__,", ".join("%s: %s" % (k,repr(getattr(self,k))) for k in self.__slots__),)
    
default_keys = {
'access': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'addr:housename': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'addr:housenumber': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'addr:interpolation': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'admin_level': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'aerialway': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'aeroway': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'amenity': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'barrier': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'bicycle': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'boundary': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'bridge': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'building': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
#'bus_routes': KeySpec(IsArea=False, IsFeature=False, IsNode=False, IsWay=True, OnlyArea=False),
'construction': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'covered': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'embankment': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'foot': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'highway': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'historic': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'horse': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'junction': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'landuse': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'layer': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'leisure': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'lock': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'man_made': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'military': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'name': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'natural': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'oneway': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'place': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'power': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'railway': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'ref': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'religion': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'route': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'service': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'shop': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'surface': KeySpec(IsArea=False, IsFeature=False, IsNode=True, IsWay=True, OnlyArea=False),
'tourism': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'tracktype': KeySpec(IsArea=False, IsFeature=False, IsNode=False, IsWay=True, OnlyArea=False),
'tunnel': KeySpec(IsArea=False, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'water': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
'waterway': KeySpec(IsArea=True, IsFeature=True, IsNode=True, IsWay=True, OnlyArea=False),
}

default_feature_keys = ['aerialway',
    'aeroway',
    'amenity',
    'barrier',
    'boundary',
    'bridge',
    'building',
    'construction',
    'embankment',
    'highway',
    'historic',
    'junction',
    'landuse',
    'leisure',
    'lock',
    'man_made',
    'military',
    'natural',
    'place',
    'power',
    'railway',
    'route',
    'service',
    'shop',
    'tourism',
    'tunnel',
    'water',
    'waterway',
]

default_other_keys = None
standard_other_keys = [
    'access',
    'addr:housename',
    'addr:housenumber',
    'addr:interpolation',
    'admin_level',
    'bicycle',
    'covered',
    'foot',
    'horse',
    'layer',
    'name',
    'oneway',
    'ref',
    'religion',
    'surface',
    'tracktype'
]


default_polygon_tags_old = {
    'aeroway': ['exclude',['taxiway']],
    'amenity': ['all',None],
    'area': ['all',None],
    'area:highway': ['all',None],
    'barrier': ['include',['city_wall', 'ditch', 'wall', 'spikes']],
    'boundary': ['all',None],
    'building': ['all',None],
    'building:part': ['all',None],
    'golf': ['all',None],
    'highway': ['include',['services', 'rest_area', 'escape', 'elevator']],
    'historic': ['all',None],
    'landuse': ['all',None],
    'leisure': ['all',None],
    'man_made': ['exclude',['cutline', 'embankment', 'pipeline']],
    'military': ['all',None],
    'natural': ['exclude',['coastline', 'cliff', 'ridge', 'arete', 'tree_row']],
    'office': ['all',None],
    'place': ['all',None],
    'power': ['include',['plant', 'substation', 'generator', 'transformer']],
    'public_transport': ['all',None],
    'railway': ['include',['station', 'turntable', 'roundhouse', 'platform']],
    'shop': ['all',None],
    'tourism': ['all',None],
    'waterway': ['include',['riverbank', 'dock', 'boatyard', 'dam']],
}

default_polygon_tags = {
    'aeroway': {'exclude': ['taxiway']},
    'amenity': 'all',
    'area': 'all',
    'area:highway': 'all',
    'barrier': {'include': ['city_wall', 'ditch', 'wall', 'spikes']},
    'boundary': 'all',
    'building': 'all',
    'building:part': 'all',
    'golf': 'all',
    'highway': {'include': ['services', 'rest_area', 'escape', 'elevator']},
    'historic': 'all',
    'landuse': 'all',
    'leisure': 'all',
    'man_made': {'exclude': ['cutline', 'embankment', 'pipeline']},
    'military': 'all',
    'natural': {'exclude': ['coastline', 'cliff', 'ridge', 'arete', 'tree_row']},
    'office': 'all',
    'place': 'all',
    'power': {'include': ['plant', 'substation', 'generator', 'transformer']},
    'public_transport': 'all',
    'railway': {'include': ['station', 'turntable', 'roundhouse', 'platform']},
    'shop': 'all',
    'tourism': 'all',
    'waterway': {'include': ['riverbank', 'dock', 'boatyard', 'dam']},
}



highway_prio_old = {
    'footway':1,'cycleway':1,'bridleway': 1,
    'path':1, 'steps': 1, 'pedestrian': 1,
    'service': 2,'track':2,'byway': 2,
    'living_street':3, 'residential':3,'road':3,'unclassified':3,
    'tertiary':4,'tertiary_link':4,
    'secondary':5,'secondary_link':5,
    'primary':6,'primary_link':6,
    'trunk':7, 'trunk_link':7,
    'motorway':8,'motorway_link':8}

highway_prio = dict((h,i) for i,h in enumerate([
    'footway','cycleway','bridleway',
    'path','steps','pedestrian',
    'service','track','byway',
    'living_street','residential','road','unclassified',
    'tertiary','tertiary_link',
    'secondary','secondary_link',
    'primary','primary_link',
    'trunk','trunk_link',
    'motorway','motorway_link',
    'siding','rail',
]))


class ParentTag:
    __slots__ = ['node_keys', 'way_key', 'way_priority']
    def __init__(self, node_keys, way_key, way_priority):
        self.node_keys=node_keys
        self.way_key=way_key
        self.way_priority=way_priority
    
    def to_cpp(self, key):
        ans=[]
        for nk in self.node_keys:
            ans.append(_geometry.ParentTagSpec(nk, key, self.way_key, self.way_priority))
        return ans
    
    def to_json(self):
        return {'node_keys': self.node_keys, 'way_key': self.way_key, 'way_priority': self.way_priority}
        
    @staticmethod
    def from_json(jj):
        return ParentTag(jj['node_keys'], jj['way_key'], jj['way_priority'])
    
    
    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__,", ".join("%s: %s" % (k,repr(getattr(self,k))) for k in self.__slots__),)
    
default_parent_tags = {
'parent_highway': ParentTag(['highway','railway'],'highway',highway_prio),
'parent_service': ParentTag(['highway'],'service',{}),
}
    
class RelationTag:
    __slots__ = ['target_key', 'source_filter', 'source_key', 'type']
    def __init__(self, target_key, source_filter, source_key, type):
        self.target_key = target_key
        self.source_filter = source_filter
        self.source_key = source_key
        self.type=type
    
    def to_json(self):
        return {'target_key': self.target_key, 'source_filter': self.source_filter, 'source_key': self.source_key, 'type': self.type}
    
    def to_cpp(self):
        return _geometry.RelationTagSpec(
            self.target_key,
            [oqt.elements.Tag(k,v) for k,v in self.source_filter.items()],
            self.source_key,
            _geometry.RelationTagSpec.Type.Min if self.type=='min' else
                _geometry.RelationTagSpec.Type.Max if self.type=='max' else
                _geometry.RelationTagSpec.Type.List if self.type=='list' else
                None
        )
        
    
    @staticmethod
    def from_json(jj):
        return RelationTag(jj['target_key'],jj['source_filter'], jj['source_key'], jj['type'])

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__,", ".join("%s: %s" % (k,repr(getattr(self,k))) for k in self.__slots__),)


default_relation_tag_spec = [
RelationTag('min_admin_level',   {'type':'boundary', 'boundary': 'administrative'}, 'admin_level', 'min'),
RelationTag('max_admin_level',   {'type':'boundary', 'boundary': 'administrative'}, 'admin_level', 'max'),
RelationTag('bus_routes',        {'type':'route',    'route':'bus'},                'ref', 'list'),
RelationTag('bicycle_routes',    {'type':'route',    'route':'bicycle'},            'ref', 'list'),
]


def write_entry(obj, key, val, ident, last=False):
    print('%s"%s": %s%s' % ('  '*ident, key, json.dumps(val), '' if last else ','), file=obj)

def write_entries(obj, key, vals, ident, last=False):
    print('%s"%s": {' % ('  '*ident, key,),file=obj)
    
    li = len(vals)-1
    for i,(k,v) in enumerate(sorted(vals.items())):
        write_entry(obj, k, v, ident+1, i==li)
    
    print('%s}%s' % ('  '*ident, '' if last else ','),file=obj)


class GeometryStyle:
    __slots__ = ['feature_keys','other_keys','polygon_tags','parent_tags','relation_tag_spec','multipolygons','boundary_relations']
    def __init__(self,
            feature_keys=None,
            other_keys=None,
            polygon_tags=None,
            parent_tags=None,
            relation_tag_spec=None,
            multipolygons=True,
            boundary_relations=True):
        
        self.feature_keys = default_feature_keys if feature_keys is None else feature_keys
        self.other_keys = default_other_keys if other_keys is None else other_keys
        self.polygon_tags = default_polygon_tags if polygon_tags is None else polygon_tags
        self.parent_tags = default_parent_tags if parent_tags is None else parent_tags
        self.relation_tag_spec = default_relation_tag_spec if relation_tag_spec is None else relation_tag_spec
        self.multipolygons = multipolygons
        self.boundary_relations=boundary_relations
        
        
    
    def to_json(self):
        
        result = {}
        result['feature_keys'] = self.feature_keys
        result['polygon_tags'] = self.polygon_tags
        result['other_keys'] = self.other_keys
        result['parent_tags'] = dict((k, v.to_json()) for k,v in self.parent_tags.items())
        result['relation_tag_spec'] = [p.to_json() for p in self.relation_tag_spec]
        result['multipolygons']=self.multipolygons
        result['boundary_relations']=self.boundary_relations
        return result
    
    def dump(self, obj, pretty=False):
        
        
        
        if pretty:
            json.dump(self.to_json(), obj, indent=4, sort_keys=True)
            
            #res = self.to_json()
            
            
            #print("{",file=obj)
            #write_entries(obj, 'feature_keys', json.dumps(res['feature_keys']), 1)
            #write_entries(obj, 'polygon_tags', res['polygon_tags'], 1)
            #write_entries(obj, 'other_keys', res['other_keys'], 1)
            #write_entries(obj, 'parent_tags', res['parent_tags'], 1)
            #write_entries(obj, 'parent_relations', res['parent_relations'], 1)
            #write_entry(obj, 'multipolygons',self.multipolygons,1)
            #write_entry(obj, 'boundary_relations',self.boundary_relations,1,last=True)
            #print("}",file=obj)
            
        else:
            json.dump(self.to_json(), obj)
    
    
    @staticmethod
    def from_json(jj):
        
        
        parent_tags = dict((k, ParentTag.from_json(v)) for k,v in jj['parent_tags'].items())
        relation_tag_spec = [RelationTag.from_json(v) for v in jj['relation_tag_spec']]
        
        return GeometryStyle(jj['feature_keys'], jj['other_keys'], jj['polygon_tags'], parent_tags, relation_tag_spec, jj['multipolygons'], jj['boundary_relations'])
    
    @staticmethod
    def load(obj):
        return GeometryStyle.from_json(json.load(obj))
    
    
    def set_params(self, params, add_min_zoom=False):
        
        
        params.all_other_keys=True
        
        params.feature_keys = set(self.feature_keys)
        
        polygon_tags = {}
        for key,tt in self.polygon_tags.items():
            if tt == 'all':
                polygon_tags[key] = _geometry.PolygonTag(key, _geometry.PolygonTag.Type.All, set([]))
            elif 'include' in tt:
                polygon_tags[key] = _geometry.PolygonTag(key, _geometry.PolygonTag.Type.Include, set(tt['include']))
            elif 'exclude' in tt:
                polygon_tags[key] = _geometry.PolygonTag(key, _geometry.PolygonTag.Type.Exclude, set(tt['exclude']))
                
        params.polygon_tags=polygon_tags
        
        parent_tag_spec=[]
        if self.parent_tags:
            for k,v in self.parent_tags.items():
                parent_tag_spec += v.to_cpp(k)
            
        params.parent_tag_spec = parent_tag_spec
        
        
        if self.relation_tag_spec:
            params.relation_tag_spec = [p.to_cpp() for p in self.relation_tag_spec]
        
        
        if not self.other_keys is None:
            params.all_other_keys=False
            other_keys = [k for k in self.other_keys]
                        
            if add_min_zoom:
                other_keys.append('min_zoom')
            
            for k in self.parent_tags:
                other_keys.append(p.node_key)
            
            
            for p in self.relation_tag_spec.items():
                other_keys.append(p.target_key)
        
            params.other_keys=set(other_keys)
        
        
        
        params.add_multipolygons = self.multipolygons
        params.add_boundary_polygons = self.boundary_relations
        
    
    
    
        
    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__,", ".join("%s: %.50s" % (k,repr(getattr(self,k))) for k in self.__slots__),)
