#ifndef LL1_CODEGEN_MACHINEREGISTERINFO_H
#define LL1_CODEGEN_MACHINEREGISTERINFO_H

#include <ll1/CodeGen/Register.h>
#include <vector>

namespace ll1 {

class TargetRegisterClass;
class MachineInstr;

// ============================================================
// MachineRegisterInfo — Manages virtual register creation and
// tracks register-to-class mapping.
// ============================================================
class MachineRegisterInfo {
public:
    // Create a new virtual register of the given register class.
    Register createVirtualRegister(const TargetRegisterClass *RC);

    // Get the register class for a register (virtual or physical).
    const TargetRegisterClass* getRegClass(Register Reg) const;

    // Set the register class for a register.
    void setRegClass(Register Reg, const TargetRegisterClass *RC);

    // Number of virtual registers created.
    unsigned getNumVirtRegs() const { return VRegCount; }

    // Get the "def" instruction for a virtual register (set by ISel).
    // Returns the first MachineInstr* that defines this vreg (nullptr if not set).
    MachineInstr* getVRegDef(Register VReg) const;
    void setVRegDef(Register VReg, MachineInstr *MI);

private:
    Register NextVirtualReg = FirstVirtualRegister;
    unsigned VRegCount = 0;

    // Maps virtual register index → register class.
    // Index = virtReg2Index(Register)
    std::vector<const TargetRegisterClass*> VRegClasses;

    // Maps virtual register index → defining MachineInstr.
    std::vector<MachineInstr*> VRegDefs;
};

} // namespace ll1
#endif
