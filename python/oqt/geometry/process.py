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
from . import _geometry
from oqt import _block
import json, csv
from oqt.utils import addto, Prog, get_locs, replace_ws, addto_merge
import time,sys,re, gzip

from . import style as geometrystyle, minzoomvalues


def read_blocks(prfx, box_in, lastdate=None, objflags=None, numchan=4):
    
    tiles=[]
    result = addto(tiles)
    fns,locs,box = get_locs(prfx,box_in,lastdate)
    
    if objflags is None:
        objflags=_block.ReadBlockFlags()
    
    _block.read_blocks_merge_primitive(fns, result, locs, numchan=numchan, objflags=objflags)
    return tiles




def prep_geometry_params(prfx, box_in, style, lastdate, minzoom, numchan, minlen, minarea):
    params = _geometry.GeometryParameters()
        
    params.numchan=numchan
    params.recalcqts=True
    
    if box_in is None:
        
        box_in=utils.bbox(-1800000000,-900000000,1800000000,900000000)
    
    
    
    params.filenames,params.locs, params.box = get_locs(prfx,box_in,lastdate)
    print("%d fns, %d qts, %d blocks" % (len(params.filenames),len(params.locs),sum(len(v) for k,v in params.locs.items())))
    
    
    if style is None:
        style=geometrystyle.GeometryStyle()
    
    style.set_params(params, minzoom != [])
    
    print("%d styles" % len(params.style))
    
    
    if minzoom != []:
        if minzoom is None or minzoom=='default':
            minzoom=[t[:4] for t in minzoomvalues.default]
        if isinstance(minzoom, basestring):
            minzoom=[t[:4] for t in minzoomvalues.read(minzoom)]
        
        params.findmz = _geometry.findminzoom_onetag(minzoom,minlen,minarea)
        print ('findmz', params.findmz)
        
    return params, style

def process_geometry(prfx, box_in, stylefn=None, collect=True, outfn=None, lastdate=None,indexed=False,minzoom=None,nothread=False,mergetiles=False, maxtilelevel=None, groups=None, numchan=4,minlen=0,minarea=5):
    tiles={} if mergetiles else []
    
    
    params,style = prep_geometry_params(prfx, box_in, stylefn, lastdate, minzoom, numchan, minlen, minarea)
    
    if not groups is None:
        params.groups=groups
    
    if outfn:
        params.outfn=outfn
    params.indexed=indexed
    
    
    if mergetiles and collect:
        for l in params.locs:
            if not maxtilelevel is None and (l&31) > maxtilelevel:
                continue
            tiles[l]=_block.PrimitiveBlock(0,0)
            tiles[l].Quadtree=l
    
    
    cnt,errs=None,None
    
    
    
    if len(params.locs) > 2500 and outfn is not None:
        collect=False
        
    callback = None
    if collect:
        if mergetiles:
            callback=Prog(addto_merge(tiles, True),params.locs)
        else:
            callback=Prog(addto(tiles),params.locs)
    
    #else:
    #    if (groups and outfn) or params.connstring != '':
    #        callback=Prog(locs=params.locs)
    #    else:
    #        pass
    
    
    if nothread:
        errs = _geometry.process_geometry_nothread(params, callback)
    elif groups is not None:
        errs = _geometry.process_geometry_sortblocks(params, callback)
    else:
        errs = _geometry.process_geometry(params, callback)
    
    if collect:
        if mergetiles:
            res=[]
            for a,b in sorted(tiles.items()):
                if len(b):
                    b.sort()
                    res.append(b)
            return res
        else:
            return tiles
    
    return errs




def to_json(tile,split=True):
    res = {}
    
    
    for obj in tile:
        
        tab=None
        if split:
            if obj.Type==_block.ElementType.Point:
                tab = 'point'
            if obj.Type==_block.ElementType.Linestring:
                tab='line'
            if obj.Type in (_block.ElementType.SimplePolygon, _block.ElementType.ComplicatedPolygon):
                tab='polygon'
        if not tab in res:
            res[tab] = {'type':'FeatureCollection','features': [], 'quadtree': _block.quadtree_tuple(tile.Quadtree), 'bbox': None, 'table': tab}
        
        res[tab]['features'].append(make_json_feat(obj))
    
    for k,v in res.items():
        v['features'].sort(key=lambda x: x['id'])
        v['bbox'] = collect_bbox(f['bbox'] for f in v['features'])
        
    return res.values()

def coord_json(ll):
    xy = ll.transform
    return [round(xy.x,1),round(xy.y,1)]

def ring_bbox(cc):
    x,y=zip(*cc)
    return [min(x),min(y),max(x),max(y)]

def collect_bbox(bboxes):
    mx,my,Mx,My = zip(*bboxes)
    return [min(mx),min(my),max(Mx),max(My)]

def make_json_feat(obj):
    res = {'type':'Feature',}
    if obj.Type==_block.ElementType.ComplicatedPolygon:
        res['id'] = -1*obj.Id
    else:
        res['id'] = obj.Id
    res['quadtree'] = _block.quadtree_tuple(obj.Quadtree)
    res['minzoom']  = obj.MinZoom
    res['properties'] = dict((t.key,t.val) for t in obj.Tags)
    if obj.Type==_block.ElementType.Point:
        res['geometry'] = {'type':'Point','coordinates':coord_json(obj.LonLat)}
        res['bbox'] = res['geometry']['coordinates']*2
    elif obj.Type==_block.ElementType.Linestring:
        res['geometry'] = {'type':'LineString', 'coordinates': map(coord_json, obj.LonLats)}
        res['bbox'] = ring_bbox(res['geometry']['coordinates'])
        res['properties']['way_length'] = round(obj.Length,1)
        res['properties']['z_order'] = obj.ZOrder
    elif obj.Type==_block.ElementType.SimplePolygon:
        res['geometry'] = {'type':'Polygon', 'coordinates': [map(coord_json, obj.LonLats)]}
        res['bbox'] = ring_bbox(res['geometry']['coordinates'][0])
        res['properties']['way_area'] = round(obj.Area,1)
        res['properties']['z_order'] = obj.ZOrder

    elif obj.Type==_block.ElementType.ComplicatedPolygon:
        res['geometry'] = {'type':'Polygon', 'coordinates': [map(coord_json, obj.OuterLonLats)]+[map(coord_json,ii) for ii in obj.InnerLonLats]}
        res['bbox'] = ring_bbox(res['geometry']['coordinates'][0])
        res['properties']['way_area'] = round(obj.Area,1)
        res['properties']['z_order'] = obj.ZOrder
    return res





