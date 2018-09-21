from __future__ import print_function
from . import _oqt as oq
import json, psycopg2,csv
from .utils import addto, Prog, get_locs, replace_ws, addto_merge
import time,sys,re


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

add_highway = {
    'highway': oq.parenttag_spec('highway','parent_highway','highway',highway_prio),
    'service': oq.parenttag_spec('highway','parent_service','service',{}),
    'railway': oq.parenttag_spec('railway','parent_highway','highway',highway_prio),
}
def has_mem(ct,k):
    for a,b,c,d in ct:
        if k==a:
            return True
    return False
def make_tag_cols(coltags):
    common = [("osm_id","bigint"),("tile","bigint"), ("quadtree","bigint")]

    post = []
    if has_mem(coltags,'*'):
        #post.append(('other_tags','jsonb'))
        post.append(('other_tags','hstore'))
    elif has_mem(coltags, 'XXX'):
        #post.append(('tags', 'jsonb'))
        post.append(('tags', 'hstore'))
    if has_mem(coltags, 'layer'):
        post.append(('layer','int'))
    if has_mem(coltags,'minzoom'):
        post.append(('minzoom','integer'))
    point = common+[(a,"text" ) for a,b,c,d in coltags if b and not a in ("*","minzoom",'XXX','layer')]+post+[("way","geometry(Point,3857)")]
    line = common+[(a,"text") for a,b,c,d in coltags if c and not a in ("*","minzoom",'XXX','layer')]+post+[('z_order','int'),('length','float'),("way","geometry(LineString,3857)")]
    poly = common[:1]+[('part','int')]+common[1:]+[(a,"text") for a,b,c,d in coltags if d and not a in ("*","minzoom",'XXX','layer')]+post+[('z_order','int'),('way_area','float'),("way","geometry(Polygon,3857)")]
    
    return point,line,poly
    


def create_tables(curs, table_prfx,coltags):
    curs.execute("begin")
    for t in ('point','line','polygon','roads'):
        curs.execute("drop table if exists "+table_prfx+t+" cascade" )
    
    point_cols,line_cols, poly_cols = make_tag_cols(coltags)
    point_create = "create table %spoint (%s) with oids" % (table_prfx, ", ".join('"%s" %s' % (a,b) for a,b in point_cols))
    line_create = "create table %sline (%s) with oids" % (table_prfx, ", ".join('"%s" %s' % (a,b) for a,b in line_cols))
    poly_create = "create table %spolygon (%s) with oids" % (table_prfx, ", ".join('"%s" %s' % (a,b) for a,b in poly_cols))
    
    #point_create="create table "+table_prfx+"point (osm_id bigint, tile bigint, quadtree bigint, %s, way geometry(Point,3785)) with oids" % ", ".join("other_tags jsonb" if a=="*" else "minzoom integer" if a=="minzoom" else ('"%s" %s' % (a,'text')) for a,b,c,d in coltags if b)
    #line_create ="create table "+table_prfx+"line (osm_id bigint, tile bigint, quadtree bigint, %s, z_order int, way geometry(LineString,3785)) with oids" % ", ".join("other_tags jsonb" if a=="*" else "minzoom integer" if a=="minzoom" else ('"%s" %s' % (a,'text')) for a,b,c,d in coltags if c)
    #poly_create ="create table "+table_prfx+"polygon (osm_id bigint, part int, tile bigint, quadtree bigint, %s, z_order int, way_area float,way geometry(Polygon,3785)) with oids" % ", ".join("other_tags jsonb" if a=="*" else "minzoom integer" if a=="minzoom" else ('"%s" %s' % (a,'text')) for a,b,c,d in coltags if d)
    if curs==None:
        return point_create, line_create, poly_create
    curs.execute(point_create)
    curs.execute(line_create)
    curs.execute(poly_create)
    curs.execute("commit")




def create_indices(curs, table_prfx, extraindices=False, planet=False):
    write_indices(curs,table_prfx,indices)
    if extraindices:
        write_indices(curs,table_prfx,extra)
    if planet:
        write_indices(curs,table_prfx,planetosm)
        
    
def write_indices(curs,table_prfx, inds):
    ist=time.time()
    curs.execute("begin")
    for ii in inds:
        qu=ii.replace("%ZZ%", table_prfx)
        sys.stdout.write("%-150.150s" % (replace_ws(qu),))
        sys.stdout.flush()
        qst=time.time()
        
        curs.execute(qu)
        
        sys.stdout.write(" %7.1fs\n" % (time.time()-qst))
        sys.stdout.flush()
    curs.execute('commit')
    print("created indices in %8.1fs" % (time.time()-ist))

