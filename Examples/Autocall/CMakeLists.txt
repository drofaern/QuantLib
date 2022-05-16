add_executable(Autocall Autocall.cpp)
target_link_libraries(Autocall ql_library ${QL_EXAMPLES_LIBRARIES})
if (QL_INSTALL_EXAMPLES)
    install(TARGETS Autocall RUNTIME DESTINATION ${QL_INSTALL_EXAMPLESDIR})
endif()


#add_executable(BinaryOption Binaryoption.cpp util.hpp)
#target_link_libraries(BinaryOption ${QL_LINK_LIBRARY})

#add_executable(TouchOption TouchOption.cpp util.hpp)
#target_link_libraries(TouchOption ${QL_LINK_LIBRARY})