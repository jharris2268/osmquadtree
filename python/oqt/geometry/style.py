import json
from . import _geometry


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
    
highway_prio = {
    'footway':1,'cycleway':1,'bridleway': 1,
    'path':1, 'steps': 1, 'pedestrian': 1,
    'service': 2,'track':2,'byway': 2,
    'living_street':3, 'residential':3,'road':3,'unclassified':3,
    'tertiary':4,'tertiary_link':4,
    'secondary':5,'secondary_link':5,
    'primary':6,'primary_link':6,
    'trunk':7, 'trunk_link':7,
    'motorway':8,'motorway_link':8}



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
        
class ParentRelation:
    __slots__ = ['relation_tags','relation_key', 'operation']
    def __init__(self, relation_tags, relation_key, operation):
        self.relation_tags = relation_tags
        self.relation_key = relation_key
        self.operation = operation
    
    def to_json(self):
        return {'relation_tags': self.relation_tags, 'relation_key': self.relation_key, 'operation': self.operation}
    
    def to_cpp(self, key):
        pass
    
    @staticmethod
    def from_json(jj):
        return ParentRelation(jj['relation_tags'],jj['relation_key'], jj['operation'])

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__,", ".join("%s: %s" % (k,repr(getattr(self,k))) for k in self.__slots__),)


default_parent_relations = {
'min_admin_level': ParentRelation({'type':'boundary', 'boundary':'administrative'}, 'admin_level', 'min'),
'max_admin_level': ParentRelation({'type':'boundary', 'boundary':'administrative'}, 'admin_level', 'max'),
'bus_routes': ParentRelation({'type':'route', 'route':'bus'}, 'ref', 'list'),
'bicycle_routes': ParentRelation({'type':'route', 'route':'bicycle'}, 'ref', 'list'),
}


def write_entry(obj, key, val, ident, last=False):
    print >>obj, '%s"%s": %s%s' % ('  '*ident, key, json.dumps(val), '' if last else ',')

def write_entries(obj, key, vals, ident, last=False):
    print >>obj, '%s"%s": {' % ('  '*ident, key,)
    
    li = len(vals)-1
    for i,(k,v) in enumerate(sorted(vals.iteritems())):
        write_entry(obj, k, v, ident+1, i==li)
    
    print >>obj, '%s}%s' % ('  '*ident, '' if last else ',')


class GeometryStyle:
    __slots__ = ['keys','parent_tags','parent_relations','multipolygons','boundary_relations','other_tags']
    def __init__(self,
            keys=None,
            parent_tags=None,
            parent_relations=None,
            multipolygons=True,
            boundary_relations=True,
            other_tags=True):
        
        self.keys = default_keys if keys is None else keys
        self.parent_tags = default_parent_tags if parent_tags is None else parent_tags
        self.parent_relations = default_parent_relations if parent_relations is None else parent_relations
        self.multipolygons = multipolygons
        self.boundary_relations=boundary_relations
        self.other_tags=other_tags
        
    
    def to_json(self):
        
        result = {}
        result['keys'] = dict((k, v.to_json()) for k,v in self.keys.iteritems())
        result['parent_tags'] = dict((k, v.to_json()) for k,v in self.parent_tags.iteritems())
        result['parent_relations'] = dict((k, v.to_json()) for k,v in self.parent_relations.iteritems())
        result['multipolygons']=self.multipolygons
        result['boundary_relations']=self.boundary_relations
        result['other_tags']=self.other_tags
        return result
    
    def dump(self, obj, pretty=False):
        
        
        
        if pretty:
            res = self.to_json()
            
            
            print >>obj, "{"
            write_entries(obj, 'keys', res['keys'], 1)
            write_entries(obj, 'parent_tags', res['parent_tags'], 1)
            write_entries(obj, 'parent_relations', res['parent_relations'], 1)
            write_entry(obj, 'multipolygons',self.multipolygons,1)
            write_entry(obj, 'boundary_relations',self.boundary_relations,1)
            write_entry(obj, 'other_tags',self.other_tags,1,last=True)
            print >>obj, "}"
            
        else:
            json.dump(self.to_json(), obj)
    
    
    @staticmethod
    def from_json(jj):
        
        keys = dict((k, KeySpec.from_json(v)) for k,v in jj['keys'].iteritems())
        parent_tags = dict((k, ParentTag.from_json(v)) for k,v in jj['parent_tags'].iteritems())
        parent_relations = dict((k, ParentRelation.from_json(v)) for k,v in jj['parent_relations'].iteritems())
        
        return GeometryStyle(keys, parent_tags, parent_relations, jj['multipolygons'], jj['boundary_relations'], jj['other_tags'])
    
    @staticmethod
    def load(obj):
        return GeometryStyle.from_json(json.load(obj))
    
    
    def set_params(self, params, add_min_zoom=False):
        
        style = dict((k, v.to_cpp()) for k,v in self.keys.iteritems())
        if self.other_tags:
            style['XXX']=_geometry.StyleInfo(IsFeature=False,IsArea=False,IsWay=True,IsNode=True,IsOtherTags=True)
        
        if add_min_zoom:
            style['minzoom'] = _geometry.StyleInfo(IsFeature=False,IsArea=False,IsWay=True,IsNode=True,IsOtherTags=False)
        
        parent_tag_spec=[]
        for k,v in self.parent_tags.iteritems():
            if not self.other_tags:
                style[k] = _geometry.StyleInfo(IsFeature=False,IsArea=False,IsWay=False,IsNode=True,IsOtherTags=False)
            parent_tag_spec+=v.to_cpp(k)
        
        if not self.other_tags or True:
            for k,v in self.parent_relations.iteritems():
                style[k] = _geometry.StyleInfo(IsFeature=False,IsArea=False,IsWay=True,IsNode=False,IsOtherTags=False)
            
        params.style = style
        if self.parent_relations:
            params.add_rels=True
        
        
        if parent_tag_spec:
            params.parent_tag_spec = parent_tag_spec
        params.add_mps = self.multipolygons
        
    
    
    
        
    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__,", ".join("%s: %.50s" % (k,repr(getattr(self,k))) for k in self.__slots__),)
    
    
    