indices = [
"""CREATE TABLE %ZZ%roads AS
    SELECT osm_id,null as part,tile,quadtree,name,ref,admin_level,highway,railway,boundary,
            service,tunnel,bridge,z_order,covered,surface, minzoom, way
        FROM %ZZ%line
        WHERE highway in (
            'secondary','secondary_link','primary','primary_link',
            'trunk','trunk_link','motorway','motorway_link')
        OR railway is not null

    UNION SELECT osm_id,part,tile,quadtree,name,null as ref, admin_level,null as highway,
            null as railway, boundary, null as service,
            null as tunnel,null as bridge, 0  as z_order,null as covered,null as surface,minzoom, 
            st_exteriorring(way) as way
        FROM %ZZ%polygon WHERE
            osm_id<0 and boundary='administrative'""",
"CREATE INDEX %ZZ%point_way         ON %ZZ%point    USING gist  (way)",
"CREATE INDEX %ZZ%line_way          ON %ZZ%line     USING gist  (way)",
"CREATE INDEX %ZZ%polygon_way       ON %ZZ%polygon  USING gist  (way)",
"CREATE INDEX %ZZ%roads_way         ON %ZZ%roads    USING gist  (way)",]

extras = [
"CREATE INDEX %ZZ%point_osmid       ON %ZZ%point    USING btree (osm_id)",
"CREATE INDEX %ZZ%line_osmid        ON %ZZ%line     USING btree (osm_id)",
"CREATE INDEX %ZZ%polygon_osmid     ON %ZZ%polygon  USING btree (osm_id)",
"CREATE INDEX %ZZ%roads_osmid       ON %ZZ%roads    USING btree (osm_id)",
"CREATE INDEX %ZZ%roads_admin       ON %ZZ%roads    USING gist  (way) WHERE boundary = 'administrative'",
"CREATE INDEX %ZZ%roads_roads_ref   ON %ZZ%roads    USING gist  (way) WHERE highway IS NOT NULL AND ref IS NOT NULL",
"CREATE INDEX %ZZ%roads_admin_low   ON %ZZ%roads    USING gist  (way) WHERE boundary = 'administrative' AND admin_level IN ('0', '1', '2', '3', '4')",
"CREATE INDEX %ZZ%line_ferry        ON %ZZ%line     USING gist  (way) WHERE route = 'ferry'",
"CREATE INDEX %ZZ%line_river        ON %ZZ%line     USING gist  (way) WHERE waterway = 'river'",
"CREATE INDEX %ZZ%line_name         ON %ZZ%line     USING gist  (way) WHERE name IS NOT NULL",
"CREATE INDEX %ZZ%polygon_military  ON %ZZ%polygon  USING gist  (way) WHERE landuse = 'military'",
"CREATE INDEX %ZZ%polygon_nobuilding ON %ZZ%polygon USING gist  (way) WHERE building IS NULL",
"CREATE INDEX %ZZ%polygon_name      ON %ZZ%polygon  USING gist  (way) WHERE name IS NOT NULL",
"CREATE INDEX %ZZ%polygon_way_area_z6 ON %ZZ%polygon USING gist (way) WHERE way_area > 59750",
"CREATE INDEX %ZZ%point_place       ON %ZZ%point    USING gist  (way) WHERE place IS NOT NULL AND name IS NOT NULL",
"create view %ZZ%highway as (select * from %ZZ%line where z_order is not null and z_order!=0)",
"""create table %ZZ%boundary as (select osm_id,part,tile,quadtree,name,admin_level,boundary,minzoom, 
            st_exteriorring(way) as way from %ZZ%polygon where osm_id<0 and boundary='administrative')""",
"create view %ZZ%building as (select * from %ZZ%polygon where building is not null and building != 'no')",
"create index %ZZ%highway_view on %ZZ%line using gist (way) where z_order is not null and z_order!=0",
"create index %ZZ%boundary_view on %ZZ%boundary using gist (way)",
"create index %ZZ%building_view on %ZZ%polygon using gist (way) where building is not null and building != 'no'",

]

