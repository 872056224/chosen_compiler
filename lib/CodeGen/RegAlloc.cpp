#include <ll1/CodeGen/RegAlloc.h>

namespace ll1 {

LinearScanRegAlloc::LinearScanRegAlloc(const TargetRegisterInfo &tri) : TRI(tri) {
    for (auto reg : TRI.getAllocatableRegs()) {
        FreeRegs.push_back(reg);
    }
}

// ============================================================
// Phase 1: Compute Uses/Defs per block + assign global positions
// ============================================================
void LinearScanRegAlloc::computeBlockUsesAndDefs(MachineFunction &MF) {
    BlockLive.clear();
    unsigned pos = 0;

    for (auto &mbb : MF.getBasicBlocks()) {
        MachineBasicBlock *B = mbb.get();
        auto &info = BlockLive[B];

        info.StartPos = pos;
        std::set<Register> &localDefs = info.Defs;
        unsigned instrIdx = 0;

        for (auto &mi : B->getInstList()) {
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;

                // Track last use position within this block
                if (!mo.isDef()) {
                    info.VRegLastUse[vreg] = instrIdx;
                    if (localDefs.find(vreg) == localDefs.end()) {
                        info.Uses.insert(vreg);
                    }
                } else {
                    localDefs.insert(vreg);
                }
            }
            pos++;
            instrIdx++;
        }
        info.EndPos = pos;
    }
}

// ============================================================
// Phase 2: Solve LiveIn/LiveOut via iterative backward dataflow
// ============================================================
void LinearScanRegAlloc::computeLiveInOut(MachineFunction &MF) {
    // Initialize: LiveIn = Uses, LiveOut = {}
    for (auto &mbb : MF.getBasicBlocks()) {
        auto *B = mbb.get();
        BlockLive[B].LiveIn = BlockLive[B].Uses;
        BlockLive[B].LiveOut.clear();
    }

    // Collect blocks in reverse order for backward iteration
    std::vector<MachineBasicBlock*> reverseOrder;
    auto &blocks = MF.getBasicBlocks();
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        reverseOrder.push_back(it->get());
    }

    // Iterate to fixpoint
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *B : reverseOrder) {
            auto &info = BlockLive[B];

            // LiveOut[B] = union of LiveIn[successors]
            std::set<Register> newLiveOut;
            for (auto *succ : B->getSuccessors()) {
                auto succIt = BlockLive.find(succ);
                if (succIt == BlockLive.end()) continue;
                for (Register r : succIt->second.LiveIn) {
                    newLiveOut.insert(r);
                }
            }

            // LiveIn[B] = Uses[B] ∪ (LiveOut[B] − Defs[B])
            std::set<Register> newLiveIn = info.Uses;
            for (Register r : newLiveOut) {
                if (info.Defs.find(r) == info.Defs.end()) {
                    newLiveIn.insert(r);
                }
            }

            if (newLiveIn != info.LiveIn || newLiveOut != info.LiveOut) {
                changed = true;
                info.LiveIn = std::move(newLiveIn);
                info.LiveOut = std::move(newLiveOut);
            }
        }
    }
}

// ============================================================
// Phase 3: Build LiveRange for each vreg from block liveness
// ============================================================
void LinearScanRegAlloc::computeLiveRanges(MachineFunction &MF) {
    LiveRanges.clear();

    // First pass: record FirstDef for every vreg
    unsigned pos = 0;
    for (auto &mbb : MF.getBasicBlocks()) {
        for (auto &mi : mbb->getInstList()) {
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;

                auto &lr = LiveRanges[vreg];
                if (mo.isDef()) lr.FirstDef = pos;
                lr.LastUse = pos;
            }
            pos++;
        }
    }

    // Second pass: extend liveness using LiveIn/LiveOut sets
    for (auto &kv : BlockLive) {
        MachineBasicBlock *B = kv.first;
        auto &info = kv.second;

        // For each vreg live-in to this block, extend its range to block start
        for (Register vreg : info.LiveIn) {
            auto lrIt = LiveRanges.find(vreg);
            if (lrIt == LiveRanges.end()) continue;
            // The vreg is needed at block entry → range must cover block start
            lrIt->second.LastUse = std::max(lrIt->second.LastUse, info.StartPos);
        }

        // For each vreg live-out, extend its range to block end
        for (Register vreg : info.LiveOut) {
            auto lrIt = LiveRanges.find(vreg);
            if (lrIt == LiveRanges.end()) continue;
            // The vreg is needed after block exit → range must cover block end
            lrIt->second.LastUse = std::max(lrIt->second.LastUse, info.EndPos);
        }
    }
}

