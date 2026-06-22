#ifndef LL1_CODEGEN_REGISTER_H
#define LL1_CODEGEN_REGISTER_H

#include <cstdint>

namespace ll1 {

// ============================================================
// Register — thin wrapper over uint32_t (like LLVM's MCRegister)
//
// Encoding:
//   0                   = NoRegister (sentinel)
//   [1, 32767]          = Physical registers
//   [32768, 0xFFFFFFFF] = Virtual registers
// ============================================================

using Register = uint32_t;

inline constexpr Register NoRegister = 0xFFFFFFFFu;
inline constexpr Register FirstVirtualRegister = 32768u;

inline bool isVirtualRegister(Register Reg) {
    return Reg >= FirstVirtualRegister;
}

inline bool isPhysicalRegister(Register Reg) {
    return Reg > 0 && Reg < FirstVirtualRegister;
}

inline unsigned virtReg2Index(Register Reg) {
    return Reg - FirstVirtualRegister;
}

inline Register index2VirtReg(unsigned Index) {
    return FirstVirtualRegister + Index;
}

inline bool isValidRegister(Register Reg) {
    return Reg != NoRegister;
}

// ============================================================
// Physical register numbers — target-specific.
// These are defined per-target; here are the 8086 numbers.
// ============================================================
namespace X86 {
enum : Register {
    // 16-bit general registers
    AX = 1, CX = 2, DX = 3, BX = 4, SP = 5, BP = 6, SI = 7, DI = 8,

    // 8-bit high sub-registers
    AH = 9, CH = 10, DH = 11, BH = 12,

    // 8-bit low sub-registers
    AL = 13, CL = 14, DL = 15, BL = 16,

    // Segment registers (not allocatable)
    CS = 17, DS = 18, SS = 19, ES = 20,

    NUM_TARGET_REGS = 21
};
} // namespace X86

} // namespace ll1

#endif
