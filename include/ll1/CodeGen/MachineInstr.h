#ifndef LL1_CODEGEN_MACHINEINSTR_H
#define LL1_CODEGEN_MACHINEINSTR_H

#include <ll1/CodeGen/MachineOperand.h>
#include <vector>
#include <string>
#include <cstdint>

namespace ll1 {

// ============================================================
// MCOpcode — Machine instruction opcodes (target-independent + target-specific)
// ============================================================
// Target-independent (generic)
#define TARGET_OPCODE_LIST \
    /* Terminators */ \
    X(RET) X(JMP) X(JE) X(JNE) X(JL) X(JLE) X(JG) X(JGE) \
    /* Data movement */ \
    X(MOV) X(MOVZX) X(MOVSX) \
    /* Arithmetic */ \
    X(ADD) X(SUB) X(MUL) X(DIV) X(IDIV) X(NEG) X(CWD) \
    /* Logic */ \
    X(AND) X(OR) X(XOR) X(NOT) X(SHL) X(SHR) \
    /* Compare */ \
    X(CMP) X(TEST) \
    /* Stack */ \
    X(PUSH) X(POP) \
    /* Subroutine */ \
    X(CALL) \
    /* Phi lowering (virtual-to-virtual copy) */ \
    X(COPY) \
    /* Pseudo */ \
    X(ADJCALLSTACKDOWN) X(ADJCALLSTACKUP) \
    /* Debug / Label */ \
    X(LABEL) X(HLT)

enum class MCOpcode : uint16_t {
#define X(op) op,
    TARGET_OPCODE_LIST
#undef X
    NUM_MC_OPCODES
};

inline const char *getMCOpcodeName(MCOpcode Op) {
    switch (Op) {
#define X(op) case MCOpcode::op: return #op;
    TARGET_OPCODE_LIST
#undef X
    default: return "UNKNOWN";
    }
}

#undef TARGET_OPCODE_LIST

// ============================================================
// MachineInstrFlags
// ============================================================
enum MIFlag : uint32_t {
    NoFlags          = 0,
    IsTerminator     = 1 << 0,
    IsBranch         = 1 << 1,
    IsCall           = 1 << 2,
    IsReturn         = 1 << 3,
    IsMoveImm        = 1 << 4,    // MOV with immediate
    IsCopy           = 1 << 5,    // COPY instruction (phi lowering)
    IsPseudo         = 1 << 6,    // ADJCALLSTACKDOWN/UP, LABEL
    FrameSetup       = 1 << 7,
    FrameDestroy     = 1 << 8,
};

// ============================================================
// MachineInstr — One machine instruction
// ============================================================
class MachineInstr {
public:
    MachineInstr(MCOpcode Opc, uint32_t Flags = NoFlags);

    MCOpcode getOpcode() const { return Opc; }
    uint32_t getFlags() const { return Flags; }
    void setFlags(uint32_t f) { Flags = f; }

    // Operand access
    unsigned getNumOperands() const { return Operands.size(); }
    MachineOperand& getOperand(unsigned i) { return Operands[i]; }
    const MachineOperand& getOperand(unsigned i) const { return Operands[i]; }

    // Add operands (fluent)
    MachineInstr& addReg(Register Reg, bool isDef = false);
    MachineInstr& addImm(int16_t Val);
    MachineInstr& addFrameIndex(int FI);
    MachineInstr& addMBB(MachineBasicBlock *MBB);
    MachineInstr& addExternalSymbol(const std::string &Name);

    // Convenience: add implicit def/use
    MachineInstr& addImplicitDef(Register Reg);
    MachineInstr& addImplicitUse(Register Reg);

    // Parent basic block
    MachineBasicBlock* getParent() const { return Parent; }
    void setParent(MachineBasicBlock *BB) { Parent = BB; }

    // Debug name
    std::string toString() const;

    // Query methods
    bool isTerminator() const { return Flags & IsTerminator; }
    bool isBranch() const { return Flags & IsBranch; }
    bool isCall() const { return Flags & IsCall; }
    bool isReturn() const { return Flags & IsReturn; }
    bool isCopy() const { return Flags & IsCopy; }
    bool isPHI() const { return false; }  // PHI lowered to COPY, kept for compat

private:
    MCOpcode Opc;
    uint32_t Flags;
    std::vector<MachineOperand> Operands;
    MachineBasicBlock *Parent = nullptr;
};

} // namespace ll1
#endif
