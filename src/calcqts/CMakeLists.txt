set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/calcqts.cpp
    ${CMAKE_CURRENT_LIST_DIR}/calcqtsinmem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/calculaterelations.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtstore.cpp
    ${CMAKE_CURRENT_LIST_DIR}/qtstoresplit.cpp
    ${CMAKE_CURRENT_LIST_DIR}/waynodes.cpp
    ${CMAKE_CURRENT_LIST_DIR}/waynodesfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/wayqts.cpp
    ${CMAKE_CURRENT_LIST_DIR}/writeqts.cpp
    ${CMAKE_CURRENT_LIST_DIR}/writewaynodes.cpp
    PARENT_SCOPE)

#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/calcqts.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/calcqtsinmem.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/calculaterelations.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/qtstore.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/qtstoresplit.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/waynodes.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/waynodesfile.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/wayqts.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/writeqts.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/writewaynodes.cpp)
