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


from . import _sorting
from ._sorting import QtTree, QtTreeItem, sortblocks, mergechanges

from oqt import utils, pbfformat


def iter_tree(tree, i=0):
    t=tree.at(i)
    if t.weight:
        yield t
    for i in range(4):
        if t.children(i):
            for x in iter_tree(tree,t.children(i)):
                yield x
QtTree.__iter__ = iter_tree
QtTreeItem.__repr__ = lambda t: "qttree_item(%6d, %-29s %6d, %10d)" % (t.idx,repr(quadtree(t.qt))+",", t.total, t.weight)

