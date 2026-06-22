#ifndef LL1_CODEGEN_MACHINEOPERAND_H
#define LL1_CODEGEN_MACHINEOPERAND_H

#include <ll1/CodeGen/Register.h>
#include <string>
#include <cstdint>

namespace ll1 {

class MachineBasicBlock;

// Operand type tag
enum class MachineOperandType : uint8_t {
    MO_Register,           // Register (virtual or physical)
    MO_Immediate,          // int16_t immediate value
    MO_FrameIndex,         // Stack frame index (int)
    MO_MachineBasicBlock,  // MachineBasicBlock* (for branches)
    MO_ExternalSymbol,     // std::string (for global names / call targets)
};

class MachineOperand {
public:
    MachineOperandType getType() const { return Kind; }

    // Constructors
    static MachineOperand CreateReg(Register Reg, bool isDef = false,
                                     bool isDead = false, bool isKill = false);
    static MachineOperand CreateImm(int16_t Val);
    static MachineOperand CreateFI(int Index);
    static MachineOperand CreateMBB(MachineBasicBlock *MBB);
    static MachineOperand CreateES(const std::string &Name);

    // Accessors
    Register getReg() const;
    int16_t getImm() const;
    int getFrameIndex() const;
    MachineBasicBlock* getMBB() const;
    const std::string& getSymbolName() const;

    // Register operand flags
    bool isDef()  const;
    bool isDead() const;
    bool isKill() const;
    void setIsDef(bool v);
    void setIsDead(bool v);
    void setIsKill(bool v);

    void setReg(Register Reg);

private:
    MachineOperandType Kind;

    // Register flags (packed into RegFlags byte)
    uint8_t RegFlags = 0;   // bit0=isDef, bit1=isDead, bit2=isKill

    union {
        Register Reg;
        int16_t ImmVal;
        int FrameIdx;
        MachineBasicBlock *MBB;
    };
    // For MO_ExternalSymbol, the name is stored separately
    std::string SymbolName;
};

} // namespace ll1
#endif
