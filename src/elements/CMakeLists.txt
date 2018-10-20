set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/baseelement.cpp
    ${CMAKE_CURRENT_LIST_DIR}/combineblocks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/element.cpp
    ${CMAKE_CURRENT_LIST_DIR}/geometry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/node.cpp
    ${CMAKE_CURRENT_LIST_DIR}/quadtree.cpp
    ${CMAKE_CURRENT_LIST_DIR}/relation.cpp
    ${CMAKE_CURRENT_LIST_DIR}/way.cpp
    PARENT_SCOPE
    )



#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/baseelement.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/combineblocks.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/element.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/geometry.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/node.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/quadtree.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/relation.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/way.cpp)
