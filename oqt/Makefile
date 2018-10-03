CC = g++
CFLAGS = -pthread -std=c++14 -O3 -Wall -fPIC -I../include/ -I../thirdparty/ -I../thirdparty/xmlinspector/
LIBS = -lz -lstdc++fs -lpq
#LIBS = -lz
ROOT_OBJS = count.o
HEADERS = 
PBFFORMAT_OBJS = pbfformat/objsidset.o pbfformat/readfileblocks.o pbfformat/writeblock.o pbfformat/fileblock.o pbfformat/readminimal.o pbfformat/writepbffile.o pbfformat/readblockscaller.o pbfformat/readfile.o pbfformat/readfileparallel.o pbfformat/readblock.o
ELEMENT_OBJS = elements/combineblocks.o elements/element.o elements/baseelement.o elements/way.o elements/node.o elements/relation.o elements/geometry.o elements/quadtree.o
UTILS_OBJS = utils/bbox.o utils/compress.o utils/date.o utils/geometry.o utils/logger.o utils/operatingsystem.o utils/timing.o utils/pbf/packedint.o utils/pbf/protobuf.o utils/pbf/varint.o utils/pbf/fixedint.o
UPDATE_OBJS = update/applychange.o update/xmlchange.o update/update.o
GEOM_OBJS = geometry/addparenttags.o  geometry/addwaynodes.o  geometry/geometrytypes.o  geometry/handlerelations.o  geometry/makegeometries.o  geometry/multipolygons.o  geometry/postgiswriter.o
SORTING_OBJS = sorting/common.o sorting/splitbyid.o sorting/sortgroup.o sorting/sortblocks.o sorting/resortobjects.o sorting/final.o sorting/mergechanges.o sorting/tempobjs.o sorting/qttree.o sorting/qttreegroups.o
CALCQTS_OBJS = calcqts/qtstore.o calcqts/qtstoresplit.o calcqts/calculaterelations.o calcqts/waynodes.o calcqts/waynodesfile.o calcqts/writewaynodes.o calcqts/writeqts.o calcqts/calcqtsinmem.o calcqts/calcqts.o calcqts/wayqts.o
OBJS = $(ROOT_OBJS) $(GEOM_OBJS) $(PBFFORMAT_OBJS) $(ELEMENT_OBJS) $(UTILS_OBJS) $(UPDATE_OBJS) $(SORTING_OBJS) $(CALCQTS_OBJS)

default: oqt liboqt.so.0.0
oqt: oqt_main.o liboqt.so.0.0
	$(CC) $(CFLAGS) -o oqt oqt_main.o $(LIBS) -L/usr/local/lib -loqt #-ltcmalloc

liboqt.so.0.0: $(OBJS)
	$(CC) $(CFLAGS) -o liboqt.so.0.0 -shared $(OBJS) $(LIBS)
 
count.o:  ../src/count.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/count.cpp

oqt_main.o: ../src/oqt_main.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/oqt_main.cpp



geometry/addparenttags.o: ../src/geometry/addparenttags.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/addparenttags.cpp -o geometry/addparenttags.o

geometry/geometrytypes.o: ../src/geometry/geometrytypes.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/geometrytypes.cpp -o geometry/geometrytypes.o
geometry/handlerelations.o: ../src/geometry/handlerelations.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/handlerelations.cpp -o geometry/handlerelations.o
geometry/makegeometries.o: ../src/geometry/makegeometries.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/makegeometries.cpp -o geometry/makegeometries.o
geometry/multipolygons.o: ../src/geometry/multipolygons.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/multipolygons.cpp -o geometry/multipolygons.o
geometry/addwaynodes.o: ../src/geometry/addwaynodes.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/addwaynodes.cpp -o geometry/addwaynodes.o
geometry/postgiswriter.o: ../src/geometry/postgiswriter.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/geometry/postgiswriter.cpp -o geometry/postgiswriter.o

pbfformat/objsidset.o: ../src/pbfformat/objsidset.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/objsidset.cpp -o pbfformat/objsidset.o

pbfformat/readfileblocks.o: ../src/pbfformat/readfileblocks.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readfileblocks.cpp -o pbfformat/readfileblocks.o

pbfformat/readfile.o: ../src/pbfformat/readfile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readfile.cpp -o pbfformat/readfile.o

pbfformat/readfileparallel.o: ../src/pbfformat/readfileparallel.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readfileparallel.cpp -o pbfformat/readfileparallel.o
		
pbfformat/writeblock.o: ../src/pbfformat/writeblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/writeblock.cpp -o pbfformat/writeblock.o

pbfformat/fileblock.o: ../src/pbfformat/fileblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/fileblock.cpp -o pbfformat/fileblock.o

pbfformat/readblock.o: ../src/pbfformat/readblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readblock.cpp -o pbfformat/readblock.o

pbfformat/readminimal.o: ../src/pbfformat/readminimal.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readminimal.cpp -o pbfformat/readminimal.o

pbfformat/readblockscaller.o: ../src/pbfformat/readblockscaller.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readblockscaller.cpp -o pbfformat/readblockscaller.o

pbfformat/writepbffile.o: ../src/pbfformat/writepbffile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/writepbffile.cpp -o pbfformat/writepbffile.o


