enable_testing()
add_executable(test_type IR/test_type.cpp)
target_link_libraries(test_type PRIVATE ll1IR)
add_test(NAME Type COMMAND test_type)

add_executable(test_value IR/test_value.cpp)
target_link_libraries(test_value PRIVATE ll1IR)
add_test(NAME Value COMMAND test_value)

add_executable(test_instruction IR/test_instruction.cpp)
target_link_libraries(test_instruction PRIVATE ll1IR)
add_test(NAME Instruction COMMAND test_instruction)

add_executable(test_bb IR/test_bb.cpp)
target_link_libraries(test_bb PRIVATE ll1IR)
add_test(NAME BasicBlock COMMAND test_bb)

add_executable(test_function IR/test_function.cpp)
target_link_libraries(test_function PRIVATE ll1IR)
add_test(NAME Function COMMAND test_function)

add_executable(test_irbuilder IR/test_irbuilder.cpp)
target_link_libraries(test_irbuilder PRIVATE ll1IR)
add_test(NAME IRBuilder COMMAND test_irbuilder)

add_executable(test_lexer Lex/test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE ll1Lex)
add_test(NAME Lexer COMMAND test_lexer)

add_executable(test_ast AST/test_ast.cpp)
target_link_libraries(test_ast PRIVATE ll1AST)
add_test(NAME AST COMMAND test_ast)

add_executable(test_parser Parse/test_parser.cpp)
target_link_libraries(test_parser PRIVATE ll1Parse ll1Lex ll1AST)
add_test(NAME Parser COMMAND test_parser)
