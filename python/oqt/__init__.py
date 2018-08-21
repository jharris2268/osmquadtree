from __future__ import print_function
from . import _oqt
from .processgeometry import process_geometry, write_to_postgis, read_blocks
import sys, time
from .utils import intm
from .xmlchange import read_timestamp
time_str = lambda t: time.strftime("%Y-%m-%dT%H:%M:%S",time.gmtime(t))

bbox = _oqt.bbox
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
        return cls(_oqt.quadtree_from_xyz(a,b,c))
    @classmethod
    def from_string(cls, s):
        return cls(_oqt.quadtree_from_string(s))

    @classmethod
    def calculate(cls, minx, miny, maxx, maxy, buffer=0.05, maxlevel=18):
        return cls(_oqt.calculate(minx,miny,maxx,maxy,buffer,maxlevel))


    def __int__(self):
        return self.q

    def __str__(self):
        return _oqt.quadtree_string(self.q)

    def __repr__(self):
        return "quadtree(%s)" % str(self)

    def tuple(self):
        return _oqt.quadtree_tuple(self.q)

    def common(self, other):
        return quadtree(_oqt.quadtree_common(self,other))

    def round(self, nl):
        return quadtree(_oqt.quadtree_round(self.q, nl))

    def bbox(self, buffer=0.05):
        return _oqt.quadtree_bbox(self.q, buffer)
       
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

_oqt.qttree.__iter__ = iter_tree
_oqt.qttree_item.__repr__ = lambda t: "qttree_item(%6d, %-29s %6d, %10d)" % (t.idx,repr(quadtree(t.qt))+",", t.total, t.weight)




class py_logger(_oqt.logger):
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
_oqt.set_logger(_logger)

sortblocks = _oqt.sortblocks
calcqts = _oqt.calcqts
mergechanges = _oqt.mergechanges
count = _oqt.count

#_oqt.node._Quadtree=_oqt.node.Quadtree
#_oqt.node.Quadtree=property(lambda q: quadtree(q._Quadtree))

_oqt.node.__repr__ = lambda n: "Node(%10d %0.40s %.40s % 8d % 8d %-18s %d)" % (n.Id,repr(n.Info),repr(n.Tags),n.Lon,n.Lat,n.Quadtree,n.ChangeType)
_oqt.node.__unicode__ = lambda n: u"Node(%10d %s %s % 8d % 8d %-18s %d)" % (n.Id,n.Info,n.Tags,n.Lon,n.Lat,n.Quadtree,n.ChangeType)
_oqt.node.tuple = property(lambda n: (0, n.ChangeType, n.Id,n.Info.tuple,[t.tuple for t in n.Tags], n.Lon,n.Lat, n.Quadtree))


#_oqt.way._Quadtree=_oqt.way.Quadtree
#_oqt.way.Quadtree=property(lambda q: quadtree(q._Quadtree))
_oqt.way.__repr__ = lambda w: "Way(%10d %0.40s %.40s %.20s %-18s %d)" % (w.Id,repr(w.Info),repr(w.Tags),w.Refs,w.Quadtree,w.ChangeType)
_oqt.way.__unicode__ = lambda w: u"Way(%10d %s %s %s %-18s %d)" % (w.Id,w.Info,w.Tags,w.Refs,w.Quadtree,w.ChangeType)
_oqt.way.tuple = property(lambda w: (1, w.ChangeType, w.Id,w.Info.tuple,[t.tuple for t in w.Tags], w.Refs, w.Quadtree))

#_oqt.relation._Quadtree=_oqt.relation.Quadtree
#_oqt.relation.Quadtree=property(lambda q: quadtree(q._Quadtree))
_oqt.relation.__repr__ = lambda r: "Relation(%10d %0.40s %.40s %.20s %-18s %d)" % (r.Id,repr(r.Info),repr(r.Tags),r.Members,r.Quadtree,r.ChangeType)
_oqt.relation.__unicode__ = lambda r: u"Relation(%10d %s %s %s %-18s %d)" % (r.Id,r.Info,r.Tags,r.Members,r.Quadtree,r.ChangeType)
_oqt.relation.tuple = property(lambda r: (2, r.ChangeType, r.Id,r.Info.tuple,[t.tuple for t in r.Tags], [m.tuple for m in r.Members], r.Quadtree))

