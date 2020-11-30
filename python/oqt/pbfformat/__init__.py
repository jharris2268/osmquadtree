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
from oqt import utils

from . import _pbfformat
from ._pbfformat import get_header_block, read_primitive_block, read_minimal_block
from ._pbfformat import ReadBlockFlags, calc_idset_filter, ReadBlocksIter
from ._pbfformat import read_blocks_primitive, read_blocks_minimal, read_blocks_merge_primitive
from ._pbfformat import WritePbfFile

from .filelocs import prep_poly,  get_locs, get_locs_single, Poly, read_poly_file

_pbfformat._ReadBlocksCaller = _pbfformat.ReadBlocksCaller
import numbers

class ReadBlocksCaller:
    def __init__(self, prfx, bbox, poly=None, lastdate=None):
        if not lastdate is None:
            if not isinstance(lastdate, numbers.Integral):
                lastdate=utils.get_date(lastdate)
                
                #raise Exception("??")# = _oqt.read_date(lastdate)
        else:
            lastdate=0
        
        self.bbox=bbox
        self.poly=prep_poly(poly)
        if self.bbox is None:
            if self.poly:
                ln=[l.lon for l in self.poly]
                lt=[l.lat for l in self.poly]
                self.bbox=utils.bbox(min(ln),min(lt),max(ln),max(lt))
            else:
                self.bbox=utils.bbox(-1800000000,-900000000,1800000000,900000000)
        if self.poly is None:
            self.poly=[]
        self.rbc = _pbfformat.make_read_blocks_caller(prfx, self.bbox, self.poly, lastdate)
    
    def read_primitive(self, cb, numblocks=512, numchan=4, flags=ReadBlockFlags(),filter=None):
        return _pbfformat.read_blocks_caller_read_primitive(self.rbc, cb, numchan, numblocks, flags, filter)
                    
    def read_minimal(self, cb, numblocks=512, numchan=4, flags=ReadBlockFlags(),filter=None):
        return _pbfformat.read_blocks_caller_read_minimal(self.rbc, cb, numchan, numblocks, flags, filter)            

    def num_tiles(self):
        return self.rbc.num_tiles()
        
    def calc_idset(self, bbox=None, poly=None):
        if bbox is None:
            bbox=self.bbox
        if poly is None:
            poly=self.poly
        else:
            poly=prep_poly(poly)
        return _pbfformat.calc_idset_filter(self.rbc, bbox, poly, 4)

_pbfformat.ReadBlocksCaller=ReadBlocksCaller


class ReadFile:
    def __init__(self, fn):
        self.fn=fn
        self.readfile = None
    
    def __iter__(self):
        self.readfile = _pbfformat.make_readfile(self.fn)
        return self
    
    def __next__(self):
        
        a = self.readfile.next()
        if not a:
            raise StopIteration()
        return a
            
        