planetosm = [
"drop view if exists planet_osm_point",
"drop view if exists planet_osm_line",
"drop view if exists planet_osm_polygon",
"drop view if exists planet_osm_roads",
"drop view if exists planet_osm_highway",
"drop view if exists planet_osm_boundary",
"drop view if exists planet_osm_building",
"create view planet_osm_point as (select * from %ZZ%point)",
"create view planet_osm_line as (select * from %ZZ%line)",
"create view planet_osm_polygon as (select * from %ZZ%polygon)",
"create view planet_osm_roads as (select * from %ZZ%roads)",
"create view planet_osm_highway as (select * from %ZZ%highway)",
"create view planet_osm_boundary as (select * from %ZZ%boundary)",
"create view planet_osm_building as (select * from %ZZ%building)",
]

def create_tables_lowzoom(curs, prfx, newprefix, minzoom, simp=None, cols=None):
    
    
    queries = []
    
    for tab in ("point", "line", "polygon","roads"):
        queries.append("drop table if exists %ZZ%"+tab+" cascade")
    
    
    
    for tab in ("point", "line", "polygon", "roads"):
    
        colsstr="*"
        if not simp is None:
            ncols=None            
            
            if cols is None:
                curs.execute("select * from "+prfx+tab+" limit 0")
                ncols=['"%s"'  % c[0] for c in curs.description if c[0]!='way']
            else:
                ncols=cols[:]
                
            if tab=='point':
                ncols.append('way')
            else:
                ncols.append("st_simplify(way, "+str(simp)+") as way")
            colsstr = ", ".join(ncols)
            
    
        queries.append("create table %ZZ%" + tab + " as "+\
             "(select "+colsstr+" from "+prfx+tab+\
             " where minzoom <= "+str(minzoom)+")")
             
    
    queries += indices[1:]
    queries += extras
    
    print("call %d queries..." % len(queries))
    write_indices(curs,newprefix,queries)
    
    
def create_views_lowzoom(curs, prfx, newprefix, minzoom, indicies=False):
    queries=[]
    for tab in ("point", "line", "polygon","roads", "highway", "building", "boundary"):
        queries.append("drop view if exists %ZZ%"+tab)
        queries.append("create view %ZZ%"+tab+" as (select * from "+prfx+tab+" where minzoom <= "+str(minzoom)+")")
    
    if indicies:
        for tab in ("point", "line", "polygon","roads","boundary"):
            queries.append("create index %ZZ%"+tab+"_way on "+prfx+tab+" using gist(way) where minzoom <= "+str(minzoom))
        
        queries.append("create index %ZZ%highway_way on "+prfx+"line using gist (way) where z_order is not null and z_order!=0 and minzoom <= "+str(minzoom))
        queries.append("create index %ZZ%building_view on "+prfx+"polygon using gist (way) where building is not null and building != 'no' and minzoom <= "+str(minzoom))
    write_indices(curs,newprefix,queries)
    

def read_blocks(prfx, box_in, lastdate=None, objflags=7, numchan=4):
    
    tiles=[]
    result = addto(tiles)
    fns,locs,box = get_locs(prfx,box_in,lastdate)
    
    oq.read_blocks_merge_primitive(fns, result, locs, numchan=numchan, objflags=objflags)
    return tiles

read_style = lambda stylefn: dict((t['Tag'], oq.style_info(IsFeature=t['IsFeature'],IsArea=t['IsPoly']!='no',IsNode=t['IsNode'],IsWay=t['IsWay'], IsOtherTags=('IsOtherTags' in t and t['IsOtherTags']))) for t in json.load(open(stylefn))) 
read_minzoom = lambda minzoomfn: [(int(a),b,c,int(d)) for a,b,c,d,e in (r[:5] for r in csv.reader(open(minzoomfn)) if len(r)>=5)]
def process_geometry(prfx, box_in, stylefn, collect=True, outfn=None, lastdate=None,indexed=False,minzoomfn=None,nothread=False,mergetiles=False, groups=None, numchan=4,minlen=0,minarea=5):
    tiles={} if mergetiles else []
    
    params = oq.geometry_parameters()
        
    params.numchan=numchan
    params.apt_spec = add_highway
    params.add_rels=True
    params.add_mps=True
    params.recalcqts=True
    
    if not groups is None:
        params.groups=groups
    
    if outfn:
        params.outfn=outfn
    params.indexed=indexed
    
    if box_in is None:
        
        box_in=oq.bbox(-1800000000,-900000000,1800000000,900000000)
    
    
    
    params.filenames,params.locs, params.box = get_locs(prfx,box_in,lastdate)
    print("%d fns, %d qts, %d blocks" % (len(params.filenames),len(params.locs),sum(len(v) for k,v in params.locs.items())))
    if mergetiles and collect:
        for l in locs:
            tiles[l]=oq.primitiveblock(0,0)
            tiles[l].quadtree=l
    
    params.style = read_style(stylefn)
    print("%d styles" % len(style))
    
    if 'minzoom' in style and minzoomfn is not None:
        params.findmz = oq.findminzoom_onetag(read_minzoom(minzoomfn),minlen=minlen,minarea=minarea)
        print ('findmz', params.findmz)
    cnt,errs=None,None
    
    
    
    if len(locs) > 2500 and outfn is not None:
        collect=False
    params.result = Prog((addto_merge(tiles, minzoomfn!=None) if mergetiles else addto(tiles)), locs) if collect else None
    
    if nothread:
        cnt, errs = oq.process_geometry_nothread(params)
    elif groups is not None:
        cnt, errs = oq.process_geometry_sortblocks(params)
    else:
        cnt, errs = oq.process_geometry(params)
    
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
    


