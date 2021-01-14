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

from ._utils import bbox_empty, bbox_planet, get_logger
from ._utils import LonLat, point_in_poly, segment_intersects, line_intersects, line_box_intersects
from ._utils import compress, decompress, compress_gzip, decompress_gzip
from ._utils import checkstats, file_size
from ._utils import PbfTag, PbfValue, PbfData, read_all_pbf_tags, pack_pbf_tags, read_packed_delta, read_packed_int, write_packed_delta, write_packed_int, zig_zag, un_zig_zag

from .misc import *

from . import _utils
import oqt.elements

import time,calendar
get_date=lambda t: calendar.timegm(time.strptime(t,"%Y-%m-%dT%H:%M:%S"))

bbox = _utils.bbox
bbox.__repr__=lambda b: "bbox(% 10d, % 10d, % 10d, % 10d)" % (b.minx,b.miny,b.maxx,b.maxy)
bbox.__len__ = lambda b: 4


bbox.overlaps_quadtree = lambda bx, q: oqt.elements.overlaps_quadtree(bx,q)

def bbox_getitem(b, i):
    if i==0: return b.minx
    if i==1: return b.miny
    if i==2: return b.maxx
    if i==3: return b.maxy
    raise IndexError()

bbox.__getitem__ = bbox_getitem

def bbox_transform(bx):
    ll = LonLat(bx[0],bx[1]).transform
    ur = LonLat(bx[2],bx[3]).transform
    return [ll.x,ll.y,ur.x,ur.y]
bbox.transform = bbox_transform


PbfTag.__repr__ = lambda p: "(%d, %d, %.20s)" % (p.tag, p.value, p.data)
PbfTag.tuple = property(lambda p: (p.tag, p.value, p.data))


class py_logger(_utils.Logger):
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
_utils.set_logger(_logger)



