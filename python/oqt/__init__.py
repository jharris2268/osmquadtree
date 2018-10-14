from __future__ import print_function
from . import _oqt
from .processgeometry import process_geometry, write_to_postgis, read_blocks
import sys, time, numbers
from .utils import intm, read_poly_file, Poly
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

_oqt.QtTree.__iter__ = iter_tree
_oqt.QtTreeItem.__repr__ = lambda t: "qttree_item(%6d, %-29s %6d, %10d)" % (t.idx,repr(quadtree(t.qt))+",", t.total, t.weight)




class py_logger(_oqt.Logger):
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
run_count = _oqt.run_count

#_oqt.Node._Quadtree=_oqt.Node.Quadtree
#_oqt.Node.Quadtree=property(lambda q: quadtree(q._Quadtree))

_oqt.Node.__repr__ = lambda n: "Node(%10d %0.40s %.40s % 8d % 8d %-18s %d)" % (n.Id,repr(n.Info),repr(n.Tags),n.Lon,n.Lat,n.Quadtree,n.ChangeType)
_oqt.Node.__unicode__ = lambda n: u"Node(%10d %s %s % 8d % 8d %-18s %d)" % (n.Id,n.Info,n.Tags,n.Lon,n.Lat,n.Quadtree,n.ChangeType)
_oqt.Node.tuple = property(lambda n: (0, n.ChangeType, n.Id,n.Info.tuple,[t.tuple for t in n.Tags], n.Lon,n.Lat, n.Quadtree))


#_oqt.Way._Quadtree=_oqt.Way.Quadtree
#_oqt.Way.Quadtree=property(lambda q: quadtree(q._Quadtree))
_oqt.Way.__repr__ = lambda w: "Way(%10d %0.40s %.40s %.20s %-18s %d)" % (w.Id,repr(w.Info),repr(w.Tags),w.Refs,w.Quadtree,w.ChangeType)
_oqt.Way.__unicode__ = lambda w: u"Way(%10d %s %s %s %-18s %d)" % (w.Id,w.Info,w.Tags,w.Refs,w.Quadtree,w.ChangeType)
_oqt.Way.tuple = property(lambda w: (1, w.ChangeType, w.Id,w.Info.tuple,[t.tuple for t in w.Tags], w.Refs, w.Quadtree))

#_oqt.Relation._Quadtree=_oqt.Relation.Quadtree
#_oqt.Relation.Quadtree=property(lambda q: quadtree(q._Quadtree))
_oqt.Relation.__repr__ = lambda r: "Relation(%10d %0.40s %.40s %.20s %-18s %d)" % (r.Id,repr(r.Info),repr(r.Tags),r.Members,r.Quadtree,r.ChangeType)
_oqt.Relation.__unicode__ = lambda r: u"Relation(%10d %s %s %s %-18s %d)" % (r.Id,r.Info,r.Tags,r.Members,r.Quadtree,r.ChangeType)
_oqt.Relation.tuple = property(lambda r: (2, r.ChangeType, r.Id,r.Info.tuple,[t.tuple for t in r.Tags], [m.tuple for m in r.Members], r.Quadtree))

_oqt.ElementInfo.__unicode__ = lambda i: u"(%d, %d, %d, %d, %s, %s)" % (i.version,i.timestamp,i.changeset,i.user_id,i.user,'t' if i.visible else 'f')
_oqt.ElementInfo.__repr__ = lambda i: "ElementInfo(%d,..)" % (i.version,)
_oqt.ElementInfo.tuple = property(lambda i: (i.version,i.timestamp,i.changeset,i.user_id,i.user,i.visible))
_oqt.Tag.__repr__ = lambda t: "Tag(%s,%.50s)" % (t.key,repr(t.val))
_oqt.Tag.__unicode__ = lambda t: u"%s='%s'" % (t.key,t.val)
_oqt.Tag.tuple = property(lambda t: (t.key,t.val))

_oqt.Member.__repr__ = lambda m: "%s %d%s" % ('n' if m.type==0 else 'w' if m.type==1 else 'r', m.ref, " {%s}" % m.role if m.role else '')
_oqt.Member.tuple = property(lambda m: (m.type,m.ref,m.role))

_oqt.PbfTag.__repr__ = lambda p: "(%d, %d, %.20s)" % (p.tag, p.value, p.data)
_oqt.PbfTag.tuple = property(lambda p: (p.tag, p.value, p.data))

