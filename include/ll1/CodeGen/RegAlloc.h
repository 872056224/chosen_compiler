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
// 1. Assign global instruction positions per block
// 2. Compute Uses/Defs per block
// 3. Solve LiveIn/LiveOut via backward iterative dataflow on CFG
// 4. Derive live ranges from LiveIn/LiveOut sets
// 5. Walk instructions, assign physregs from allocation pool
// 6. On conflict, spill the value with furthest next-use
// 7. Insert spill/reload MachineInstr as needed
// 8. Coalesce COPY instructions when possible
//
// LLVM reference: LiveVariables.cpp — SSA-based backward propagation,
// walking from uses back to defs via CFG predecessors.
// ============================================================
class LinearScanRegAlloc {
public:
    LinearScanRegAlloc(const TargetRegisterInfo &TRI);

    void run(MachineFunction &MF);

    // Get spill size (additional bytes needed beyond normal frame)
    int getSpillSize() const;

private:
    const TargetRegisterInfo &TRI;
    MachineFunction *MF = nullptr;

    // ============================================================
    // Block-level liveness (dataflow)
    // ============================================================
    struct BlockLiveInfo {
        std::set<Register> Uses;   // vregs read before defined in this block
        std::set<Register> Defs;   // vregs defined in this block
        std::set<Register> LiveIn; // vregs live entering this block
        std::set<Register> LiveOut;// vregs live leaving this block
        unsigned StartPos = 0;     // global position of first instruction
        unsigned EndPos = 0;       // global position after last instruction
    };

    std::unordered_map<MachineBasicBlock*, BlockLiveInfo> BlockInfo;

    // ============================================================
    // Live range per virtual register
    // ============================================================
    struct LiveRange {
        unsigned FirstDef = UINT32_MAX;  // global position of definition
        unsigned LastUse = 0;            // global position of last use
        bool valid() const { return FirstDef != UINT32_MAX; }
    };
    std::unordered_map<Register, LiveRange> LiveRanges;

    // ============================================================
    // Registers currently free
    // ============================================================
    std::vector<Register> FreeRegs;

    // Currently occupied physical registers
    struct Occupant {
        Register VReg;      // Virtual register occupying this physreg
        unsigned LastUse;   // When this vreg is last used
    };
    std::unordered_map<Register, Occupant> Occupied;

    // Virtual → Physical mapping (output)
    std::unordered_map<Register, Register> V2P;

    // Spill slots: vreg → FrameIndex (from CreateStackObject)
    std::unordered_map<Register, int> SpillSlots;

    // Vregs evicted during current allocation step that need spill stores.
    // Second element is the physreg that held the value at eviction time.
    std::vector<std::pair<Register, Register>> PendingSpillStores;

    // Statistics
    unsigned CopyEliminated = 0;
    unsigned Spills = 0;

    // ============================================================
    // Phase 1: Assign global positions and compute Uses/Defs
    // ============================================================
    void assignPositionsAndComputeUsesDefs(MachineFunction &MF);

    // ============================================================
    // Phase 2: Solve LiveIn/LiveOut (backward iterative dataflow)
    // ============================================================
    void solveLiveInOut(MachineFunction &MF);

    // ============================================================
    // Phase 3: Build LiveRange for each vreg
    // ============================================================
    void buildLiveRanges(MachineFunction &MF);

    // ============================================================
    // Allocation
    // ============================================================
    Register allocate(Register VReg, unsigned currentPos,
                      const std::set<Register> *liveOut = nullptr);

    // Free registers whose occupants are dead (last use < current position)
    void freeDeadRegs(unsigned currentPos, const std::set<Register> *liveOut = nullptr);

    // Spill: evict a physreg's occupant to memory
    void spill(Register PhysReg);

    // Reload: load a spilled vreg into a physreg
    Register reload(Register VReg);

    // Try to coalesce a COPY instruction
    bool tryCoalesceCopy(MachineInstr &MI);

    // Get a spill slot FrameIndex for a vreg
    int getSpillSlot(Register VReg);

    // Get the reg alloc mapping (for the printer)
    const std::unordered_map<Register, Register>& getMapping() const { return V2P; }
    const std::unordered_map<Register, int>& getSpillSlots() const { return SpillSlots; }

    friend class CodeGen;
};

} // namespace ll1
#endif
