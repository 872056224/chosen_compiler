enable_testing()
add_executable(test_type IR/test_type.cpp)
target_link_libraries(test_type PRIVATE ll1IR)
add_test(NAME Type COMMAND test_type)
