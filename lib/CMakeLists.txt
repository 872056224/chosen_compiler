add_library(ll1IR OBJECT
    IR/Type.cpp
    IR/Value.cpp
    IR/User.cpp
    IR/Instruction.cpp
    IR/BasicBlock.cpp
    IR/Function.cpp
    IR/Module.cpp
    IR/IRBuilder.cpp
    IR/IRPrinter.cpp
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

add_library(ll1IRGen OBJECT
    IRGen/IRGen.cpp
)
target_include_directories(ll1IRGen PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1IRGen PUBLIC ll1IR ll1Parse ll1AST)

# ============================================================
# CodeGen SDK — Machine IR + Target infrastructure (Phase 0+)
# ============================================================
add_library(ll1CodeGenSDK OBJECT
    CodeGen/MachineOperand.cpp
    CodeGen/MachineInstr.cpp
    CodeGen/MachineRegisterInfo.cpp
    CodeGen/MachineFunction.cpp
    CodeGen/MachineFrameInfo.cpp
    CodeGen/Target/TargetRegisterInfo.cpp
    CodeGen/Target/TargetInstrInfo.cpp
    CodeGen/Target/TargetLowering.cpp
    CodeGen/SelectionDAG/SDNode.cpp
    CodeGen/SelectionDAG/SelectionDAG.cpp
    CodeGen/SelectionDAG/SDBuilder.cpp
    CodeGen/Target8086/Target8086RegisterInfo.cpp
    CodeGen/Target8086/Target8086InstrInfo.cpp
    CodeGen/Target8086/Target8086Lowering.cpp
    CodeGen/Target8086/Target8086ISel.cpp
    CodeGen/Target8086/Target8086MCInstPrinter.cpp
    CodeGen/Target8086/Target8086.cpp
    CodeGen/RegAlloc.cpp
)
target_include_directories(ll1CodeGenSDK PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1CodeGenSDK PUBLIC ll1IR)

# Existing CodeGen (backward compat + new pipeline)
add_library(ll1CodeGen OBJECT
    CodeGen/CodeGen.cpp
)
target_include_directories(ll1CodeGen PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1CodeGen PUBLIC ll1CodeGenSDK ll1IR)

add_library(ll1Opt OBJECT
    Opt/PassManager.cpp
    Opt/Passes/Dominators.cpp
    Opt/Passes/Mem2Reg.cpp
    Opt/Passes/PhiElim.cpp
    Opt/Passes/InstCombine.cpp
    Opt/Passes/Reassociate.cpp
    Opt/Passes/GVN.cpp
    Opt/Passes/SimplifyCFG.cpp
    Opt/Passes/DCE.cpp
)
target_include_directories(ll1Opt PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1Opt PUBLIC ll1IR)

add_library(ll1Driver OBJECT
    Driver/Compiler.cpp
)
target_include_directories(ll1Driver PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ll1Driver PUBLIC ll1CodeGen ll1IRGen ll1Opt ll1Sema ll1Parse ll1Lex ll1AST ll1IR)
