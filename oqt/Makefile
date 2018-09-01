CC = g++
CFLAGS = -pthread -std=c++14 -O3 -Wall -fPIC -I../include/ -I/home/james/build/xml-inspector/ -I../include/oqt/postgresql
LIBS = -lz -lstdc++fs -lpq
#LIBS = -lz
OBJS = calcqts.o combineblocks.o count.o mergechanges.o minimalblock.o packedblock.o primitivegroup.o quadtree.o quadtreegroups.o readblock.o readfile.o simplepbf.o sortblocks.o sortfile.o store.o tempobjs.o update.o utils.o waynodes.o writeblock.o writefile.o writepbffile.o xmlchange.o applychange.o
HEADERS = 
PBFFORMAT_OBJS = pbfformat/objsidset.o
GEOM_OBJS = geometry/addparenttags.o  geometry/addwaynodes.o  geometry/geometrytypes.o  geometry/handlerelations.o  geometry/makegeometries.o  geometry/multipolygons.o  geometry/postgiswriter.o


default: oqt liboqt.so.0.0
oqt: oqt_main.o liboqt.so.0.0
	$(CC) $(CFLAGS) -o oqt oqt_main.o $(LIBS) -L/usr/local/lib -loqt #-ltcmalloc

liboqt.so.0.0: $(OBJS) $(GEOM_OBJS) $(PBFFORMAT_OBJS)
	$(CC) $(CFLAGS) -o liboqt.so.0.0 -shared $(OBJS) $(GEOM_OBJS) $(PBFFORMAT_OBJS) $(LIBS)
 
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

simplepbf.o: ../src/simplepbf.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/simplepbf.cpp

primitivegroup.o: ../src/primitivegroup.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/primitivegroup.cpp

readblock.o: ../src/readblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/readblock.cpp

minimalblock.o: ../src/minimalblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/minimalblock.cpp

quadtree.o: ../src/quadtree.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/quadtree.cpp

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

writeblock.o: ../src/writeblock.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/writeblock.cpp

store.o: ../src/store.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/store.cpp

waynodes.o: ../src/waynodes.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/waynodes.cpp

quadtreegroups.o: ../src/quadtreegroups.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/quadtreegroups.cpp

combineblocks.o: ../src/combineblocks.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/combineblocks.cpp

utils.o: ../src/utils.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/utils.cpp

writefile.o: ../src/writefile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/writefile.cpp

mergechanges.o: ../src/mergechanges.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/mergechanges.cpp

writepbffile.o: ../src/writepbffile.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/writepbffile.cpp
	
tempobjs.o: ../src/tempobjs.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/tempobjs.cpp

oqt_main.o: ../src/oqt_main.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/oqt_main.cpp

applychange.o: ../src/applychange.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/applychange.cpp

xmlchange.o: ../src/xmlchange.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/xmlchange.cpp

update.o: ../src/update.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c ../src/update.cpp

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
	
clean:
	$(RM) oqt *.o liboqt.so* geometry/*.o *~

