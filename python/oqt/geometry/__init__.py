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

from . import _geometry, style, process, minzoomvalues

from ._geometry import Point,Linestring,SimplePolygon, ComplicatedPolygon, WayWithNodes, read_blocks_geometry
from oqt.elements import find_tag
from .process import process_geometry, read_blocks, make_json_feat, to_json
#from .postgis import write_to_csvfile, write_to_postgis


Point.find_tag = find_tag
Linestring.find_tag = find_tag
SimplePolygon.find_tag = find_tag
ComplicatedPolygon.find_tag = find_tag

ComplicatedPolygon.OuterRefs = property(lambda cp: _geometry.ringpart_refs(cp.OuterRing))
ComplicatedPolygon.OuterLonLats = property(lambda cp: _geometry.ringpart_lonlats(cp.OuterRing))
ComplicatedPolygon.InnerRefs = property(lambda cp: [_geometry.ringpart_refs(ii) for ii in cp.InnerRings])
ComplicatedPolygon.InnerLonLats = property(lambda cp: [_geometry.ringpart_lonlats(ii) for ii in cp.InnerRings])


Point.__repr__ = lambda p: "Point(%10d %.50s %-18s [% 8d % 8d] )" % (p.Id,p.Tags,p.Quadtree,p.LonLat.lon,p.LonLat.lat)
Linestring.__repr__ = lambda l: "Linestring(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (l.Id,l.Tags,l.Quadtree,len(l.Refs),l.Length)
SimplePolygon.__repr__ = lambda p: "SimplePolygon(%10d %.50s %-18s [% 4d pts, %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,len(p.Refs),p.Area)
ComplicatedPolygon.__repr__ = lambda p: "ComplicatedPolygon(%10d %.50s %-18s [P. %2d % 4d pts (%2d ints), %6.1fm] )" % (p.Id,p.Tags,p.Quadtree,p.Part,len(p.OuterRefs),len(p.InnerRings),p.Area)

Point.to_json = property(make_json_feat)
Linestring.to_json = property(make_json_feat)
SimplePolygon.to_json = property(make_json_feat)
ComplicatedPolygon.to_json = property(make_json_feat)
