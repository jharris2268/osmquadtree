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
from . import _update, xmlchange
from oqt import pbfformat, utils
import json, csv, time, sys, subprocess, os

try:
    from urllib import urlopen as urllib_urlopen
except:
    from urllib.request import urlopen as urllib_urlopen

def make_filename(ts, roundtime=False):
    if roundtime:
        ts = int(round(ts/24./60/60)*24*60*60+0.5)

    tm = time.gmtime(ts)
    if roundtime:
        return "%04d%02d%02d.pbfc" % (tm.tm_year, tm.tm_mon, tm.tm_mday)
    return "%04d-%02d-%02dT%02d-%02d-%02d.pbfc" % (tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec)

def datestr(ts):
    tm = time.gmtime(ts)
    return "%04d-%02d-%02dT%02d:%02d:%02d" % (tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec)


class timer:
    def __init__(self):
        self.tms = [(time.time(),'')]
    def __call__(self, what):
        self.tms.append((time.time(), what))

    def total(self):
        ft = self.tms[0][0]
        lt = self.tms[-1][0]
        return lt-ft

    def __str__(self):
        if len(self.tms)<2: return ''
        maxln = max(len(b) for a,b in self.tms)
        res=[]
        for (a,b),(c,d) in zip(self.tms,self.tms[1:]):
            res.append("%s:%s %5.1fs" % (d, " "*(maxln-len(d)), c-a))
        
        res.append("TOTAL:%s %5.1fs" % (" "*(maxln-5), self.total()))
        return "\n".join(res)


def find_change(src_filenames, prfx, infiles, startdate, enddate, outfn, use_alt=False,allow_missing_users=False):
    print("call find_change",
        "%d src files: %s%s" % (len(src_filenames),src_filenames[0],(" => %s" % src_filenames[-1] if len(src_filenames)>1 else '')),
        repr(prfx),
        "%d infiles %s%s" % (len(infiles), infiles[0],('=> %s' % infiles[-1] if len(infiles)>1 else '')),
        repr(startdate),repr(enddate),repr(outfn),'allow_missing_users=',allow_missing_users)
        
    tm = timer()
    objs = _update.element_map()

    for src in src_filenames:
        _update.read_xml_change_file_em(src,True,objs,allow_missing_users)
    tm("read xml")
    
    aoe = _update.add_orig_elements_alt if use_alt else _update.add_orig_elements
    
    orig_allocs,qts,tree = aoe(objs,prfx, infiles)
    tm("find orig")
    _update.calc_change_qts(objs,qts)
    tm("calc qts")

    tiles = _update.find_change_tiles(objs,orig_allocs,tree,startdate,enddate)
    print("have %d tiles, %d objs" % (len(tiles),sum(map(len,tiles))))
    tm("find tiles")

    objs.clear()
    print("after objs.clear(), len(objs)=%d" % len(objs))
    del objs,qts,orig_allocs
    out = pbfformat.WritePbfFile(prfx+outfn,bounds=utils.bbox(-1800000000,-900000000,1800000000,900000000), numchan=4, indexed=True, dropqts=False, change=True, tempfile=False)
    print("calling out.write")
    out.write(tiles)
    print("calling out.finish()")
    ln,nt = out.finish()
    del tiles, out
    tm("write pbfc")
    print("calling _change.write_index_file")
    _update.write_index_file(prfx+outfn)
    tm("write index")
    print("done")

    print(tm)
    return tm.total(), ln, nt


def check_diffslocation(location, initial=None, srcprfx=None):
    if not os.path.exists(location):
        a,b=os.path.split(location)
        print("path %r doesn't exist, create dir %r" % (location, a))
        os.mkdir(a)
    diffs=[]
    if os.path.exists(location+'state.csv'):
        diffs = [l for l in csv.reader(open(location+'state.csv')) if l]
    tms=[]
    if srcprfx is not None:
        fd = max(int(diffs[-1][0]) if diffs else 0, initial or 0)
        if fd==0: raise Exception("??")
        new_diffs,tms_ = fetch_new_diffs(srcprfx, location, fd)
        tms.extend(tms_)
        if new_diffs:
            ww=csv.writer(open(location+'state.csv','a'))
            for a,b in new_diffs:
                ww.writerow([a,b])
                diffs.append([a,b])
    
    return diffs,tms

def fetch_new_diffs(srcprfx, diffsLocation, current_state):
    state, timestamp = get_state(srcprfx)
    newdiffs,tms=[],[]
    print("lastest state %d, current diff %d, add %d" % (state,current_state,state-current_state))
    if state > current_state:
        
        for v in range(current_state+1, state+1):
            st=time.time()
            c,d = fetch_diff(srcprfx,diffsLocation, v)
            newdiffs.append([c,d])
            tms.append(("fetch %d" % c, time.time()-st))
    return newdiffs, tms

def fetch_diff(srcprfx, diffsLocation, state_in):
    state, timestamp = get_state(srcprfx, state_in)
    diff_url = get_diff_url(srcprfx, state)
    outfn = "%s%d.osc.gz" % (diffsLocation, state)
    subprocess.check_call(['wget', '-O', outfn, diff_url])
    return state,timestamp

