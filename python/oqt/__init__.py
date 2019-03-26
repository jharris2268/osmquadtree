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
from . import _block, _change, _core, _geometry, _postgis


from .processgeometry import process_geometry, read_blocks
from .postgis import write_to_postgis
import sys, time, numbers
from .utils import intm, read_poly_file, Poly
from .xmlchange import read_timestamp
time_str = lambda t: time.strftime("%Y-%m-%dT%H:%M:%S",time.gmtime(t))

bbox = _block.bbox
bbox.__repr__=lambda b: "bbox(% 10d, % 10d, % 10d, % 10d)" % (b.minx,b.miny,b.maxx,b.maxy)
bbox.__len__ = lambda b: 4
def bbox_getitem(b, i):
    if i==0: return b.minx
    if i==1: return b.miny
    if i==2: return b.maxx
    if i==3: return b.maxy
    raise IndexError()

bbox.__getitem__ = bbox_getitem

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
        return _block.quadtree_string(self.q)

    def __repr__(self):
        return "quadtree(%s)" % str(self)

    def tuple(self):
        return _block.quadtree_tuple(self.q)

    def common(self, other):
        return quadtree(_block.quadtree_common(self,other))

    def round(self, nl):
        return quadtree(_block.quadtree_round(self.q, nl))

    def bbox(self, buffer=0.05):
        return _block.quadtree_bbox(self.q, buffer)
       
    def __eq__(self,other):
        return self.q==other.q
    def __ne__(self,other):
        return self.q!=other.q
    def __lt__(self,other):
        return self.q<other.q

def iter_tree(tree, i=0):
    t=tree.at(i)
    if t.weight:
        yield t
    for i in range(4):
        if t.children(i):
            for x in iter_tree(tree,t.children(i)):
                yield x

_core.QtTree.__iter__ = iter_tree
_core.QtTreeItem.__repr__ = lambda t: "qttree_item(%6d, %-29s %6d, %10d)" % (t.idx,repr(quadtree(t.qt))+",", t.total, t.weight)




class py_logger(_core.Logger):
    def __init__(self):
        super(py_logger,self).__init__()
        self.msgs=[]
        self.ln=0

    def progress(self,d,m):
        if len(m)>self.ln:
            self.ln=len(m)

        sys.stdout.write("\r%6.1f%% %s%s" % (d,m," "*(self.ln-len(m))))
        sys.stdout.flush()

    def message(self,m):
        if self.ln:
            print()
        print(m)
        self.ln=0
        self.msgs.append(m)

_logger = py_logger()
_core.set_logger(_logger)

sortblocks = _core.sortblocks
calcqts = _core.calcqts
mergechanges = _core.mergechanges
run_count = _core.run_count

#_oqt.Node._Quadtree=_oqt.Node.Quadtree
#_oqt.Node.Quadtree=property(lambda q: quadtree(q._Quadtree))

_block.Node.__repr__ = lambda n: "Node(%10d %0.40s %.40s % 8d % 8d %-18s %d)" % (n.Id,repr(n.Info),repr(n.Tags),n.Lon,n.Lat,n.Quadtree,n.ChangeType)
_block.Node.__unicode__ = lambda n: u"Node(%10d %s %s % 8d % 8d %-18s %d)" % (n.Id,n.Info,n.Tags,n.Lon,n.Lat,n.Quadtree,n.ChangeType)
_block.Node.tuple = property(lambda n: (0, n.ChangeType, n.Id,n.Info.tuple,[t.tuple for t in n.Tags], n.Lon,n.Lat, n.Quadtree))


#_oqt.Way._Quadtree=_oqt.Way.Quadtree
#_oqt.Way.Quadtree=property(lambda q: quadtree(q._Quadtree))
_block.Way.__repr__ = lambda w: "Way(%10d %0.40s %.40s %.20s %-18s %d)" % (w.Id,repr(w.Info),repr(w.Tags),w.Refs,w.Quadtree,w.ChangeType)
_block.Way.__unicode__ = lambda w: u"Way(%10d %s %s %s %-18s %d)" % (w.Id,w.Info,w.Tags,w.Refs,w.Quadtree,w.ChangeType)
_block.Way.tuple = property(lambda w: (1, w.ChangeType, w.Id,w.Info.tuple,[t.tuple for t in w.Tags], w.Refs, w.Quadtree))

#_oqt.Relation._Quadtree=_oqt.Relation.Quadtree
#_oqt.Relation.Quadtree=property(lambda q: quadtree(q._Quadtree))
_block.Relation.__repr__ = lambda r: "Relation(%10d %0.40s %.40s %.20s %-18s %d)" % (r.Id,repr(r.Info),repr(r.Tags),r.Members,r.Quadtree,r.ChangeType)
_block.Relation.__unicode__ = lambda r: u"Relation(%10d %s %s %s %-18s %d)" % (r.Id,r.Info,r.Tags,r.Members,r.Quadtree,r.ChangeType)
_block.Relation.tuple = property(lambda r: (2, r.ChangeType, r.Id,r.Info.tuple,[t.tuple for t in r.Tags], [m.tuple for m in r.Members], r.Quadtree))

_block.ElementInfo.__unicode__ = lambda i: u"(%d, %d, %d, %d, %s, %s)" % (i.version,i.timestamp,i.changeset,i.user_id,i.user,'t' if i.visible else 'f')
_block.ElementInfo.__repr__ = lambda i: "ElementInfo(%d,..)" % (i.version,)
_block.ElementInfo.tuple = property(lambda i: (i.version,i.timestamp,i.changeset,i.user_id,i.user,i.visible))
_block.Tag.__repr__ = lambda t: "Tag(%s,%.50s)" % (t.key,repr(t.val))
_block.Tag.__unicode__ = lambda t: u"%s='%s'" % (t.key,t.val)
_block.Tag.tuple = property(lambda t: (t.key,t.val))

