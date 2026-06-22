#ifndef LL1_CODEGEN_TARGET8086_TARGET8086INSTRINFO_H
#define LL1_CODEGEN_TARGET8086_TARGET8086INSTRINFO_H

#include <ll1/CodeGen/Target/TargetInstrInfo.h>

namespace ll1 {

// ============================================================
// Target8086InstrInfo — 8086-specific instruction information
// ============================================================
class Target8086InstrInfo : public TargetInstrInfo {
public:
    unsigned getNumOpcodes() const override;

    bool isMoveInstr(const MachineInstr &MI, Register &Src, Register &Dst) const override;

    MachineInstr genLoad(Register DestReg, int FrameIdx, uint16_t Size) override;
    MachineInstr genStore(Register SrcReg, int FrameIdx, uint16_t Size) override;
    MachineInstr genCopy(Register DestReg, Register SrcReg) override;
    MachineInstr genSpill(Register PhysReg, int SpillOffset) override;
    MachineInstr genReload(Register PhysReg, int SpillOffset) override;
};

} // namespace ll1
#endif