def find_tag(obj, idx):
    for tg in obj.Tags:
        if tg.key==idx:
            return tg.val
    return None
_oqt.Node.find_tag = find_tag
_oqt.Way.find_tag = find_tag
_oqt.Relation.find_tag = find_tag
_oqt.Point.find_tag = find_tag
_oqt.Linestring.find_tag = find_tag
_oqt.SimplePolygon.find_tag = find_tag
_oqt.ComplicatedPolygon.find_tag = find_tag

_oqt.ComplicatedPolygon.OuterRefs = property(lambda cp: _oqt.ringpart_refs(cp.OuterRing))
_oqt.ComplicatedPolygon.OuterLonLats = property(lambda cp: _oqt.ringpart_lonlats(cp.OuterRing))
_oqt.ComplicatedPolygon.InnerRefs = property(lambda cp: [_oqt.ringpart_refs(ii) for ii in cp.InnerRings])
_oqt.ComplicatedPolygon.InnerLonLats = property(lambda cp: [_oqt.ringpart_lonlats(ii) for ii in cp.InnerRings])


_oqt.Point.__repr__ = lambda p: "Point(%10d %.50s %-18s [% 8d % 8d] )" % (p.Id,p.Tags,p.Quadtree,p.LonLat.lon,p.LonLat.lat)
_oqt.Linestring.__repr__ = lambda l: "Linestring(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (l.Id,l.Tags,l.Quadtree,len(l.Refs),l.Length)
_oqt.SimplePolygon.__repr__ = lambda p: "SimplePolygon(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,len(p.Refs),p.Area)
_oqt.ComplicatedPolygon.__repr__ = lambda p: "ComplicatedPolygon(%10d %.50s %-18s [P. %2d % 4d pts (%2d ints), %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,p.Part,len(p.OuterRefs),len(p.InnerRings),p.Area)

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

    
_oqt.MinimalBlock.nodes = property(lambda mb: minimalblock_nodes(mb))
_oqt.MinimalBlock.ways = property(lambda mb: minimalblock_ways(mb))
_oqt.MinimalBlock.relations = property(lambda mb: minimalblock_relations(mb))
_oqt.MinimalBlock.geometries = property(lambda mb: minimalblock_geometries(mb))


_oqt._ReadBlocksCaller = _oqt.ReadBlocksCaller

def prep_poly(poly):
    if poly is None:
        return []
    
    if not poly:
        return []
    
    if isinstance(poly[0], _oqt.lonlat):
        return poly
    
    if type(poly)==str:
        poly=read_poly_file(poly)
    
    
    return [_oqt.lonlat(x,y) for x,y in poly]

class ReadBlocksCaller:
    def __init__(self, prfx, bbox, poly=None, lastdate=None):
        if not lastdate is None:
            if not isinstance(lastdate, numbers.Integral):
                lastdate = _oqt.read_date(lastdate)
        else:
            lastdate=0
        
        self.bbox=bbox
        self.poly=prep_poly(poly)
        if self.bbox is None:
            if self.poly:
                ln=[l.lon for l in self.poly]
                lt=[l.lat for l in self.poly]
                self.bbox=_oqt.bbox(min(ln),min(lt),max(ln),max(lt))
            else:
                self.bbox=_oqt.bbox(-1800000000,-900000000,1800000000,900000000)
        self.rbc = _oqt.make_read_blocks_caller(prfx, self.bbox, self.poly, lastdate)
    
    def read_primitive(self, cb, numblocks=512, numchan=4, filter=None):
        return _oqt.read_blocks_caller_read_primitive(self.rbc, cb, numblocks, numchan, filter)
                    
    def read_minimal(self, cb, numblocks=512, numchan=4, filter=None):
        return _oqt.read_blocks_caller_read_minimal(self.rbc, cb, numblocks, numchan, filter)            

    def num_tiles(self):
        return self.rbc.num_tiles()
        
    def calc_idset(self, bbox=None, poly=None):
        if bbox is None:
            bbox=self.bbox
        if poly is None:
            poly=self.poly
        else:
            poly=prep_poly(poly)
        return _oqt.calc_idset_filter(self.rbc, bbox, poly, 4)

_oqt.ReadBlocksCaller=ReadBlocksCaller

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

