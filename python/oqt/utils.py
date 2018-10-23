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
import time, sys, json, os, re
from . import _oqt as oq

def intm(f):
    if f<0:
        return int(f*10000000-0.5)
    return int(f*10000000+0.5)


def addto(res, filterempty=False):
    def _(bls):
        res.extend((b for b in bls if (b is not None) and (not filterempty or len(b)>0)))
        return True
    return _

def find_tile(bls, qt):
    
    if qt in bls:
        return bls[qt]
    if qt==0:
        print("adding tile 0")
        bls[0] = oq.primitiveblock(0,0)
        bls[0].quadtree=0
        return bls[0]
    return find_tile(bls, oq.quadtree_round(qt,(qt&31)-1))

def addto_merge(res, resort_tiles=False):
    def _(bls):
        for bl in bls:
            if len(bl)>0:
                if resort_tiles:
                    
                    for o in bl:
                        find_tile(res, o.Quadtree).add(o)
                elif bl.quadtree in res:
                    for o in bl:
                        res[bl.quadtree].add(o)
                else:
                    res[bl.quadtree]=bl
        return True
    return _



class Prog:
    def __init__(self, tiles=None,locs=None):
        self.st=time.time()
        self.nb=0
        self.npt,self.nln,self.nsp,self.ncp=0,0,0,0
        self.tiles = tiles
        self.locs=None
        if locs:
            self.locs=dict((q,100.*i/len(locs)) for i,q in enumerate(sorted(locs)))
        
    def __call__(self, bls):
        if self.tiles is not None:
            self.tiles(bls)
            
        
        self.nb+=len(bls)
        for bl in bls:
            for o in bl:
                if o.Type==oq.ElementType.Point: self.npt+=1
                elif o.Type==oq.ElementType.Linestring: self.nln+=1
                elif o.Type==oq.ElementType.SimplePolygon: self.nsp+=1
                elif o.Type==oq.ElementType.ComplicatedPolygon: self.ncp+=1
        lc=''
        if self.locs:
            maxq=max(b.Quadtree for b in bls)
            if maxq in self.locs:
                lc = "%6.1f%% " % self.locs[maxq]
        print("\r%5d: %s%6.1fs %2d blcks [%-18s] %10d %10d %10d %10d" % (self.nb,lc,time.time()-self.st,len(bls),oq.quadtree_string(max(b.Quadtree for b in bls)) if bls else '', self.npt,self.nln,self.nsp,self.ncp),)
        sys.stdout.flush()
        return True


class tboxbuf:
    def __init__(self, box, buffer):
        self.box=box
        self.buffer=buffer
    def __call__(self, q):
        return self.box.overlaps(oq.quadtree_bbox(q, self.buffer))
def get_locs_single(fn, box_in,asmerge=False, buffer=None):
    hh = oq.getHeaderBlock(fn)
    if not len(hh.index):
        raise Exception(fn+" doesn't contain a header index")
    
    box,test = prep_box(box_in)
    
    
    if not buffer is None:
        test=tboxbuf(box,buffer)
        
    if asmerge:
        return [fn],dict((a,[(0,b)]) for a,b,c in hh.index if test(a)),box        
        
    return [b for a,b,c in hh.index if test(a)]


def pnpoly(vertx, verty, testx, testy):
    j, c = len(vertx)-1,False
    for i in range(len(vertx)):
        if ( ((verty[i]>testy) != (verty[j]>testy)) and (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) ):
            c = not c
        j=i
    return c



    

class Poly:
    def __init__(self, coords):
                
        self.vertx,self.verty = zip(*coords)
        mx = min(self.vertx)
        my = min(self.verty)
        Mx = max(self.vertx)
        My = max(self.verty)
        
        self.bounds = oq.bbox(mx,my,Mx,My)
    
    
    def test(self, qt):
        return self.test_box(oq.quadtree_bbox(qt,0.05))
    
    
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
        return oq.bbox(-1800000000,-900000000,1800000000,900000000), lambda x: True
        
    if isinstance(box_in, oq.bbox):
        return box_in, box_in.overlaps_quadtree
    try:
        box = oq.bbox(*box_in)
        return box, box.overlaps_quadtree
    except:
        pass
    
    try:
        box = oq.bbox(*(intm(f) for f in box_in))
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
    locs = dict((a,[(0,b)]) for a,b,c in oq.get_header_block(fns[0]).Index if box is None or boxtest(a))
    for i,f in enumerate(fns[1:]):
        for a,b,c in oq.get_header_block(f).Index:
            if a in locs:
                locs[a].append((i+1,b))
    return fns, locs, box

replace_ws = lambda w: re.sub('\s+', ' ', w)
