#include <ll1/CodeGen/Target8086/Target8086InstrInfo.h>

namespace ll1 {

unsigned Target8086InstrInfo::getNumOpcodes() const {
    return static_cast<unsigned>(MCOpcode::NUM_MC_OPCODES);
}

bool Target8086InstrInfo::isMoveInstr(const MachineInstr &MI, Register &Src, Register &Dst) const {
    if (MI.getOpcode() == MCOpcode::MOV || MI.getOpcode() == MCOpcode::COPY) {
        if (MI.getNumOperands() >= 2) {
            Dst = MI.getOperand(0).getReg();
            Src = MI.getOperand(1).getReg();
            return true;
        }
    }
    return false;
}

MachineInstr Target8086InstrInfo::genLoad(Register DestReg, int FrameIdx, uint16_t /*Size*/) {
    MachineInstr MI(MCOpcode::MOV, IsMoveImm);
    MI.addReg(DestReg, true);       // isDef
    MI.addFrameIndex(FrameIdx);      // source: [bx+offset]
    return MI;
}

MachineInstr Target8086InstrInfo::genStore(Register SrcReg, int FrameIdx, uint16_t /*Size*/) {
    MachineInstr MI(MCOpcode::MOV);
    MI.addFrameIndex(FrameIdx);      // dest: [bx+offset]
    MI.addReg(SrcReg);               // source
    return MI;
}

MachineInstr Target8086InstrInfo::genCopy(Register DestReg, Register SrcReg) {
    MachineInstr MI(MCOpcode::COPY, IsCopy);
    MI.addReg(DestReg, true);  // isDef
    MI.addReg(SrcReg);         // isUse
    return MI;
}

MachineInstr Target8086InstrInfo::genSpill(Register PhysReg, int SpillOffset) {
    // MOV [bx+offset], PhysReg
    MachineInstr MI(MCOpcode::MOV);
    // Dest is a memory reference: [bx+offset]
    auto memOp = MachineOperand::CreateFI(SpillOffset);
    MI.addReg(PhysReg);       // src
    // We need to represent the memory dest somehow — use a virtual FI for now
    // The printer will resolve this to [bx+offset]
    return MI;
}

MachineInstr Target8086InstrInfo::genReload(Register PhysReg, int SpillOffset) {
    // MOV PhysReg, [bx+offset]
    MachineInstr MI(MCOpcode::MOV);
    MI.addReg(PhysReg, true);  // dest (def)
    // Source is memory: [bx+offset]
    return MI;
}

} // namespace ll1