def to_json(tile,split=True):
    res = {}
    
    
    for obj in tile:
        
        tab=None
        if split:
            if obj.Type==oq.elementtype.Point:
                tab = 'point'
            if obj.Type==oq.elementtype.Linestring:
                tab='line'
            if obj.Type in (oq.elementtype.SimplePolygon, oq.elementtype.ComplicatedPolygon):
                tab='polygon'
        if not tab in res:
            res[tab] = {'type':'FeatureCollection','features': [], 'quadtree': oq.quadtree_tuple(tile.quadtree), 'bbox': None, 'table': tab}
        
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
    if obj.Type==6:
        res['id'] = -1*obj.Id
    else:
        res['id'] = obj.Id
    res['quadtree'] = oq.quadtree_tuple(obj.Quadtree)
    res['minzoom']  = obj.MinZoom
    res['properties'] = dict((t.key,t.val) for t in obj.Tags)
    if obj.Type==3:
        res['geometry'] = {'type':'Point','coordinates':coord_json(obj.LonLat)}
        res['bbox'] = res['geometry']['coordinates']*2
    elif obj.Type==4:
        res['geometry'] = {'type':'LineString', 'coordinates': map(coord_json, obj.LonLats)}
        res['bbox'] = ring_bbox(res['geometry']['coordinates'])
        res['properties']['way_length'] = obj.Length
        res['properties']['z_order'] = obj.ZOrder
    elif obj.Type==5:
        res['geometry'] = {'type':'Polygon', 'coordinates': [map(coord_json, obj.LonLats)]}
        res['bbox'] = ring_bbox(res['geometry']['coordinates'][0])
        res['properties']['way_area'] = obj.Area
        res['properties']['z_order'] = obj.ZOrder

    elif obj.Type==6:
        res['geometry'] = {'type':'Polygon', 'coordinates': [map(coord_json, obj.OuterLonLats)]+[map(coord_json,ii) for ii in obj.InnerLonLats]}
        res['bbox'] = ring_bbox(res['geometry']['coordinates'][0])
        res['properties']['way_area'] = obj.Area
        res['properties']['z_order'] = obj.ZOrder
    return res





def write_to_postgis(prfx, box_in, stylefn, connstr,tabprfx,lastdate=None,minzoomfn=None,nothread=False, numchan=4, extraindices=False):
    fns,locs,box = get_locs(prfx,box_in,lastdate)
    print("%d fns, %d qts, %d blocks" % (len(fns),len(locs),sum(len(v) for k,v in locs.items())))
    style = read_style(stylefn)
    print("%d styles" % len(style))
    coltags = sorted((k,v.IsNode,v.IsWay,v.IsWay) for k,v in style.items() if k not in ('z_order','way_area'))
    if tabprfx and not tabprfx.endswith('_'):
        tabprfx = tabprfx+'_'
    
    create_tables(psycopg2.connect(connstr).cursor(), tabprfx,coltags)
    
    findmz=None
    if 'minzoom' in style and minzoomfn is not None:
        findmz = oq.findminzoom_onetag(read_minzoom(minzoomfn),minlen=0,minarea=5)
        print ('findmz', findmz)
    
        
    cnt,errs=None,None
    if nothread:
        cnt, errs = oq.process_geometry_nothread(fns, Prog(locs=locs), locs, 128, style, box, add_highway, True, True, True, findmz, "", False, connstr, tabprfx, coltags)
    else:
        cnt, errs = oq.process_geometry(fns, Prog(locs=locs), locs, numchan, 128, style, box, add_highway, True, True, True, findmz, "", False, connstr, tabprfx, coltags)
    create_indices(psycopg2.connect(connstr).cursor(), tabprfx, extraindices, extraindices)




