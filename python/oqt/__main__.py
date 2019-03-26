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
from . import xmlchange, update, sys, run_calcqts_alt
if len(sys.argv)>=3 and sys.argv[1]=='update':
    nd = None
    if len(sys.argv)>3:
        nd = int(sys.argv[3])
    r=update.run_update(sys.argv[2], nd)
    sys.exit(r)

elif len(sys.argv)>=7 and sys.argv[1]=='initial':
    prfx = sys.argv[2]
    origfn = sys.argv[3]
    enddate = sys.argv[4]
    try:
        xmlchange.read_timestamp(enddate)
    except:
        raise Exception("can't understand timestamp %s" % repr(enddate))
    
    diffs = sys.argv[5]
       
    state = int(sys.argv[6])
    
    kw = {}
    if len(sys.argv)>7:
        for a in sys.argv[7:]:
            if not a.count("=")==1:
                raise Exception("invalid keyword %s" % a)
                
            k,v=a.split("=")
            kw[k.lower()] = v
        #print(kw)
    r=update.run_initial(prfx, origfn, enddate, diffs, state, **kw)
    sys.exit(r)

elif len(sys.argv)>=3 and sys.argv[1]=='droplast':
    r=update.run_droplast(sys.argv[2])
    sys.exit(r)

elif len(sys.argv)>=3 and sys.argv[1]=='calcqts':
    qtsfn=None
    if len(sys.argv)>3:
        for s in sys.argv[3:]:
            if s.startswith('qtsfn='):
                qtsfn=s[6:]
                
    r=run_calcqts_alt(sys.argv[2], qtsfn)
    sys.exit(r)

else:
    print("""
%ZZ% update <prfx> <[optional] number of days>
%ZZ% initial <prfx> <origfn> <enddate> <diff location> <state> {[optional] RoundTime=<true|false> AllowMissingUsers=<true|false> SourcePrfx=<replication url base (default https://planet.openstreetmap.org/replication/day/)> MergeOscFiles=<true|false>}
%ZZ% droplast <prfx>
%ZZ% calcqts <prfx> {[optional] qtsfn=<default prfx[:-4]+'-qts.pbf'}
""".replace("%ZZ%", sys.argv[0]))
