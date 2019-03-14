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

from . import _oqt as oq
import struct

def read_file_block(fileobj):
    bl, = struct.unpack('>L', fileobj.read(4))
    
    hh = fileobj.read(bl)
    
    ty, dl = None,None
    for tg in oq.read_all_pbf_tags(hh):
        if tg.tag==1:
            ty = tg.data
        elif tg.tag==3:
            dl = tg.value
    #print(bl,ty,dl)
    hh2 = fileobj.read(dl)
    
    uc=None
    dd=None
    for tg in oq.read_all_pbf_tags(hh2):
        if tg.tag==1:
            return ty, tg.data
        
        elif tg.tag==2:
            uc=tg.value
        elif tg.tag==3:
            dd=tg.data
    
    return ty, oq.decompress(dd,uc)
    
        
    
class PbfFile:
    def __init__(self, fn):
        self.fileobj = open(fn)
        
        
    @property
    def fileloc(self):
        return self.fileobj.tell()
        
    def read_block(self, loc=None):
        if not loc is None:
            self.fileobj.seek(loc)
        
        return read_file_block(self.fileobj)

    def __iter__(self):
        self.fileobj.seek(0)
        return self
    
    def next(self):
        try:
            return self.read_block()
        except:
            raise StopIteration()
