add_library(ll1IR OBJECT
    IR/Type.cpp
    IR/Value.cpp
    IR/User.cpp
    IR/Instruction.cpp
    IR/BasicBlock.cpp
    IR/Function.cpp
    IR/Module.cpp
    IR/IRBuilder.cpp
)
target_include_directories(ll1IR PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(ll1Lex OBJECT
    Lex/Lexer.cpp
)
target_include_directories(ll1Lex PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(ll1AST OBJECT
    AST/AST.cpp
)
target_include_directories(ll1AST PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(ll1Parse OBJECT
    Parse/Parser.cpp
)
target_include_directories(ll1Parse PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1Parse PUBLIC ll1Lex ll1AST)

add_library(ll1Sema OBJECT
    Sema/Scope.cpp
    Sema/Sema.cpp
)
target_include_directories(ll1Sema PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1Sema PUBLIC ll1Parse ll1AST ll1Lex)
