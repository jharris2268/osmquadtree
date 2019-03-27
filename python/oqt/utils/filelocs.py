
import os

from oqt import _block, _change
from . import _utils
from .misc import intm
import json

class tboxbuf:
    def __init__(self, box, buffer):
        self.box=box
        self.buffer=buffer
    def __call__(self, q):
        return self.box.overlaps(_block.quadtree_bbox(q, self.buffer))
def get_locs_single(fn, box_in,asmerge=False, buffer=None):
    hh = _change.get_header_block(fn)
    if not len(hh.Index):
        raise Exception(fn+" doesn't contain a header index")
    
    box,test = prep_box(box_in)
    
    
    if not buffer is None:
        test=tboxbuf(box,buffer)
        
    if asmerge:
        ll = {}
        for a,b,c in hh.Index:
            if test(a):
                if not a in ll:
                    ll[a]=[]
                ll[a].append((0,b))
        return [fn], ll, box
        #return [fn],dict((a,[(0,b)]) for a,b,c in hh.Index if test(a)),box        
        
    return [b for a,b,c in hh.Index if test(a)]


def pnpoly(vertx, verty, testx, testy):
    j, c = len(vertx)-1,False
    for i in range(len(vertx)):
        if ( ((verty[i]>testy) != (verty[j]>testy)) and (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) ):
            c = not c
        j=i
    return c



    

class Poly:
    def __init__(self, coords):
        
        if isinstance(coords, basestring):
            coords = read_poly_file(coords)
        
        self.vertx,self.verty = zip(*coords)
        mx = min(self.vertx)
        my = min(self.verty)
        Mx = max(self.vertx)
        My = max(self.verty)
        
        self.bounds = _utils.bbox(mx,my,Mx,My)
    
    
    def test(self, qt):
        return self.test_box(_block.quadtree_bbox(qt,0.05))
    
    
    def test_box(self, tb):
        
        
        if not self.bounds.overlaps(tb):
            return 0
        
        
        if tb.contains_box(self.bounds):
            return 1
        a,b,c,d = tb    
        
        x,y = (a+c)/2, (b+d)/2
        if pnpoly(self.vertx,self.verty, x, y):
            return 2
        
        if self.intersect_box(tb):
            return 3
        
        return 4
    
    def intersect_box(self, bx):
        def onsegment(p, q, r):
            if q[0] > max(p[0], r[0]): return False
            if q[0] < min(p[0], r[0]): return False
            if q[1] > max(p[1], r[1]): return False
            if q[1] < min(p[1], r[1]): return False
            
            return True

        def orientation(p, q, r):
            res = (q[1]-p[1]) * (r[0] - q[0]) - (q[0] - p[0]) * (r[1]-q[1])
            return -1 if res<0 else 0 if res==0 else 1


        def intersects_seg(p1, q1, p2, q2):
            o1 = orientation(p1, q1, p2)
            o2 = orientation(p1, q1, q2)
            o3 = orientation(p2, q2, p1)
            o4 = orientation(p2, q2, q1)

            
            if (o1 != o2) and (o3 != o4):
                return True
                
            if o1 == 0 and onsegment(p1,p2,q1):
                return True    
            
            if o2 == 0 and onsegment(p1,q2,q1):
                return True
            
            if o3 == 0 and onsegment(p2,p1,q2):
                return True
            
            if o4 == 0 and onsegment(p2,q1,q2):
                return True
            
            return False
        
        vertc=zip(self.vertx,self.verty)
        def intersects_line(c,d):
            for a,b in zip(vertc,vertc[1:]):
                if intersects_seg(a,b,c,d):
                    return True
            return False
        
        #left
        if bx[0] > self.bounds[0]:
            if intersects_line((bx[0],bx[1]),(bx[0],bx[3])):
                return True
        #bottom
        if bx[1] > self.bounds[1]:
            if intersects_line((bx[0],bx[1]),(bx[2],bx[1])):
                return True
        
        #right
        if bx[2] < self.bounds[2]:
            if intersects_line((bx[2],bx[1]),(bx[2],bx[3])):
                return True
        #top
        if bx[3] < self.bounds[3]:
            if intersects_line((bx[0],bx[3]),(bx[2],bx[3])):
                return True
                
        
        return False
    
    def __call__(self, qt):
        return self.test(qt) in (1,2,3)


def read_poly_file(fl):
    
    res=[]
    for l in open(fl):
        try:
            a,b = map(float, l.strip().split())
            res.append((intm(a), intm(b)))
            
        except:
            pass
    
    return res
    

def prep_box(box_in):
    if box_in is None:
        return _utils.bbox(-1800000000,-900000000,1800000000,900000000), lambda x: True
        
    if isinstance(box_in, _utils.bbox):
        return box_in, box_in.overlaps_quadtree
    try:
        box = _utils.bbox(*box_in)
        return box, box.overlaps_quadtree
    except:
        pass
    
    try:
        box = _utils.bbox(*(intm(f) for f in box_in))
        return box, box.overlaps_quadtree
    except:
        pass
    
    poly = Poly(box_in)
    return poly.bounds, poly


def get_locs(prfx,box_in=None, lastdate=None):
    
    
    
    if os.path.exists(prfx) and not os.path.isdir(prfx):
        return get_locs_single(prfx, box_in, True)
        
    
    box,boxtest = prep_box(box_in)    
    fl_path=prfx+'filelist.json'
    if not os.path.exists(fl_path):
        raise Exception(fl_path+" doesn't exist")
    fns = []
    sk=0
    for f in json.load(open(fl_path)):
        if lastdate is None or f['EndDate'] <= lastdate:
            fns.append(prfx+f['Filename'])
        else:
            sk+=1
    if lastdate is not None:
        print("using %d of %d files: %s => %s" % (len(fns),sk+len(fns),fns[0],fns[-1]))
    #fns = [prfx+f['Filename'] for f in json.load(open(fl_path))]
    locs = dict((a,[(0,b)]) for a,b,c in _change.get_header_block(fns[0]).Index if box is None or boxtest(a))
    for i,f in enumerate(fns[1:]):
        for a,b,c in _change.get_header_block(f).Index:
            if a in locs:
                locs[a].append((i+1,b))
    return fns, locs, box
