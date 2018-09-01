cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(PkgConfig)


project(oqtcpp)
set(CMAKE_BUILD_TYPE Release)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_EXTENSIONS Off)
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)
add_definitions(-Wall)


#Bring the headers, such as Student.h into the project
include_directories(include)

#However, the file(GLOB...) allows for wildcard additions:
set(SOURCES
    src/applychange.cpp
    src/combineblocks.cpp
    src/mergechanges.cpp
    src/oqt_main.cpp
    src/primitivegroup.cpp
    src/quadtreegroups.cpp
    src/readfile.cpp
    src/sortblocks.cpp
    src/store.cpp
    src/update.cpp
    src/waynodes.cpp
    src/writefile.cpp
    src/xmlchange.cpp
    src/calcqts.cpp
    src/count.cpp
    src/minimalblock.cpp
    src/packedblock.cpp
    src/quadtree.cpp
    src/readblock.cpp
    src/simplepbf.cpp
    src/sortfile.cpp
    src/tempobjs.cpp
    src/utils.cpp
    src/writeblock.cpp
    src/writepbffile.cpp
)

set (GEOMETRY_SOURCES
    src/geometry/addparenttags.cpp
    src/geometry/addwaynodes.cpp
    src/geometry/geometrytypes.cpp
    src/geometry/handlerelations.cpp
    src/geometry/makegeometries.cpp
    src/geometry/multipolygons.cpp
    src/geometry/postgiswriter.cpp
)
set (PBFFORMAT_SOURCES
    src/pbfformat/objsidset.cpp
)

#find_package(PQ REQUIRED)
#include_directories(${PQ_INCLUDE_DIRS})

find_package(Threads)


include_directories(/home/james/build/xml-inspector)


set (LIBS pq z stdc++fs)

#Generate the shared library from the sources
add_library(oqt_lib SHARED ${SOURCES} ${GEOMETRY_SOURCES} ${PBFFORMAT_SOURCES} )
set_target_properties(oqt_lib PROPERTIES OUTPUT_NAME oqt)

target_link_libraries(oqt_lib ${LIBS})

add_executable(oqt "src/oqt_main.cpp")
target_link_libraries(oqt oqt_lib ${LIBS})


#Set the location for library installation -- i.e., /usr/lib in this case
# not really necessary in this example. Use "sudo make install" to apply
install(TARGETS oqt_lib DESTINATION /usr/local/lib)