_oqt.info.__unicode__ = lambda i: u"(%d, %d, %d, %d, %s, %s)" % (i.version,i.timestamp,i.changeset,i.user_id,i.user,'t' if i.visible else 'f')
_oqt.info.__repr__ = lambda i: "info(%d,...)" % (i.version,)
_oqt.info.tuple = property(lambda i: (i.version,i.timestamp,i.changeset,i.user_id,i.user,i.visible))
_oqt.tag.__repr__ = lambda t: "Tag(%s,%.50s)" % (t.key,repr(t.val))
_oqt.tag.__unicode__ = lambda t: u"%s='%s'" % (t.key,t.val)
_oqt.tag.tuple = property(lambda t: (t.key,t.val))

_oqt.member.__repr__ = lambda m: "%s %d%s" % ('n' if m.type==0 else 'w' if m.type==1 else 'r', m.ref, " {%s}" % m.role if m.role else '')
_oqt.member.tuple = property(lambda m: (m.type,m.ref,m.role))

_oqt.PbfTag.__repr__ = lambda p: "(%d, %d, %.20s)" % (p.tag, p.value, p.data)
_oqt.PbfTag.tuple = property(lambda p: (p.tag, p.value, p.data))

def find_tag(obj, idx):
    for tg in obj.Tags:
        if tg.key==idx:
            return tg.val
    return None
_oqt.node.find_tag = find_tag
_oqt.way.find_tag = find_tag
_oqt.relation.find_tag = find_tag
_oqt.point.find_tag = find_tag
_oqt.linestring.find_tag = find_tag
_oqt.simplepolygon.find_tag = find_tag
_oqt.complicatedpolygon.find_tag = find_tag

_oqt.complicatedpolygon.OuterRefs = property(lambda cp: _oqt.ringpart_refs(cp.Outers))
_oqt.complicatedpolygon.OuterLonLats = property(lambda cp: _oqt.ringpart_lonlats(cp.Outers))
_oqt.complicatedpolygon.InnerRefs = property(lambda cp: [_oqt.ringpart_refs(ii) for ii in cp.Inners])
_oqt.complicatedpolygon.InnerLonLats = property(lambda cp: [_oqt.ringpart_lonlats(ii) for ii in cp.Inners])


_oqt.point.__repr__ = lambda p: "Point(%10d %.50s %-18s [% 8d % 8d] )" % (p.Id,p.Tags,p.Quadtree,p.LonLat.lon,p.LonLat.lat)
_oqt.linestring.__repr__ = lambda l: "LineString(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (l.Id,l.Tags,l.Quadtree,len(l.Refs),l.Length)
_oqt.simplepolygon.__repr__ = lambda p: "SimplePolygon(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,len(p.Refs),p.Area)
_oqt.complicatedpolygon.__repr__ = lambda p: "ComplicatedPolygon(%10d %.50s %-18s [P. %2d % 4d pts (%2d ints), %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,p.Part,len(p.OuterRefs),len(p.Inners),p.Area)

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

    
_oqt.minimalblock.nodes = property(lambda mb: minimalblock_nodes(mb))
_oqt.minimalblock.ways = property(lambda mb: minimalblock_ways(mb))
_oqt.minimalblock.relations = property(lambda mb: minimalblock_relations(mb))
_oqt.minimalblock.geometries = property(lambda mb: minimalblock_geometries(mb))


def run_calcqts_alt(origfn, qtsfn=None, numchan=4, splitways=True, resort=True, buffer=0.05, max_depth=18):
    _oqt.get_logger().reset_timing()
    if qtsfn is None:
        qtsfn=origfn[:-4]+"-qts.pbf"
    waynodes_fn = qtsfn+"-waynodes"
    
    wns,rels,nodes_fn,node_locs = _oqt.write_waynodes(origfn, waynodes_fn, numchan, resort)
    
    _oqt.checkstats()
    way_qts = _oqt.make_qtstore_split(1<<20,True)
    
    if splitways:
        midway = 320<<20
        _oqt.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, midway)
        _oqt.checkstats()
        _oqt.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, midway, 0)
        _oqt.checkstats()
        
    else:
        _oqt.find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, 0)
        _oqt.checkstats()
    
    
    _oqt.write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth)
    _oqt.get_logger().timing_messages()
    return 1

