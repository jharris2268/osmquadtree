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
from six import string_types

from . import _geometry
from oqt import elements, pbfformat, utils
import json, csv
from oqt.utils import addto, replace_ws, addto_merge, Prog
from oqt.pbfformat import get_locs
import time,sys,re, gzip

from . import style as geometrystyle, minzoomvalues


def read_blocks(prfx, box_in, lastdate=None, objflags=None, numchan=4):
    
    tiles=[]
    result = addto(tiles)
    fns,locs,box = get_locs(prfx,box_in,lastdate)
    
    if objflags is None:
        objflags=pbfformat.ReadBlockFlags()
    
    pbfformat.read_blocks_merge_primitive(fns, result, locs, numchan=numchan, objflags=objflags)
    return tiles




def prep_geometry_params(prfx, box_in, style, lastdate, minzoom, numchan, minlen, minarea):
    params = _geometry.GeometryParameters()
        
    params.numchan=numchan
    params.recalcqts=True
    
    if box_in is None:
        
        box_in=utils.bbox(-1800000000,-900000000,1800000000,900000000)
    
    
    if not prfx is None:
    
        params.filenames,params.locs, params.box = get_locs(prfx,box_in,lastdate)
    #print("%d fns, %d qts, %d blocks" % (len(params.filenames),len(params.locs),sum(len(v) for k,v in params.locs.items())))
    
    
    if style is None:
        style=geometrystyle.GeometryStyle()
    
    style.set_params(params, minzoom != [])
    
    #print("%d styles" % len(params.style))
    
    
    if minzoom != []:
        if minzoom is None or minzoom=='default':
            minzoom=[t[:4] for t in minzoomvalues.default]
        if isinstance(minzoom, string_types):
            minzoom=[t[:4] for t in minzoomvalues.read(minzoom)]
        
        params.findmz = _geometry.findminzoom_onetag(minzoom,minlen,minarea)
        #print ('findmz', params.findmz)
        
    return params, style

def process_geometry(prfx, box_in, stylefn=None, collect=True, outfn=None, lastdate=None,indexed=False,minzoom=None,nothread=False,mergetiles=False, maxtilelevel=None, groups=None, numchan=4,minlen=0,minarea=5):
    tiles={} if mergetiles else []
    
    
    params,style = prep_geometry_params(prfx, box_in, stylefn, lastdate, minzoom, numchan, minlen, minarea)
    if not maxtilelevel is None:
        params.max_min_zoom_level = maxtilelevel
        
    if not groups is None:
        params.groups=groups
    
    if outfn:
        params.outfn=outfn
    params.indexed=indexed
    
    
    if mergetiles and collect:
        for l in params.locs:
            if not maxtilelevel is None and (l&31) > maxtilelevel:
                continue
            tiles[l]=elements.PrimitiveBlock(0,0)
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
            return errs, res
        else:
            return errs, tiles
    
    return errs, None


def process_geometry_from_vec(blocks, style=None, nothread=False):
    params,style = prep_geometry_params(None, None, style, 0, None, 4, 0, 5)
    
    tiles=[]
    callback=Prog(addto(tiles))
    
    errs = _geometry.process_geometry_from_vec(blocks, params, nothread, callback)
    
    return errs, tiles
    
    



def to_json(tile,split=True):
    res = {}
    
    
    for obj in tile:
        
        tab=None
        if split:
            if obj.Type==elements.ElementType.Point:
                tab = 'point'
            if obj.Type==elements.ElementType.Linestring:
                tab='line'
            if obj.Type in (elements.ElementType.SimplePolygon, elements.ElementType.ComplicatedPolygon):
                tab='polygon'
        if not tab in res:
            res[tab] = {'type':'FeatureCollection','features': [], 'quadtree': elements.quadtree_tuple(tile.Quadtree), 'bbox': None, 'table': tab}
        
        res[tab]['features'].append(make_json_feat(obj))
    
    for k,v in res.items():
        v['features'].sort(key=lambda x: x['id'])
        v['bbox'] = collect_bbox(f['bbox'] for f in v['features'])
        
    return res.values()

def coord_json(ll):
    xy = ll.transform
    #return [round(xy.x,1),round(xy.y,1)]
    return [xy.x,xy.y]

