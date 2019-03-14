set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/fileblock.cpp
    ${CMAKE_CURRENT_LIST_DIR}/objsidset.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readblock.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readblocknew.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readblockscaller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readfileblocks.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readfileparallel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/readminimal.cpp
    ${CMAKE_CURRENT_LIST_DIR}/writeblock.cpp
    ${CMAKE_CURRENT_LIST_DIR}/writepbffile.cpp
    PARENT_SCOPE
    )


#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/fileblock.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/objsidset.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/readblock.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/readblockscaller.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/readfile.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/readfileblocks.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/readfileparallel.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/readminimal.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/writeblock.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/writepbffile.cpp)
