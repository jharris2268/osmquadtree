from ._utils import bbox_empty, bbox_planet, get_logger
from ._utils import LonLat, point_in_poly, segment_intersects, line_intersects, line_box_intersects
from ._utils import compress, decompress, compress_gzip, decompress_gzip
from ._utils import checkstats, file_size
from ._utils import PbfTag, PbfValue, PbfData, read_all_pbf_tags, pack_pbf_tags, read_packed_delta, read_packed_int, zig_zag, un_zig_zag
from .filelocs import *
from .misc import *

from . import _utils
import oqt._block

bbox = _utils.bbox
bbox.__repr__=lambda b: "bbox(% 10d, % 10d, % 10d, % 10d)" % (b.minx,b.miny,b.maxx,b.maxy)
bbox.__len__ = lambda b: 4


bbox.overlaps_quadtree = lambda bx, q: oqt._block.overlaps_quadtree(bx,q)

def bbox_getitem(b, i):
    if i==0: return b.minx
    if i==1: return b.miny
    if i==2: return b.maxx
    if i==3: return b.maxy
    raise IndexError()

bbox.__getitem__ = bbox_getitem



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


def find_tag(obj, idx):
    for tg in obj.Tags:
        if tg.key==idx:
            return tg.val
    return None
