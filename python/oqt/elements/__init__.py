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

from . import _elements
from ._elements import Node,Way,Relation,ElementInfo,Tag,Member,ElementType,changetype,PrimitiveBlock,MinimalBlock
from ._elements import quadtree_bbox, overlaps_quadtree, quadtree_round, quadtree_string, quadtree_tuple, quadtree_common

def find_tag(obj, idx):
    for tg in obj.Tags:
        if tg.key==idx:
            return tg.val
    return None

class quadtree:
    def __init__(self, q):
        self.q=q

    @classmethod
    def from_tuple(cls, a,b,c):
        return cls(_block.quadtree_from_xyz(a,b,c))
    @classmethod
    def from_string(cls, s):
        return cls(_block.quadtree_from_string(s))

    @classmethod
    def calculate(cls, minx, miny, maxx, maxy, buffer=0.05, maxlevel=18):
        return cls(_block.calculate(minx,miny,maxx,maxy,buffer,maxlevel))


    def __int__(self):
        return self.q

    def __str__(self):
        return _elements.quadtree_string(self.q)

    def __repr__(self):
        return "quadtree(%s)" % str(self)

    def tuple(self):
        return _elements.quadtree_tuple(self.q)

    def common(self, other):
        return quadtree(_elements.quadtree_common(self,other))

    def round(self, nl):
        return quadtree(_elements.quadtree_round(self.q, nl))

    def bbox(self, buffer=0.05):
        return _elements.quadtree_bbox(self.q, buffer)
       
    def __eq__(self,other):
        return self.q==other.q
    def __ne__(self,other):
        return self.q!=other.q
    def __lt__(self,other):
        return self.q<other.q





Node.__repr__ = lambda n: "Node(%10d %0.40s %.40s % 8d % 8d %-18s %d)" % (n.Id,repr(n.Info),repr(n.Tags),n.Lon,n.Lat,n.Quadtree,n.ChangeType)
Node.__unicode__ = lambda n: u"Node(%10d %s %s % 8d % 8d %-18s %d)" % (n.Id,n.Info,n.Tags,n.Lon,n.Lat,n.Quadtree,n.ChangeType)
Node.tuple = property(lambda n: (0, n.ChangeType, n.Id,n.Info.tuple,[t.tuple for t in n.Tags], n.Lon,n.Lat, n.Quadtree))


#_oqt.Way._Quadtree=_oqt.Way.Quadtree
#_oqt.Way.Quadtree=property(lambda q: quadtree(q._Quadtree))
Way.__repr__ = lambda w: "Way(%10d %0.40s %.40s %.20s %-18s %d)" % (w.Id,repr(w.Info),repr(w.Tags),w.Refs,w.Quadtree,w.ChangeType)
Way.__unicode__ = lambda w: u"Way(%10d %s %s %s %-18s %d)" % (w.Id,w.Info,w.Tags,w.Refs,w.Quadtree,w.ChangeType)
Way.tuple = property(lambda w: (1, w.ChangeType, w.Id,w.Info.tuple,[t.tuple for t in w.Tags], w.Refs, w.Quadtree))

#_oqt.Relation._Quadtree=_oqt.Relation.Quadtree
#_oqt.Relation.Quadtree=property(lambda q: quadtree(q._Quadtree))
Relation.__repr__ = lambda r: "Relation(%10d %0.40s %.40s %.20s %-18s %d)" % (r.Id,repr(r.Info),repr(r.Tags),r.Members,r.Quadtree,r.ChangeType)
Relation.__unicode__ = lambda r: u"Relation(%10d %s %s %s %-18s %d)" % (r.Id,r.Info,r.Tags,r.Members,r.Quadtree,r.ChangeType)
Relation.tuple = property(lambda r: (2, r.ChangeType, r.Id,r.Info.tuple,[t.tuple for t in r.Tags], [m.tuple for m in r.Members], r.Quadtree))

ElementInfo.__unicode__ = lambda i: u"(%d, %d, %d, %d, %s, %s)" % (i.version,i.timestamp,i.changeset,i.user_id,i.user,'t' if i.visible else 'f')
ElementInfo.__repr__ = lambda i: "ElementInfo(%d,..)" % (i.version,)
ElementInfo.tuple = property(lambda i: (i.version,i.timestamp,i.changeset,i.user_id,i.user,i.visible))
Tag.__repr__ = lambda t: "Tag(%s,%.50s)" % (t.key,repr(t.val))
Tag.__unicode__ = lambda t: u"%s='%s'" % (t.key,t.val)
Tag.tuple = property(lambda t: (t.key,t.val))

Member.__repr__ = lambda m: "%s %d%s" % ('n' if m.type==ElementType.Node else 'w' if m.type==ElementType.Way else 'r', m.ref, " {%s}" % m.role if m.role else '')
Member.tuple = property(lambda m: (m.type,m.ref,m.role))


Node.find_tag = find_tag
Way.find_tag = find_tag
Relation.find_tag = find_tag

Node.__init__ = _elements.make_node
Way.__init__ = _elements.make_way
Relation.__init__ = _elements.make_relation

class minimalblock_nodes:
    def __init__(self, mb):
        self.mb=mb
    def __len__(self):
        return int(self.mb.nodes_len())
    def __getitem__(self,i):
        return self.mb.nodes_getitem(i)

class minimalblock_ways:
    def __init__(self, mb):
        self.mb=mb
    def __len__(self):
        return int(self.mb.ways_len())
    def __getitem__(self,i):
        return self.mb.ways_getitem(i)

class minimalblock_relations:
    def __init__(self, mb):
        self.mb=mb
    def __len__(self):
        return int(self.mb.relations_len())
    def __getitem__(self,i):
        return self.mb.relations_getitem(i)

class minimalblock_geometries:
    def __init__(self, mb):
        self.mb=mb
    def __len__(self):
        return int(self.mb.geometries_len())
    def __getitem__(self,i):
        return self.mb.geometries_getitem(i)

    
MinimalBlock.nodes = property(lambda mb: minimalblock_nodes(mb))
MinimalBlock.ways = property(lambda mb: minimalblock_ways(mb))
MinimalBlock.relations = property(lambda mb: minimalblock_relations(mb))
MinimalBlock.geometries = property(lambda mb: minimalblock_geometries(mb))