elements/element.o: ../src/elements/element.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/element.cpp -o elements/element.o

elements/baseelement.o: ../src/elements/baseelement.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/baseelement.cpp -o elements/baseelement.o

elements/node.o: ../src/elements/node.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/node.cpp -o elements/node.o
	
elements/way.o: ../src/elements/way.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/way.cpp -o elements/way.o

elements/relation.o: ../src/elements/relation.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/relation.cpp -o elements/relation.o

elements/geometry.o: ../src/elements/geometry.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/geometry.cpp -o elements/geometry.o

elements/combineblocks.o: ../src/elements/combineblocks.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/combineblocks.cpp -o elements/combineblocks.o	


elements/quadtree.o: ../src/elements/quadtree.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/quadtree.cpp -o elements/quadtree.o
	

sorting/common.o: ../src/sorting/common.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/common.cpp -o sorting/common.o

sorting/splitbyid.o: ../src/sorting/splitbyid.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/splitbyid.cpp -o sorting/splitbyid.o

sorting/final.o: ../src/sorting/final.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/final.cpp -o sorting/final.o

sorting/sortblocks.o: ../src/sorting/sortblocks.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/sortblocks.cpp -o sorting/sortblocks.o

sorting/sortgroup.o: ../src/sorting/sortgroup.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/sortgroup.cpp -o sorting/sortgroup.o
	
sorting/resortobjects.o: ../src/sorting/resortobjects.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/resortobjects.cpp -o sorting/resortobjects.o

sorting/mergechanges.o: ../src/sorting/mergechanges.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/mergechanges.cpp -o sorting/mergechanges.o

sorting/tempobjs.o: ../src/sorting/tempobjs.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/tempobjs.cpp -o sorting/tempobjs.o

sorting/qttree.o: ../src/sorting/qttree.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/qttree.cpp -o sorting/qttree.o

sorting/qttreegroups.o: ../src/sorting/qttreegroups.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sorting/qttreegroups.cpp -o sorting/qttreegroups.o

utils/bbox.o: ../src/utils/bbox.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/bbox.cpp -o utils/bbox.o

utils/compress.o: ../src/utils/compress.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/compress.cpp -o utils/compress.o

utils/date.o: ../src/utils/date.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/date.cpp -o utils/date.o

utils/geometry.o: ../src/utils/geometry.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/geometry.cpp -o utils/geometry.o

utils/logger.o: ../src/utils/logger.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/logger.cpp -o utils/logger.o

utils/operatingsystem.o: ../src/utils/operatingsystem.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/operatingsystem.cpp -o utils/operatingsystem.o

utils/timing.o: ../src/utils/timing.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/timing.cpp -o utils/timing.o

utils/pbf/packedint.o: ../src/utils/pbf/packedint.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/packedint.cpp -o utils/pbf/packedint.o

utils/pbf/varint.o: ../src/utils/pbf/varint.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/varint.cpp -o utils/pbf/varint.o
	
utils/pbf/protobuf.o: ../src/utils/pbf/protobuf.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/protobuf.cpp -o utils/pbf/protobuf.o

utils/pbf/fixedint.o: ../src/utils/pbf/fixedint.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/fixedint.cpp -o utils/pbf/fixedint.o

update/applychange.o: ../src/update/applychange.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update/applychange.cpp -o update/applychange.o

update/xmlchange.o: ../src/update/xmlchange.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update/xmlchange.cpp -o update/xmlchange.o
	
update/update.o: ../src/update/update.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update/update.cpp -o update/update.o

calcqts/qtstore.o: ../src/calcqts/qtstore.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/qtstore.cpp -o calcqts/qtstore.o

calcqts/qtstoresplit.o: ../src/calcqts/qtstoresplit.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/qtstoresplit.cpp -o calcqts/qtstoresplit.o

calcqts/calculaterelations.o: ../src/calcqts/calculaterelations.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/calculaterelations.cpp -o calcqts/calculaterelations.o

calcqts/waynodes.o: ../src/calcqts/waynodes.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/waynodes.cpp -o calcqts/waynodes.o
	
calcqts/waynodesfile.o: ../src/calcqts/waynodesfile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/waynodesfile.cpp -o calcqts/waynodesfile.o

calcqts/writewaynodes.o: ../src/calcqts/writewaynodes.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/writewaynodes.cpp -o calcqts/writewaynodes.o

calcqts/writeqts.o: ../src/calcqts/writeqts.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/writeqts.cpp -o calcqts/writeqts.o

calcqts/calcqts.o: ../src/calcqts/calcqts.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/calcqts.cpp -o calcqts/calcqts.o
	
calcqts/wayqts.o: ../src/calcqts/wayqts.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/wayqts.cpp -o calcqts/wayqts.o
	
calcqts/calcqtsinmem.o: ../src/calcqts/calcqtsinmem.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts/calcqtsinmem.cpp -o calcqts/calcqtsinmem.o




clean:
	$(RM) liboqt.so.0.0 oqt *.o update/*.o utils/*.o utils/pbf/*.o sorting/*.o pbfformat/*.o calcqts/*.o elements/*.o geometry/*.o

