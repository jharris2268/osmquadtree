CC = g++
CFLAGS = -pthread -std=c++14 -O3 -Wall -fPIC -I../include/ -I../thirdparty/ -I../thirdparty/xmlinspector/
LIBS = -lz -lstdc++fs -lpq
#LIBS = -lz
ROOT_OBJS = calcqts.o count.o mergechanges.o primitivegroup.o quadtreegroups.o readblock.o readfile.o sortblocks.o sortfile.o store.o tempobjs.o utils.o waynodes.o
HEADERS = 
PBFFORMAT_OBJS = pbfformat/objsidset.o pbfformat/readfileblocks.o pbfformat/writeblock.o pbfformat/fileblock.o pbfformat/readminimal.o pbfformat/writepbffile.o 
ELEMENT_OBJS = elements/combineblocks.o elements/element.o elements/way.o elements/node.o elements/relation.o elements/geometry.o elements/quadtree.o
UTILS_OBJS = utils/bbox.o utils/pbf/packedint.o utils/pbf/protobuf.o utils/pbf/varint.o
UPDATE_OBJS = update/applychange.o update/xmlchange.o update/update.o
GEOM_OBJS = geometry/addparenttags.o  geometry/addwaynodes.o  geometry/geometrytypes.o  geometry/handlerelations.o  geometry/makegeometries.o  geometry/multipolygons.o  geometry/postgiswriter.o
SORTING_OBJS = sorting/common.o sorting/splitbyid.o

OBJS = $(ROOT_OBJS) $(GEOM_OBJS) $(PBFFORMAT_OBJS) $(ELEMENT_OBJS) $(UTILS_OBJS) $(UPDATE_OBJS) $(SORTING_OBJS)

default: oqt liboqt.so.0.0
oqt: oqt_main.o liboqt.so.0.0
	$(CC) $(CFLAGS) -o oqt oqt_main.o $(LIBS) -L/usr/local/lib -loqt #-ltcmalloc

liboqt.so.0.0: $(OBJS)
	$(CC) $(CFLAGS) -o liboqt.so.0.0 -shared $(OBJS) $(LIBS)
 
count.o:  ../src/count.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/count.cpp

countun.o:  countun.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/countun.cpp

countfull.o:  ../src/countfull.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/countfull.cpp

countpacked.o:  ../src/countpacked.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/countpacked.cpp

calcqts.o:  ../src/calcqts.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts.cpp

calcqts_old.o:  ../src/calcqts_old.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/calcqts_old.cpp

readfile.o: ../src/readfile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/readfile.cpp


primitivegroup.o: ../src/primitivegroup.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/primitivegroup.cpp

readblock.o: ../src/readblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/readblock.cpp





packedblock.o: ../src/packedblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/packedblock.cpp

sortblocks.o: ../src/sortblocks.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sortblocks.cpp

sortblocks_old.o: ../src/sortblocks_old.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sortblocks_old.cpp

sortfile.o: ../src/sortfile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sortfile.cpp
sortfile_packed.o: ../src/sortfile_packed.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/sortfile_packed.cpp

store.o: ../src/store.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/store.cpp

waynodes.o: ../src/waynodes.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/waynodes.cpp

quadtreegroups.o: ../src/quadtreegroups.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/quadtreegroups.cpp


utils.o: ../src/utils.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils.cpp

mergechanges.o: ../src/mergechanges.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/mergechanges.cpp
	
tempobjs.o: ../src/tempobjs.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/tempobjs.cpp

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

pbfformat/writeblock.o: ../src/pbfformat/writeblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/writeblock.cpp -o pbfformat/writeblock.o

pbfformat/fileblock.o: ../src/pbfformat/fileblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/fileblock.cpp -o pbfformat/fileblock.o

pbfformat/readminimal.o: ../src/pbfformat/readminimal.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/readminimal.cpp -o pbfformat/readminimal.o

pbfformat/writepbffile.o: ../src/pbfformat/writepbffile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/pbfformat/writepbffile.cpp -o pbfformat/writepbffile.o


elements/element.o: ../src/elements/element.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/elements/element.cpp -o elements/element.o

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


utils/bbox.o: ../src/utils/bbox.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/bbox.cpp -o utils/bbox.o
	
utils/pbf/packedint.o: ../src/utils/pbf/packedint.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/packedint.cpp -o utils/pbf/packedint.o

utils/pbf/varint.o: ../src/utils/pbf/varint.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/varint.cpp -o utils/pbf/varint.o
	
utils/pbf/protobuf.o: ../src/utils/pbf/protobuf.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils/pbf/protobuf.cpp -o utils/pbf/protobuf.o


update/applychange.o: ../src/update/applychange.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update/applychange.cpp -o update/applychange.o

update/xmlchange.o: ../src/update/xmlchange.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update/xmlchange.cpp -o update/xmlchange.o
	
update/update.o: ../src/update/update.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update/update.cpp -o update/update.o

clean:
	$(RM) oqt *.o liboqt.so* geometry/*.o pbfformat/*.o