// ============================================================
// getSpillSlot — allocate a spill slot for a vreg
// ============================================================
int LinearScanRegAlloc::getSpillSlot(Register VReg) {
    auto it = SpillSlots.find(VReg);
    if (it != SpillSlots.end()) return it->second;
    int off = NextSpillSlot;
    NextSpillSlot += 2;
    MaxSpillOffset = std::max(MaxSpillOffset, off);
    SpillSlots[VReg] = off;
    return off;
}

// ============================================================
// allocate — get a physreg for a vreg (evict if needed)
// ============================================================
Register LinearScanRegAlloc::allocate(Register VReg, unsigned currentPos) {
    auto it = V2P.find(VReg);
    if (it != V2P.end()) return it->second;

    auto lrIt = LiveRanges.find(VReg);
    unsigned lastUse = lrIt != LiveRanges.end() ? lrIt->second.LastUse : currentPos;

    if (!FreeRegs.empty()) {
        Register phys = FreeRegs.back();
        FreeRegs.pop_back();
        V2P[VReg] = phys;
        Occupied[phys] = {VReg, lastUse};
        return phys;
    }

    // Evict the register with furthest last use
    Register evictPhys = Occupied.begin()->first;
    unsigned furthestUse = 0;
    for (auto &kv : Occupied) {
        if (kv.second.LastUse > furthestUse) {
            furthestUse = kv.second.LastUse;
            evictPhys = kv.first;
        }
    }

    spill(evictPhys);

    // Now allocate the freed register
    V2P[VReg] = evictPhys;
    Occupied[evictPhys] = {VReg, lastUse};
    return evictPhys;
}

// ============================================================
// freeDeadRegs
// ============================================================
void LinearScanRegAlloc::freeDeadRegs(unsigned currentPos, unsigned instrIdx) {
    std::vector<Register> toFree;
    auto &blockInfo = BlockLive[CurBlock];
    for (auto &kv : Occupied) {
        Register vreg = kv.second.VReg;
        bool hasFutureUse = false;
        // Check VRegLastUse: does this vreg have uses later in this block?
        auto lastUseIt = blockInfo.VRegLastUse.find(vreg);
        if (lastUseIt != blockInfo.VRegLastUse.end() && lastUseIt->second >= instrIdx) {
            hasFutureUse = true;
        }
        // Check LiveOut: does this vreg flow to successors?
        if (blockInfo.LiveOut.find(vreg) != blockInfo.LiveOut.end()) {
            hasFutureUse = true;
        }
        // Also check global position-based liveness
        auto lrIt = LiveRanges.find(vreg);
        if (!hasFutureUse && lrIt != LiveRanges.end() && lrIt->second.LastUse >= currentPos) {
            hasFutureUse = true;
        }
        if (!hasFutureUse) {
            toFree.push_back(kv.first);
        }
    }
    for (auto reg : toFree) {
        Occupied.erase(reg);
        FreeRegs.push_back(reg);
    }
}

// ============================================================
// spill
// ============================================================
void LinearScanRegAlloc::spill(Register PhysReg) {
    auto it = Occupied.find(PhysReg);
    if (it == Occupied.end()) return;

    Register vreg = it->second.VReg;
    getSpillSlot(vreg);
    PendingSpillStores.push_back({vreg, PhysReg});
    EvictedPhysRegs[vreg] = PhysReg;  // remember which physreg was evicted
    V2P.erase(vreg);
    Occupied.erase(it);
    FreeRegs.push_back(PhysReg);
    Spills++;
}

// ============================================================
// tryCoalesceCopy
// ============================================================
bool LinearScanRegAlloc::tryCoalesceCopy(MachineInstr &MI) {
    if (MI.getOpcode() != MCOpcode::COPY) return false;
    if (MI.getNumOperands() < 2) return false;
    Register dst = MI.getOperand(0).getReg();
    Register src = MI.getOperand(1).getReg();
    auto dstIt = V2P.find(dst);
    auto srcIt = V2P.find(src);
    if (dstIt != V2P.end() && srcIt != V2P.end() && dstIt->second == srcIt->second) {
        CopyEliminated++;
        return true;
    }
    return false;
}

