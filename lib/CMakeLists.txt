add_library(ll1IR OBJECT
    IR/Type.cpp
    IR/Value.cpp
    IR/User.cpp
)
target_include_directories(ll1IR PUBLIC ${CMAKE_SOURCE_DIR}/include)