_block.Member.__repr__ = lambda m: "%s %d%s" % ('n' if m.type==_block.ElementType.Node else 'w' if m.type==_block.ElementType.Way else 'r', m.ref, " {%s}" % m.role if m.role else '')
_block.Member.tuple = property(lambda m: (m.type,m.ref,m.role))

_block.PbfTag.__repr__ = lambda p: "(%d, %d, %.20s)" % (p.tag, p.value, p.data)
_block.PbfTag.tuple = property(lambda p: (p.tag, p.value, p.data))

def find_tag(obj, idx):
    for tg in obj.Tags:
        if tg.key==idx:
            return tg.val
    return None
_block.Node.find_tag = find_tag
_block.Way.find_tag = find_tag
_block.Relation.find_tag = find_tag
_geometry.Point.find_tag = find_tag
_geometry.Linestring.find_tag = find_tag
_geometry.SimplePolygon.find_tag = find_tag
_geometry.ComplicatedPolygon.find_tag = find_tag

_geometry.ComplicatedPolygon.OuterRefs = property(lambda cp: _geometry.ringpart_refs(cp.OuterRing))
_geometry.ComplicatedPolygon.OuterLonLats = property(lambda cp: _geometry.ringpart_lonlats(cp.OuterRing))
_geometry.ComplicatedPolygon.InnerRefs = property(lambda cp: [_geometry.ringpart_refs(ii) for ii in cp.InnerRings])
_geometry.ComplicatedPolygon.InnerLonLats = property(lambda cp: [_geometry.ringpart_lonlats(ii) for ii in cp.InnerRings])


_geometry.Point.__repr__ = lambda p: "Point(%10d %.50s %-18s [% 8d % 8d] )" % (p.Id,p.Tags,p.Quadtree,p.LonLat.lon,p.LonLat.lat)
_geometry.Linestring.__repr__ = lambda l: "Linestring(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (l.Id,l.Tags,l.Quadtree,len(l.Refs),l.Length)
_geometry.SimplePolygon.__repr__ = lambda p: "SimplePolygon(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,len(p.Refs),p.Area)
_geometry.ComplicatedPolygon.__repr__ = lambda p: "ComplicatedPolygon(%10d %.50s %-18s [P. %2d % 4d pts (%2d ints), %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,p.Part,len(p.OuterRefs),len(p.InnerRings),p.Area)

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

    
_block.MinimalBlock.nodes = property(lambda mb: minimalblock_nodes(mb))
_block.MinimalBlock.ways = property(lambda mb: minimalblock_ways(mb))
_block.MinimalBlock.relations = property(lambda mb: minimalblock_relations(mb))
_block.MinimalBlock.geometries = property(lambda mb: minimalblock_geometries(mb))


_block._ReadBlocksCaller = _block.ReadBlocksCaller

def prep_poly(poly):
    if poly is None:
        return []
    
    if not poly:
        return []
    
    if isinstance(poly[0], _oqt.lonlat):
        return poly
    
    if type(poly)==str:
        poly=read_poly_file(poly)
    
    
    return [_block.lonlat(x,y) for x,y in poly]

class ReadBlocksCaller:
    def __init__(self, prfx, bbox, poly=None, lastdate=None):
        if not lastdate is None:
            if not isinstance(lastdate, numbers.Integral):
                raise Exception("??")# = _oqt.read_date(lastdate)
        else:
            lastdate=0
        
        self.bbox=bbox
        self.poly=prep_poly(poly)
        if self.bbox is None:
            if self.poly:
                ln=[l.lon for l in self.poly]
                lt=[l.lat for l in self.poly]
                self.bbox=_block.bbox(min(ln),min(lt),max(ln),max(lt))
            else:
                self.bbox=_block.bbox(-1800000000,-900000000,1800000000,900000000)
        self.rbc = _block.make_read_blocks_caller(prfx, self.bbox, self.poly, lastdate)
    
    def read_primitive(self, cb, numblocks=512, numchan=4, filter=None):
        return _block.read_blocks_caller_read_primitive(self.rbc, cb, numblocks, numchan, filter)
                    
    def read_minimal(self, cb, numblocks=512, numchan=4, filter=None):
        return _block.read_blocks_caller_read_minimal(self.rbc, cb, numblocks, numchan, filter)            

    def num_tiles(self):
        return self.rbc.num_tiles()
        
    def calc_idset(self, bbox=None, poly=None):
        if bbox is None:
            bbox=self.bbox
        if poly is None:
            poly=self.poly
        else:
            poly=prep_poly(poly)
        return _block.calc_idset_filter(self.rbc, bbox, poly, 4)

_block.ReadBlocksCaller=ReadBlocksCaller

def run_calcqts_alt(origfn, qtsfn=None, numchan=4, splitways=True, resort=True, buffer=0.05, max_depth=18):
    _oqt.get_logger().reset_timing()
    if qtsfn is None:
        qtsfn=origfn[:-4]+"-qts.pbf"
    waynodes_fn = qtsfn+"-waynodes"
    
    wns,rels,nodes_fn,node_locs = _block.write_waynodes(origfn, waynodes_fn, numchan, resort)
    
    _core.checkstats()
    way_qts = _oqt.make_qtstore_split(1<<20,True)
    
    if splitways:
        midway = 256<<20
        _core.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, midway)
        _core.checkstats()
        _core.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, midway, 2*midway)
        _core.checkstats()
        _core.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 2*midway, 0)
        _core.checkstats()
        
    else:
        _core.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, 0)
        _core.checkstats()
    
    
    _core.write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth)
    _core.get_logger().timing_messages()
    return 1

