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
// 1. Compute dataflow-based liveness (uses/defs per block,
//    LiveIn/LiveOut via iterative backward propagation)
// 2. Build LiveRange per vreg from block-level liveness
// 3. Walk instructions, assign physregs from allocation pool
// 4. On conflict, spill the value with furthest next-use
// 5. Insert spill/reload MachineInstr as needed
// 6. Coalesce COPY instructions when possible
// ============================================================
class LinearScanRegAlloc {
public:
    LinearScanRegAlloc(const TargetRegisterInfo &TRI);

    void run(MachineFunction &MF);

    // Result accessors (for printer)
    const std::unordered_map<Register, Register>& getMapping() const { return V2P; }
    const std::unordered_map<Register, int>& getSpillSlots() const { return SpillSlots; }
    int getSpillSize() const { return MaxSpillOffset - FrameSize; }

private:
    const TargetRegisterInfo &TRI;

    // === Liveness data structures ===

    // Per-block liveness (computed once, used for live ranges)
    struct BlockLiveInfo {
        std::set<Register> Uses;   // vregs used before any def in this block
        std::set<Register> Defs;   // vregs defined in this block
        std::set<Register> LiveIn; // vregs live entering this block
        std::set<Register> LiveOut;// vregs live leaving this block
        unsigned StartPos;          // global position of first instruction
        unsigned EndPos;            // global position after last instruction
        // Per-vreg last-use instruction index within this block (0-based)
        std::unordered_map<Register, unsigned> VRegLastUse;
    };
    std::unordered_map<MachineBasicBlock*, BlockLiveInfo> BlockLive;

    // Live range per virtual register (computed from BlockLive)
    struct LiveRange {
        unsigned FirstDef = 0;  // global position of unique definition
        unsigned LastUse = 0;   // global position of last use
        bool SpilledHere = false; // temporary flag during allocation
    };
    std::unordered_map<Register, LiveRange> LiveRanges;

    // === Allocation state ===

    // Registers currently free
    std::vector<Register> FreeRegs;

    // Currently occupied physical registers
    struct Occupant {
        Register VReg;      // Virtual register occupying this physreg
        unsigned LastUse;   // When this vreg is last used (from LiveRange)
    };
    std::unordered_map<Register, Occupant> Occupied;

    // Virtual → Physical mapping (output)
    std::unordered_map<Register, Register> V2P;

    // Spill slots: vreg → frame index (offset from bx)
    std::unordered_map<Register, int> SpillSlots;
    int NextSpillSlot = 0;
    int FrameSize = 0;
    int MaxSpillOffset = 0;

    // Vregs evicted during current allocation step that need spill stores.
    // Second element is the physreg that held the value at eviction time.
    std::vector<std::pair<Register, Register>> PendingSpillStores;

    // Persistent: evicted vreg → physreg it was evicted from (for block-entry reloads)
    std::unordered_map<Register, Register> EvictedPhysRegs;

    // Current block being processed (for freeDeadRegs liveness check)
    MachineBasicBlock *CurBlock = nullptr;

    // Statistics
    unsigned CopyEliminated = 0;
    unsigned Spills = 0;

    // === Liveness computation ===

    // Phase 1: Compute Uses/Defs per block + assign global positions
    void computeBlockUsesAndDefs(MachineFunction &MF);

    // Phase 2: Solve LiveIn/LiveOut via iterative backward dataflow
    void computeLiveInOut(MachineFunction &MF);

    // Phase 3: Build LiveRange for each vreg from block-level liveness
    void computeLiveRanges(MachineFunction &MF);

    // === Allocation helpers ===

    // Allocate a physical register for a vreg
    Register allocate(Register VReg, unsigned currentPos);

    // Free registers whose occupants are dead (last use < current position)
    void freeDeadRegs(unsigned currentPos, unsigned instrIdx);

    // Spill: evict a physreg's occupant to memory
    void spill(Register PhysReg);

    // Try to coalesce a COPY instruction
    bool tryCoalesceCopy(MachineInstr &MI);

    // Get a spill slot for a vreg
    int getSpillSlot(Register VReg);

    friend class CodeGen;
};

} // namespace ll1
#endif
