add_executable(Autocall Autocall.cpp)
target_link_libraries(Autocall ${QL_LINK_LIBRARY})

add_executable(BinaryOption Binaryoption.cpp util.hpp)
target_link_libraries(BinaryOption ${QL_LINK_LIBRARY})

add_executable(TouchOption TouchOption.cpp util.hpp)
target_link_libraries(TouchOption ${QL_LINK_LIBRARY})