cmake_minimum_required(VERSION 3.6)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(PkgConfig)


project(oqtcpp)
set(CMAKE_BUILD_TYPE Release)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_EXTENSIONS Off)
set (CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")
add_definitions(-Wall)


include_directories(include)

include_directories(thirdparty)
include_directories(thirdparty/xmlinspector)

#set (LIBS pq z stdc++fs)
set (LIBS z stdc++fs)


add_subdirectory(src)
add_subdirectory(example)

install(DIRECTORY include/oqt DESTINATION /usr/local/include) 
