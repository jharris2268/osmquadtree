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





#find_package(PQ REQUIRED)
#include_directories(${PQ_INCLUDE_DIRS})

find_package(Threads)


include_directories(thirdparty)
include_directories(thirdparty/xml-inspector)


set (LIBS pq z stdc++fs)

#Generate the shared library from the sources
add_library(oqt_lib SHARED src/CMakeLists.txt)
set_target_properties(oqt_lib PROPERTIES OUTPUT_NAME oqt)


add_subdirectory(src)


target_link_libraries(oqt_lib ${LIBS})

add_executable(oqt "src/oqt_main.cpp")
target_link_libraries(oqt oqt_lib ${LIBS})


#Set the location for library installation -- i.e., /usr/lib in this case
# not really necessary in this example. Use "sudo make install" to apply
install(TARGETS oqt_lib DESTINATION /usr/local/lib)
