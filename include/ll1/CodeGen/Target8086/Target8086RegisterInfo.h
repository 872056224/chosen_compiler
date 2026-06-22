#ifndef LL1_CODEGEN_TARGET8086_TARGET8086REGISTERINFO_H
#define LL1_CODEGEN_TARGET8086_TARGET8086REGISTERINFO_H

#include <ll1/CodeGen/Target/TargetRegisterInfo.h>

namespace ll1 {

// ============================================================
// Target8086RegisterInfo — 8086 physical register description
//
// Register classes:
//   GR16      — All 16-bit general purpose registers
//   GR16_ABCD — Allocatable 16-bit (AX, CX, DX, BX)
//   GR8       — All 8-bit registers (high/low sub-registers)
//   SEGMENT   — Segment registers (not allocatable)
// ============================================================
class Target8086RegisterInfo : public TargetRegisterInfo {
public:
    Target8086RegisterInfo();

    unsigned getNumRegs() const override { return X86::NUM_TARGET_REGS; }
    const char* getName(Register Reg) const override;
    const TargetRegisterClass* getRegClass(Register Reg) const override;
    const std::vector<TargetRegisterClass>& regClasses() const override { return Classes; }
    const std::vector<Register>& getAllocatableRegs() const override { return AllocOrder; }

    // Convenience: get register class by member name
    const TargetRegisterClass& getGR16Class() const { return GR16; }
    const TargetRegisterClass& getGR16ABCDClass() const { return GR16_ABCD; }
    const TargetRegisterClass& getGR8Class() const { return GR8; }

private:
    TargetRegisterClass GR16;
    TargetRegisterClass GR16_ABCD;
    TargetRegisterClass GR8;
    std::vector<TargetRegisterClass> Classes;
    std::vector<Register> AllocOrder;
};

} // namespace ll1
#endif