def ring_bbox(cc):
    x,y=zip(*cc)
    return [min(x),min(y),max(x),max(y)]

def collect_bbox(bboxes):
    mx,my,Mx,My = zip(*bboxes)
    return [min(mx),min(my),max(Mx),max(My)]

def make_json_feat(obj):
    res = {'type':'Feature',}
    #if obj.Type==elements.ElementType.ComplicatedPolygon:
    #    res['id'] = -1*obj.Id
    #else:
    res['id'] = obj.Id
    res['quadtree'] = list(elements.quadtree_tuple(obj.Quadtree))
    if obj.MinZoom >= 0:
        res['minzoom']  = obj.MinZoom
    res['properties'] = dict((t.key,t.val) for t in obj.Tags)
    if obj.Layer != 0 or obj.find_tag('layer')=='0':
        res['layer'] = obj.Layer
    if obj.Type==elements.ElementType.Point:
        res['geometry'] = {'type':'Point','coordinates':coord_json(obj.LonLat)}
        res['bounds'] = res['geometry']['coordinates']*2
    elif obj.Type==elements.ElementType.Linestring:
        res['geometry'] = {'type':'LineString', 'coordinates': [coord_json(l) for l in obj.LonLats]}
        res['bounds'] = ring_bbox(res['geometry']['coordinates'])
        res['way_length'] = round(obj.Length,1)
        if obj.ZOrder!=0:
            res['z_order'] = obj.ZOrder
    elif obj.Type==elements.ElementType.SimplePolygon:
        cc = [coord_json(l) for l in obj.LonLats]
        if obj.Reversed:
            cc.reverse()
        res['geometry'] = {'type':'Polygon', 'coordinates': [cc]}
        res['bounds'] = ring_bbox(res['geometry']['coordinates'][0])
        res['way_area'] = round(obj.Area,1)
        if obj.ZOrder!=0:
            res['z_order'] = obj.ZOrder

    elif obj.Type==elements.ElementType.ComplicatedPolygon:
        if len(obj.Parts)==0:
            res['geometry'] = {'type':'GeometryCollection','geometries':[]}
        elif len(obj.Parts)==1:
            res['geometry'] = {'type':'Polygon', 'coordinates': [[coord_json(l) for l in obj.Parts[0].outer_lonlats]]+[[coord_json(l) for l  in ii] for ii in obj.Parts[0].inner_lonlats]}
            res['bounds'] = ring_bbox(res['geometry']['coordinates'][0])
        else:
            cc = []
            xx,yy=[],[]
            for p in obj.Parts:
                pp = [[coord_json(l) for l in p.outer_lonlats]]+[[coord_json(l) for l in ii] for ii in p.inner_lonlats]
                cc.append(pp)
                xx.extend(x for x,y in pp[0])
                yy.extend(y for x,y in pp[0])
                
            res['geometry'] = {'type':'MultiPolygon', 'coordinates': cc}
            res['bounds']=[min(xx),min(yy),max(xx),max(yy)]
            
            
        
        res['way_area'] = round(obj.Area,1)
        if obj.ZOrder!=0:
            res['z_order'] = obj.ZOrder
    return res

def collect_all_geojson(tiles):
    tile={'quadtree':[0,0,0],'index':0,'end_date': None,'points':[],'linestrings':[],'simple_polygons': [], 'complicated_polygons': []}
    
    tbs = {
        elements.ElementType.Point: 'points',
        elements.ElementType.Linestring: 'linestrings',
        elements.ElementType.SimplePolygon: 'simple_polygons',
        elements.ElementType.ComplicatedPolygon: 'complicated_polygons'
    }
    
    
    for tl in tiles:
        if tl.EndDate and tile['end_date'] is None:
            tile['end_date'] = time.strftime("%Y-%m-%dT%H:%M:%S",time.gmtime(tl.EndDate))
        
        for obj in tl:
            tb = tbs.get(obj.Type)
            if not tb is None:
                tile[tb].append(make_json_feat(obj))
    
    tile['points'].sort(key=lambda x: x['id'])
    tile['linestrings'].sort(key=lambda x: x['id'])
    tile['simple_polygons'].sort(key=lambda x: x['id'])
    tile['complicated_polygons'].sort(key=lambda x: x['id'])
    return tile