def get_diff_url(srcprfx, state, return_state=False):
    if state is None:
        if not return_state:
            raise Exception("must specify state to retrive diff url")
        return "%s/state.txt" % srcprfx
    a=state/1000000
    b=(state%1000000)/1000
    c=state%1000
    root = "%s%03d/%03d/%03d" % (srcprfx,a,b,c)
    
    if return_state:
        return root+'.state.txt'
    return root+'.osc.gz'
    
    

def get_state(srcprfx, state_in=None):
    state_url = get_diff_url(srcprfx, state_in, True)

    req=urllib_urlopen(state_url)
    statef = [l.decode().strip().split("=") for l in req.readlines()]
    stated = dict(x for x in statef if len(x)==2)
    assert "sequenceNumber" in stated
    assert "timestamp" in stated
    ts=stated["timestamp"].replace("\\","")
   
    tp=time.strptime(ts, "%Y-%m-%dT%H:%M:%SZ")
    tf=time.strftime("%Y-%m-%dT%H-%M-%S", tp)
    
    if state_in is not None:
        assert int(stated["sequenceNumber"]) == state_in
    
    return int(stated["sequenceNumber"]), tf
    

def run_initial(prfx, orig_fn, end_date, diffs_location, initial_state, **kw):
    settings = {
        "RoundTime": kw['roundtime'].lower() in ('yes','true') if 'roundtime' in kw else True,
        "DiffsLocation": diffs_location,
        "InitialState": initial_state,
        "SourcePrfx": kw['sourceprfx'] if 'sourceprfx' in kw else "https://planet.openstreetmap.org/replication/day/",
    }
    if 'allowmissingusers' in kw and kw['allowmissingusers'].lower() in ('yes','true'):
        settings['AllowMissingUsers']=True
    if 'mergeoscfiles' in kw and kw['mergeoscfiles'].lower() in ('yes','true'):
        settings['MergeOscFiles']=True
    print(settings)
    json.dump(settings, open(prfx+'settings.json','w'))
    nt = _update.write_index_file(prfx+orig_fn)
    filelist = [{'Filename': orig_fn, 'EndDate': end_date, 'NumTiles':nt, 'State': initial_state}]
    json.dump(filelist, open(prfx+'filelist.json','w'))

def split_available(diffsLocation, available, merge_osc_files, max_osc_size=100*1024*1024):
    if not merge_osc_files:
        for state, enddate in available:
            src=["%s%d.osc.gz" % (diffsLocation, state)]
            yield src, state, enddate
        
        return
    
    src = []
    curr_size=0
    for state, enddate in available:
        next_src="%s%d.osc.gz" % (diffsLocation, state)
        src.append(next_src)
        curr_size += os.stat(next_src).st_size
        if curr_size >= max_osc_size:
            yield src, state, enddate
            src=[]
            curr_size=0
    
    if src:
        yield src, available[-1][0], available[-1][1]
        
    
    

def run_update(prfx, nd=None, check_new_diffs=True):
    settings=None
    try:
        settings = json.load(open(prfx+'settings.json'))
    except:
        print("no settings file at %s" % (prfx,))
        return 1

    diffsLocation = settings['DiffsLocation']
    allow_missing_users=settings['AllowMissingUsers'] if 'AllowMissingUsers' in settings else False
    merge_osc_files = settings['MergeOscFiles'] if 'MergeOscFiles' in settings else False
    
    
    diffs, tms = check_diffslocation(diffsLocation, settings['InitialState'],settings['SourcePrfx'] if check_new_diffs else None)

    files = json.load(open(prfx+'filelist.json'))

    last = files[-1]['State']

    available = [(int(r[0]),xmlchange.read_timestamp(r[1])) for r in diffs if r and int(r[0])>last]
    if not available:
        print("no new diffs available")
        return 1
    if nd is not None and len(available)>nd:
        available = available[:nd]
    
    if len(available)==1:
        print("update 1 osc: %d %s" % (available[0][0], available[0][-1]))
    else:
        print("update %d osc: %d %s => %d %s" % (len(available),available[0][0], available[0][-1],available[-1][0], available[-1][-1]))

    for src, state, enddate in split_available(diffsLocation, available, merge_osc_files):
        
        infiles = [f['Filename'] for f in files]
        outfn  = make_filename(enddate, settings['RoundTime'])

        startdate = xmlchange.read_timestamp(files[-1]['EndDate'])
        tm, ln, nt = find_change(src, prfx, infiles, startdate, enddate, outfn,allow_missing_users=allow_missing_users)
        tms.append((state, tm))
        files.append({'State':state, 'NumTiles': nt, 'EndDate': datestr(enddate), 'Filename':outfn})


    
    json.dump(files, open(prfx+'filelist.json','w'))
    if len(tms)>1:
        for a,b in tms:
            print("%10s: %5.1fs" % (a,b))
        print("%10s: %5.1fs" % ('TOTAL',sum(b for a,b in tms)))
        

def run_droplast(prfx):
    files = json.load(open(prfx+'filelist.json'))

    last = files.pop()
    
    print('drop', last)
    os.remove(prfx+last['Filename'])
    
    json.dump(files, open(prfx+'filelist.json','w'))
    
    
