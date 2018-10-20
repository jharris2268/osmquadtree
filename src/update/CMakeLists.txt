set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/applychange.cpp
    ${CMAKE_CURRENT_LIST_DIR}/update.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xmlchange.cpp
    PARENT_SCOPE
    )

#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/applychange.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/update.cpp)
#target_sources(oqt_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/xmlchange.cpp)

