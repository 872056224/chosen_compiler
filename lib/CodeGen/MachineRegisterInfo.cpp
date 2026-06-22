#include <ll1/CodeGen/MachineRegisterInfo.h>
#include <ll1/CodeGen/Target/TargetRegisterInfo.h>
#include <cassert>

namespace ll1 {

Register MachineRegisterInfo::createVirtualRegister(const TargetRegisterClass *RC) {
    Register VReg = NextVirtualReg++;
    VRegCount++;
    unsigned Idx = virtReg2Index(VReg);
    if (Idx >= VRegClasses.size()) {
        VRegClasses.resize(Idx + 1, nullptr);
        VRegDefs.resize(Idx + 1, nullptr);
    }
    VRegClasses[Idx] = RC;
    return VReg;
}

const TargetRegisterClass* MachineRegisterInfo::getRegClass(Register Reg) const {
    if (isPhysicalRegister(Reg)) {
        // For physical regs, we'd need TRI to look up. But this should
        // be handled by the caller via TargetRegisterInfo directly.
        return nullptr;
    }
    unsigned Idx = virtReg2Index(Reg);
    if (Idx >= VRegClasses.size()) return nullptr;
    return VRegClasses[Idx];
}

void MachineRegisterInfo::setRegClass(Register Reg, const TargetRegisterClass *RC) {
    assert(isVirtualRegister(Reg));
    unsigned Idx = virtReg2Index(Reg);
    if (Idx >= VRegClasses.size()) {
        VRegClasses.resize(Idx + 1, nullptr);
        VRegDefs.resize(Idx + 1, nullptr);
    }
    VRegClasses[Idx] = RC;
}

MachineInstr* MachineRegisterInfo::getVRegDef(Register VReg) const {
    assert(isVirtualRegister(VReg));
    unsigned Idx = virtReg2Index(VReg);
    if (Idx >= VRegDefs.size()) return nullptr;
    return VRegDefs[Idx];
}

void MachineRegisterInfo::setVRegDef(Register VReg, MachineInstr *MI) {
    assert(isVirtualRegister(VReg));
    unsigned Idx = virtReg2Index(VReg);
    if (Idx >= VRegDefs.size()) {
        VRegDefs.resize(Idx + 1, nullptr);
    }
    VRegDefs[Idx] = MI;
}

} // namespace ll1
