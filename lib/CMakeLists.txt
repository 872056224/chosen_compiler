add_library(ll1IR OBJECT
    IR/Type.cpp
)
target_include_directories(ll1IR PUBLIC ${CMAKE_SOURCE_DIR}/include)
