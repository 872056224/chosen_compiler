add_executable(ll1c main.cpp)
target_link_libraries(ll1c PRIVATE ll1Driver ll1Opt ll1CodeGen ll1CodeGenSDK ll1IRGen ll1Sema ll1Parse ll1Lex ll1AST ll1IR)
