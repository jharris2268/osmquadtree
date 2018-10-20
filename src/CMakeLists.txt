

set(LIBRARY_SOURCES ${CMAKE_CURRENT_LIST_DIR}/count.cpp)

add_subdirectory(calcqts)
add_subdirectory(elements)
add_subdirectory(geometry)
add_subdirectory(pbfformat)
add_subdirectory(sorting)
add_subdirectory(utils)
add_subdirectory(update)

add_library(oqt_lib SHARED ${LIBRARY_SOURCES})
set_target_properties(oqt_lib PROPERTIES OUTPUT_NAME oqt)

target_link_libraries(oqt_lib ${LIBS})





install(TARGETS oqt_lib DESTINATION /usr/local/lib)
