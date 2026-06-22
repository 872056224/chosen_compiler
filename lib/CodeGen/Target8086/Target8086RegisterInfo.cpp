#include <ll1/CodeGen/Target8086/Target8086RegisterInfo.h>

namespace ll1 {

Target8086RegisterInfo::Target8086RegisterInfo() {
    // GR16: all 16-bit GPRs
    GR16.Name = "GR16";
    GR16.Regs = {X86::AX, X86::CX, X86::DX, X86::BX, X86::SP, X86::BP, X86::SI, X86::DI};
    GR16.Size = 16;
    GR16.Alignment = 2;
    GR16.Allocatable = true;

    // GR16_ABCD: allocatable 16-bit (AX/CX/DX/BX — excludes SP/BP/SI/DI)
    GR16_ABCD.Name = "GR16_ABCD";
    GR16_ABCD.Regs = {X86::AX, X86::CX, X86::DX, X86::BX};
    GR16_ABCD.Size = 16;
    GR16_ABCD.Alignment = 2;
    GR16_ABCD.Allocatable = true;

    // GR8: all 8-bit registers
    GR8.Name = "GR8";
    GR8.Regs = {X86::AH, X86::AL, X86::CH, X86::CL, X86::DH, X86::DL, X86::BH, X86::BL};
    GR8.Size = 8;
    GR8.Alignment = 1;
    GR8.Allocatable = false; // 8-bit allocatable only for byte ops

    Classes = {GR16, GR16_ABCD, GR8};
    AllocOrder = {X86::AX, X86::CX, X86::DX}; // only 3 allocatable registers
}

const char* Target8086RegisterInfo::getName(Register Reg) const {
    switch (Reg) {
    case X86::AX: return "ax"; case X86::CX: return "cx";
    case X86::DX: return "dx"; case X86::BX: return "bx";
    case X86::SP: return "sp"; case X86::BP: return "bp";
    case X86::SI: return "si"; case X86::DI: return "di";
    case X86::AH: return "ah"; case X86::CH: return "ch";
    case X86::DH: return "dh"; case X86::BH: return "bh";
    case X86::AL: return "al"; case X86::CL: return "cl";
    case X86::DL: return "dl"; case X86::BL: return "bl";
    case X86::CS: return "cs"; case X86::DS: return "ds";
    case X86::SS: return "ss"; case X86::ES: return "es";
    default: return "?";
    }
}

const TargetRegisterClass* Target8086RegisterInfo::getRegClass(Register Reg) const {
    if (GR16.contains(Reg)) return &GR16;
    if (GR8.contains(Reg)) return &GR8;
    return nullptr;
}

} // namespace ll1
