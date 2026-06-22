#ifndef LL1_CODEGEN_REGALLOC_H
#define LL1_CODEGEN_REGALLOC_H

#include <ll1/CodeGen/MachineFunction.h>
#include <ll1/CodeGen/MachineInstr.h>
#include <ll1/CodeGen/Target/TargetRegisterInfo.h>
#include <unordered_map>
#include <vector>
#include <set>

namespace ll1 {

// ============================================================
// LinearScanRegAlloc — Linear scan register allocator
//
// 1. Pre-compute live intervals (last-use position per vreg)
// 2. Walk instructions, assign physregs from allocation pool
// 3. On conflict, spill the value with furthest next-use
// 4. Insert spill/reload MachineInstr as needed
// 5. Coalesce COPY instructions when possible
// ============================================================
class LinearScanRegAlloc {
public:
    LinearScanRegAlloc(const TargetRegisterInfo &TRI);

    void run(MachineFunction &MF);

private:
    const TargetRegisterInfo &TRI;

    // Live interval info per virtual register
    struct LiveInfo {
        unsigned LastUse;   // Last instruction position where this vreg is used
        unsigned FirstDef;  // First instruction position where this vreg is defined
    };
    std::unordered_map<Register, LiveInfo> LiveIntervals;

    // Registers currently free
    std::vector<Register> FreeRegs;

    // Currently occupied physical registers
    struct Occupant {
        Register VReg;      // Virtual register occupying this physreg
        unsigned LastUse;   // When this vreg is last used
    };
    std::unordered_map<Register, Occupant> Occupied;

    // Virtual → Physical mapping (output)
    std::unordered_map<Register, Register> V2P;

    // Spill slots: vreg → [bx+offset]
    std::unordered_map<Register, int> SpillSlots;
    int NextSpillSlot = 100;  // Above normal local vars

    // Statistics
    unsigned CopyEliminated = 0;
    unsigned Spills = 0;

    // Pre-scan: compute live intervals
    void computeLiveIntervals(MachineFunction &MF);

    // Allocate a physical register for a vreg
    Register allocate(Register VReg, unsigned currentPos);

    // Free registers whose occupants are dead (last use < current position)
    void freeDeadRegs(unsigned currentPos);

    // Spill: evict a physreg's occupant to memory
    void spill(Register PhysReg);

    // Reload: load a spilled vreg into a physreg
    Register reload(Register VReg);

    // Try to coalesce a COPY instruction
    bool tryCoalesceCopy(MachineInstr &MI);

    // Get a spill slot for a vreg
    int getSpillSlot(Register VReg);

    // Get the reg alloc mapping (for the printer)
    const std::unordered_map<Register, Register>& getMapping() const { return V2P; }
    const std::unordered_map<Register, int>& getSpillSlots() const { return SpillSlots; }

    friend class CodeGen;
};

} // namespace ll1
#endif
