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

from . import _calcqts
from ._calcqts import calcqts, make_qtstore, make_qtstore_split,find_way_quadtrees,write_qts_file,write_waynodes


from oqt import utils, pbfformat

def qtstore_iter(qts):
    f=qts.first()
    while f != -1:
        yield f, qts[f]
        f=qts.next(f)

def run_calcqts_alt(origfn, qtsfn=None, numchan=4, splitways=True, resort=True, buffer=0.05, max_depth=18):
    utils.get_logger().reset_timing()
    if qtsfn is None:
        qtsfn=origfn[:-4]+"-qts.pbf"
    waynodes_fn = qtsfn+"-waynodes"
    
    wns,rels,nodes_fn,node_locs = write_waynodes(origfn, waynodes_fn, numchan, resort)
    
    utils.checkstats()
    way_qts = make_qtstore_split(1<<20,not splitways)
    
    if splitways:
        midway = 256<<20
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, midway,True)
        utils.checkstats()
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, midway, 2*midway,True)
        utils.checkstats()
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 2*midway, 0,True)
        utils.checkstats()
        
    else:
        find_way_quadtrees(nodes_fn, node_locs, numchan, way_qts, wns, buffer, max_depth, 0, 0,False)
        utils.checkstats()
    
    
    write_qts_file(qtsfn, nodes_fn, numchan, node_locs, way_qts, wns, rels, buffer, max_depth)
    utils.get_logger().timing_messages()
    return 1
