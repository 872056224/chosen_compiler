#ifndef LL1_CODEGEN_TARGET_TARGETINSTRINFO_H
#define LL1_CODEGEN_TARGET_TARGETINSTRINFO_H

#include <ll1/CodeGen/MachineInstr.h>

namespace ll1 {

// ============================================================
// TargetInstrInfo — Abstract target instruction information
// ============================================================
class TargetInstrInfo {
public:
    virtual ~TargetInstrInfo() = default;

    virtual unsigned getNumOpcodes() const = 0;
    virtual const char* getName(MCOpcode Opc) const { return getMCOpcodeName(Opc); }

    // Check if MI is a move-like instruction, return src/dst regs.
    virtual bool isMoveInstr(const MachineInstr &MI, Register &Src, Register &Dst) const;

    // Generate a load from a FrameIndex into DestReg.
    virtual MachineInstr genLoad(Register DestReg, int FrameIdx, uint16_t Size) = 0;

    // Generate a store of SrcReg into a FrameIndex.
    virtual MachineInstr genStore(Register SrcReg, int FrameIdx, uint16_t Size) = 0;

    // Generate a COPY between two registers.
    virtual MachineInstr genCopy(Register DestReg, Register SrcReg);

    // Generate a spill: store PhysReg to stack slot [bx+offset].
    virtual MachineInstr genSpill(Register PhysReg, int SpillOffset) = 0;

    // Generate a reload: load from stack slot [bx+offset] into PhysReg.
    virtual MachineInstr genReload(Register PhysReg, int SpillOffset) = 0;
};

} // namespace ll1
#endif
