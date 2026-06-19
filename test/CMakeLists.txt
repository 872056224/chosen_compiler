enable_testing()
add_executable(test_type IR/test_type.cpp)
target_link_libraries(test_type PRIVATE ll1IR)
add_test(NAME Type COMMAND test_type)

add_executable(test_value IR/test_value.cpp)
target_link_libraries(test_value PRIVATE ll1IR)
add_test(NAME Value COMMAND test_value)
