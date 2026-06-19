add_library(ll1IR OBJECT
    IR/Type.cpp
    IR/Value.cpp
    IR/User.cpp
    IR/Instruction.cpp
    IR/BasicBlock.cpp
    IR/Function.cpp
    IR/Module.cpp
)
target_include_directories(ll1IR PUBLIC ${CMAKE_SOURCE_DIR}/include)
