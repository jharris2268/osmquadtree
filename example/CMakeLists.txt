add_executable(oqt "oqt_main.cpp")
target_link_libraries(oqt oqt_lib ${LIBS})

install(TARGETS oqt DESTINATION /usr/local/bin)