// ============================================================
// run — main entry point
// ============================================================
void LinearScanRegAlloc::run(MachineFunction &MF) {
    // Phase 1-3: Dataflow liveness
    computeBlockUsesAndDefs(MF);
    computeLiveInOut(MF);
    computeLiveRanges(MF);

    // Phase 4: Linear scan allocation
    FreeRegs.clear();
    for (auto reg : TRI.getAllocatableRegs()) FreeRegs.push_back(reg);
    Occupied.clear();
    V2P.clear();
    SpillSlots.clear();
    PendingSpillStores.clear();
    EvictedPhysRegs.clear();
    FrameSize = MF.getFrameInfo().getStackSize();
    NextSpillSlot = FrameSize;
    MaxSpillOffset = FrameSize;
    CopyEliminated = 0;
    Spills = 0;

    unsigned pos = 0;
    for (auto &mbb : MF.getBasicBlocks()) {
        CurBlock = mbb.get();
        auto &insts = mbb->getInstList();
        unsigned instrIdx = 0;
        for (auto it = insts.begin(); it != insts.end(); ) {
            MachineInstr &mi = *it;

            // Update LiveRanges for all vregs in this instruction
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() == MachineOperandType::MO_Register) {
                    Register vreg = mo.getReg();
                    if (isVirtualRegister(vreg)) {
                        auto lrIt = LiveRanges.find(vreg);
                        if (lrIt != LiveRanges.end()) {
                            lrIt->second.LastUse = pos;
                        }
                    }
                }
            }

            freeDeadRegs(pos, instrIdx);

            // COPY coalescing
            if (mi.isCopy() && tryCoalesceCopy(mi)) {
                it = insts.erase(it);
                pos++;
                continue;
            }

            // Reload spilled USE operands BEFORE this instruction
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                if (mo.isDef()) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;

                auto spillIt = SpillSlots.find(vreg);
                if (spillIt != SpillSlots.end()) {
                    Register phys = allocate(vreg, pos);
                    MachineInstr reloadMI(MCOpcode::MOV);
                    reloadMI.addReg(phys, true);
                    reloadMI.addFrameIndex(spillIt->second);
                    it = insts.insert(it, std::move(reloadMI));
                    ++it;
                    pos++;
                }
            }

            // Allocate registers for ALL vreg operands
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;
                allocate(vreg, pos);
            }

            // Emit spill stores BEFORE this instr (register about to be clobbered)
            while (!PendingSpillStores.empty()) {
                auto pending = PendingSpillStores.back();
                PendingSpillStores.pop_back();
                Register vreg = pending.first;
                Register phys = pending.second;
                auto spillIt = SpillSlots.find(vreg);
                if (spillIt == SpillSlots.end()) continue;
                MachineInstr storeMI(MCOpcode::MOV);
                storeMI.addFrameIndex(spillIt->second);
                storeMI.addReg(phys);
                it = insts.insert(it, std::move(storeMI));
                ++it;
                pos++;
            }

            // Rewrite virtual registers → physical
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() == MachineOperandType::MO_Register) {
                    Register vreg = mo.getReg();
                    if (isVirtualRegister(vreg)) {
                        auto physIt = V2P.find(vreg);
                        if (physIt != V2P.end()) {
                            mo.setReg(physIt->second);
                        }
                    }
                }
            }

            ++it;
            pos++;
            instrIdx++;
        }
    }

    // Post-pass: insert reloads at block entries for LiveIn vregs that were evicted.
    // Reload into the SAME physreg that was evicted, so uses already rewritten
    // to that physreg will pick up the correct value on loop back-edge.
    for (auto &mbb : MF.getBasicBlocks()) {
        auto *B = mbb.get();
        auto liveIt = BlockLive.find(B);
        if (liveIt == BlockLive.end()) continue;

        auto instIt = B->getInstList().begin();
        for (Register vreg : liveIt->second.LiveIn) {
            auto spillIt = SpillSlots.find(vreg);
            if (spillIt == SpillSlots.end()) continue;
            auto evictIt = EvictedPhysRegs.find(vreg);
            if (evictIt == EvictedPhysRegs.end()) continue;
            Register origPhys = evictIt->second;
            // Emit: MOV origPhys, [spillSlot]
            MachineInstr reloadMI(MCOpcode::MOV);
            reloadMI.addReg(origPhys, true);
            reloadMI.addFrameIndex(spillIt->second);
            instIt = B->getInstList().insert(instIt, std::move(reloadMI));
            ++instIt;
        }
    }
}

} // namespace ll1
