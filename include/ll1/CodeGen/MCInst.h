#ifndef LL1_CODEGEN_MCINST_H
#define LL1_CODEGEN_MCINST_H

#include <string>
#include <vector>
#include <cstdint>

namespace ll1 {

// Operand types for machine instructions
struct MCOperand {
    enum Kind { Reg, Imm, Label, Mem };
    Kind K;
    std::string RegName;     // For Reg: "ax", "bx", etc.
    int16_t ImmVal = 0;      // For Imm
    std::string LabelName;   // For Label
    std::string MemBase;     // For Mem: "bp"
    int16_t MemOffset = 0;   // For Mem: offset from base
};

// Machine instruction
struct MCInst {
    std::string Opcode;               // "mov", "add", "sub", "cmp", "jmp", "je", "call", "ret", etc.
    std::vector<MCOperand> Operands;
    std::string Comment;              // Optional comment for readability
};

// Machine basic block
struct MachineBB {
    std::string Label;
    std::vector<MCInst> Instructions;
    std::vector<std::string> TextLines; // Raw assembly text lines from emit()
};

// Machine function
struct MachineFunction {
    std::string Name;
    std::vector<MachineBB> Blocks;
};

// Machine module (output)
struct MachineModule {
    std::vector<MachineFunction> Functions;
    // Global data section (for string literals, etc.)
    std::vector<std::string> DataSection;
};

} // namespace ll1
#endif
