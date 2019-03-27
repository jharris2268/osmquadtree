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
#from oqt import _block, _core, _change
from oqt import elements

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
        bls[0] = elements.primitiveblock(0,0)
        bls[0].quadtree=0
        return bls[0]
    return find_tile(bls, elements.quadtree_round(qt,(qt&31)-1))

def addto_merge(res, resort_tiles=False):
    def _(bls):
        for bl in bls:
            if len(bl)>0:
                if resort_tiles:
                    
                    for o in bl:
                        find_tile(res, o.Quadtree).add(o)
                elif bl.Quadtree in res:
                    for o in bl:
                        res[bl.Quadtree].add(o)
                else:
                    res[bl.Quadtree]=bl
        return True
    return _



class Prog:
    def __init__(self, tiles=None,locs=None):
        self.st=time.time()
        self.nb=0
        #self.npt,self.nln,self.nsp,self.ncp=0,0,0,0
        self.nobjs=0
        self.tiles = tiles
        self.locs=None
        if locs:
            self.locs=dict((q,100.*i/len(locs)) for i,q in enumerate(sorted(locs)))
        
    def __call__(self, bls):
        if self.tiles is not None:
            self.tiles(bls)
            
        
        self.nb+=len(bls)
        self.nobjs+=sum(len(b) for b in bls)
        #for bl in bls:
        #    for o in bl:
        #        if o.Type==_block.ElementType.Point: self.npt+=1
        #        elif o.Type==_block.ElementType.Linestring: self.nln+=1
        #        elif o.Type==_block.ElementType.SimplePolygon: self.nsp+=1
        #        elif o.Type==_block.ElementType.ComplicatedPolygon: self.ncp+=1
        lc=''
        maxq=max(b.Quadtree for b in bls) if bls else 0
        if self.locs:
            if maxq in self.locs:
                lc = "%6.1f%% " % self.locs[maxq]
        #print("\r%5d: %s%6.1fs %2d blcks [%-18s] %10d %10d %10d %10d" % (self.nb,lc,time.time()-self.st,len(bls),_block.quadtree_string(max(b.Quadtree for b in bls)) if bls else '', self.npt,self.nln,self.nsp,self.ncp),)
        sys.stdout.write("\r%7d: %s%6.1fs %2d blocks [%-18s] %d" % (self.nb, lc, time.time()-self.st, len(bls), elements.quadtree_string(maxq), self.nobjs))
        sys.stdout.flush()
        return True


replace_ws = lambda w: re.sub('\s+', ' ', w)


#__old_read_style = lambda stylefn: dict((t['Tag'], _geometry.StyleInfo(IsFeature=t['IsFeature'],IsArea=t['IsPoly']!='no',IsNode=t['IsNode'],IsWay=t['IsWay'], IsOtherTags=('IsOtherTags' in t and t['IsOtherTags']))) for t in json.load(open(stylefn))) 
