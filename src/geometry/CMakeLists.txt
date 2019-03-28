set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/addparenttags.cpp
    ${CMAKE_CURRENT_LIST_DIR}/addwaynodes.cpp
    ${CMAKE_CURRENT_LIST_DIR}/findminzoom.cpp
    ${CMAKE_CURRENT_LIST_DIR}/handlerelations.cpp
    ${CMAKE_CURRENT_LIST_DIR}/makegeometries.cpp
    ${CMAKE_CURRENT_LIST_DIR}/multipolygons.cpp    
    ${CMAKE_CURRENT_LIST_DIR}/postgiswriter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/process.cpp
    ${CMAKE_CURRENT_LIST_DIR}/processpostgis.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utils.cpp
    
    ${CMAKE_CURRENT_LIST_DIR}/elements/complicatedpolygon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/elements/linestring.cpp
    ${CMAKE_CURRENT_LIST_DIR}/elements/point.cpp
    ${CMAKE_CURRENT_LIST_DIR}/elements/ring.cpp
    ${CMAKE_CURRENT_LIST_DIR}/elements/simplepolygon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/elements/waywithnodes.cpp
    
    
    PARENT_SCOPE
    )

#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/addparenttags.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/addwaynodes.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/findminzoom.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/handlerelations.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/makegeometries.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/multipolygons.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/postgiswriter.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/process.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/utils.cpp)
#add_subdirectory(elements)
