set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/bbox.cpp
    ${CMAKE_CURRENT_LIST_DIR}/compress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/date.cpp
    ${CMAKE_CURRENT_LIST_DIR}/geometry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/operatingsystem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/string.cpp
    ${CMAKE_CURRENT_LIST_DIR}/timing.cpp
    
    ${CMAKE_CURRENT_LIST_DIR}/pbf/fixedint.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pbf/packedint.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pbf/protobuf.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pbf/varint.cpp
    
    PARENT_SCOPE
    )

#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bbox.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/compress.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/date.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/geometry.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/logger.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/operatingsystem.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/string.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/timing.cpp)
#add_subdirectory(pbf)
