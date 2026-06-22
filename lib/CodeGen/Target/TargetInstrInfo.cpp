#include <ll1/CodeGen/Target/TargetInstrInfo.h>

namespace ll1 {

bool TargetInstrInfo::isMoveInstr(const MachineInstr &MI, Register &Src, Register &Dst) const {
    if (MI.getOpcode() == MCOpcode::MOV || MI.getOpcode() == MCOpcode::COPY) {
        if (MI.getNumOperands() >= 2) {
            Dst = MI.getOperand(0).getReg();
            Src = MI.getOperand(1).getReg();
            return true;
        }
    }
    return false;
}

MachineInstr TargetInstrInfo::genCopy(Register DestReg, Register SrcReg) {
    MachineInstr MI(MCOpcode::COPY, IsCopy);
    MI.addReg(DestReg, true);
    MI.addReg(SrcReg);
    return MI;
}

} // namespace ll1
