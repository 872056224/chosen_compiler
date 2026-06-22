#ifndef LL1_CODEGEN_TARGET_TARGETREGISTERINFO_H
#define LL1_CODEGEN_TARGET_TARGETREGISTERINFO_H

#include <ll1/CodeGen/Register.h>
#include <vector>
#include <string>

namespace ll1 {

// ============================================================
// TargetRegisterClass — Description of a register class
// ============================================================
struct TargetRegisterClass {
    std::string Name;
    std::vector<Register> Regs;   // Physical registers in this class
    uint8_t Size;                  // Size in bits (e.g., 16 for GR16)
    uint8_t Alignment;             // Alignment in bytes
    bool Allocatable;              // Can the allocator use these registers?

    bool isAllocatable() const { return Allocatable; }
    unsigned getNumRegs() const { return Regs.size(); }
    Register getRegister(unsigned i) const { return Regs[i]; }
    bool contains(Register Reg) const;
};

// ============================================================
// TargetRegisterInfo — Abstract register file description
// ============================================================
class TargetRegisterInfo {
public:
    virtual ~TargetRegisterInfo() = default;

    virtual unsigned getNumRegs() const = 0;
    virtual const char* getName(Register Reg) const = 0;

    // Return the register class for a given register.
    virtual const TargetRegisterClass* getRegClass(Register Reg) const = 0;

    // All register classes defined for this target.
    virtual const std::vector<TargetRegisterClass>& regClasses() const = 0;

    // Registers available for allocation (in priority order).
    virtual const std::vector<Register>& getAllocatableRegs() const = 0;

    // Return a specific register class by index.
    const TargetRegisterClass* getRegClassById(unsigned Id) const;
};

} // namespace ll1
#endif
