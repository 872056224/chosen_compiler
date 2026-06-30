#include <ll1/CodeGen/RegAlloc.h>
#include <algorithm>
#include <iostream>

namespace ll1 {

LinearScanRegAlloc::LinearScanRegAlloc(const TargetRegisterInfo &tri) : TRI(tri) {
    for (auto reg : TRI.getAllocatableRegs()) {
        FreeRegs.push_back(reg);
    }
}

// ============================================================
// Phase 1: Assign global instruction positions and compute
//          Uses/Defs for each block
// ============================================================
void LinearScanRegAlloc::assignPositionsAndComputeUsesDefs(MachineFunction &MF) {
    BlockInfo.clear();
    unsigned pos = 0;

    for (auto &mbb : MF.getBasicBlocks()) {
        BlockLiveInfo &info = BlockInfo[mbb.get()];
        info.StartPos = pos;

        std::set<Register> &Uses = info.Uses;
        std::set<Register> &Defs = info.Defs;

        for (auto &mi : mbb->getInstList()) {
            std::set<Register> instDefs;
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register reg = mo.getReg();
                if (!isVirtualRegister(reg)) continue;
                if (mo.isDef()) {
                    instDefs.insert(reg);
                    Defs.insert(reg);
                }
            }

            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register reg = mo.getReg();
                if (!isVirtualRegister(reg)) continue;
                if (!mo.isDef()) {
                    if (Defs.find(reg) == Defs.end()) {
                        Uses.insert(reg);
                    }
                }
            }

            pos++;
        }

        info.EndPos = pos;
    }
}

// ============================================================
// Phase 2: Solve LiveIn/LiveOut via backward iterative dataflow
// ============================================================
void LinearScanRegAlloc::solveLiveInOut(MachineFunction &MF) {
    for (auto &mbb : MF.getBasicBlocks()) {
        BlockLiveInfo &info = BlockInfo[mbb.get()];
        info.LiveIn = info.Uses;
        info.LiveOut.clear();
    }

    bool changed = true;
    int iterations = 0;
    while (changed) {
        changed = false;
        iterations++;
        if (iterations > 100) {
            std::cerr << "[regalloc] WARNING: LiveIn/LiveOut not converging\n";
            break;
        }

        auto &blocks = MF.getBasicBlocks();
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            MachineBasicBlock *mbb = it->get();
            BlockLiveInfo &info = BlockInfo[mbb];

            std::set<Register> newLiveOut;
            for (auto *succ : mbb->getSuccessors()) {
                auto succIt = BlockInfo.find(succ);
                if (succIt != BlockInfo.end()) {
                    for (Register r : succIt->second.LiveIn) {
                        newLiveOut.insert(r);
                    }
                }
            }

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

    std::cerr << "[regalloc] LiveIn/LiveOut converged in " << iterations << " iterations\n";
}

// ============================================================
// Phase 3: Build LiveRange for each vreg
// ============================================================
void LinearScanRegAlloc::buildLiveRanges(MachineFunction &MF) {
    LiveRanges.clear();

    std::unordered_map<Register, unsigned> defPos;
    std::unordered_map<Register, MachineBasicBlock*> defBlock;
    std::unordered_map<Register, std::unordered_map<MachineBasicBlock*, unsigned>> blockLastUse;

    for (auto &mbb : MF.getBasicBlocks()) {
        unsigned blockStart = BlockInfo[mbb.get()].StartPos;
        unsigned pos = blockStart;

        for (auto &mi : mbb->getInstList()) {
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register reg = mo.getReg();
                if (!isVirtualRegister(reg)) continue;

                if (mo.isDef()) {
                    if (defPos.find(reg) == defPos.end()) {
                        defPos[reg] = pos;
                        defBlock[reg] = mbb.get();
                    }
                } else {
                    blockLastUse[reg][mbb.get()] = pos;
                }
            }
            pos++;
        }
    }

    for (auto &kv : defPos) {
        Register reg = kv.first;
        LiveRange &lr = LiveRanges[reg];
        lr.FirstDef = kv.second;
        lr.LastUse = kv.second;

        MachineBasicBlock *defB = defBlock[reg];

        for (auto &bi : BlockInfo) {
            MachineBasicBlock *mbb = bi.first;
            const BlockLiveInfo &info = bi.second;

            bool liveInBlock = false;
            if (info.LiveIn.find(reg) != info.LiveIn.end()) liveInBlock = true;
            if (mbb == defB) liveInBlock = true;
            if (info.LiveOut.find(reg) != info.LiveOut.end()) liveInBlock = true;

            if (liveInBlock) {
                auto useIt = blockLastUse.find(reg);
                if (useIt != blockLastUse.end()) {
                    auto blockUseIt = useIt->second.find(mbb);
                    if (blockUseIt != useIt->second.end()) {
                        lr.LastUse = std::max(lr.LastUse, blockUseIt->second);
                    }
                }
                // Extend to end of block if live-out (forces register preservation)
                if (info.LiveOut.find(reg) != info.LiveOut.end()) {
                    lr.LastUse = std::max(lr.LastUse, info.EndPos - 1);
                }
            }
        }
    }
}

// ============================================================
// Spill slot management — uses MachineFrameInfo::CreateStackObject
// ============================================================
int LinearScanRegAlloc::getSpillSlot(Register VReg) {
    auto it = SpillSlots.find(VReg);
    if (it != SpillSlots.end()) return it->second;

    if (!MF) return -1;

    int fi = MF->getFrameInfo().CreateStackObject(2, 2, false, 0);
    SpillSlots[VReg] = fi;
    return fi;
}

int LinearScanRegAlloc::getSpillSize() const {
    return 0;
}

// ============================================================
// Allocate — prefer evicting non-LiveOut registers
// ============================================================
Register LinearScanRegAlloc::allocate(Register VReg, unsigned currentPos,
                                       const std::set<Register> *liveOut) {
    auto it = V2P.find(VReg);
    if (it != V2P.end()) return it->second;

    unsigned lastUse = UINT32_MAX;
    auto lrIt = LiveRanges.find(VReg);
    if (lrIt != LiveRanges.end() && lrIt->second.valid()) {
        lastUse = lrIt->second.LastUse;
    }

    if (!FreeRegs.empty()) {
        Register phys = FreeRegs.back();
        FreeRegs.pop_back();
        V2P[VReg] = phys;
        Occupied[phys] = {VReg, lastUse};
        return phys;
    }

    // All registers occupied — evict one
    // First choice: a register whose vreg is NOT LiveOut (won't need reload soon)
    // Second choice: the register with furthest last use
    Register evictPhys = NoRegister;
    Register evictNonLiveOut = NoRegister;
    unsigned furthestUse = 0;
    unsigned furthestNonLiveOutUse = 0;

    for (auto &kv : Occupied) {
        bool nonLiveOut = liveOut && liveOut->find(kv.second.VReg) == liveOut->end();
        if (nonLiveOut && kv.second.LastUse > furthestNonLiveOutUse) {
            furthestNonLiveOutUse = kv.second.LastUse;
            evictNonLiveOut = kv.first;
        }
        if (kv.second.LastUse > furthestUse) {
            furthestUse = kv.second.LastUse;
            evictPhys = kv.first;
        }
    }

    // Prefer non-LiveOut registers
    if (evictNonLiveOut != NoRegister) {
        evictPhys = evictNonLiveOut;
    }

    if (evictPhys == NoRegister) {
        if (!Occupied.empty()) {
            evictPhys = Occupied.begin()->first;
        } else {
            evictPhys = X86::AX;
        }
    }

    spill(evictPhys);
    // spill() adds evictPhys to FreeRegs, but we're immediately reusing it.
    // Remove it from FreeRegs to prevent double-allocation.
    if (!FreeRegs.empty() && FreeRegs.back() == evictPhys) {
        FreeRegs.pop_back();
    }
    V2P[VReg] = evictPhys;
    Occupied[evictPhys] = {VReg, lastUse};
    return evictPhys;
}

void LinearScanRegAlloc::freeDeadRegs(unsigned currentPos,
                                       const std::set<Register> *liveOut) {
    std::vector<Register> toFree;
    for (auto &kv : Occupied) {
        // Never free a register whose vreg is LiveOut of this block
        if (liveOut && liveOut->find(kv.second.VReg) != liveOut->end()) continue;
        if (kv.second.LastUse != UINT32_MAX && kv.second.LastUse < currentPos) {
            toFree.push_back(kv.first);
        }
    }
    for (auto reg : toFree) {
        Occupied.erase(reg);
        FreeRegs.push_back(reg);
    }
}

void LinearScanRegAlloc::spill(Register PhysReg) {
    auto it = Occupied.find(PhysReg);
    if (it == Occupied.end()) return;

    Register vreg = it->second.VReg;
    getSpillSlot(vreg);
    PendingSpillStores.push_back({vreg, PhysReg});
    V2P.erase(vreg);
    Occupied.erase(it);
    FreeRegs.push_back(PhysReg);
    Spills++;
}

Register LinearScanRegAlloc::reload(Register VReg) {
    return allocate(VReg, 0, nullptr);
}

bool LinearScanRegAlloc::tryCoalesceCopy(MachineInstr &MI) {
    if (MI.getOpcode() != MCOpcode::COPY) return false;
    if (MI.getNumOperands() < 2) return false;
    Register dst = MI.getOperand(0).getReg();
    Register src = MI.getOperand(1).getReg();

    auto srcIt = V2P.find(src);
    if (srcIt == V2P.end()) return false;

    Register srcPhys = srcIt->second;
    V2P[dst] = srcPhys;
    CopyEliminated++;
    return true;
}

// ============================================================
// run — main entry point
// ============================================================
void LinearScanRegAlloc::run(MachineFunction &MF) {
    this->MF = &MF;

    assignPositionsAndComputeUsesDefs(MF);
    solveLiveInOut(MF);
    buildLiveRanges(MF);

    FreeRegs.clear();
    for (auto reg : TRI.getAllocatableRegs()) FreeRegs.push_back(reg);
    Occupied.clear();
    V2P.clear();
    SpillSlots.clear();
    PendingSpillStores.clear();
    CopyEliminated = 0;
    Spills = 0;

    // --- Phase 4: Linear scan with spill/reload ---
    for (auto &mbb : MF.getBasicBlocks()) {
        auto &insts = mbb->getInstList();
        unsigned blockStart = BlockInfo[mbb.get()].StartPos;
        unsigned pos = blockStart;
        const std::set<Register> &liveOut = BlockInfo[mbb.get()].LiveOut;

        // At block entry, reload any LiveIn vregs that were spilled
        // (needed for loop back-edges)
        const std::set<Register> &liveIn = BlockInfo[mbb.get()].LiveIn;
        for (auto it = insts.begin(); it != insts.end(); ) {
            // Only insert reloads at the very beginning (before first real instr)
            MachineInstr &mi = *it;
            if (mi.isCopy()) { ++it; continue; }
            // Found first real instruction — insert reloads before it
            bool inserted = false;
            for (Register vreg : liveIn) {
                auto spillIt = SpillSlots.find(vreg);
                if (spillIt != SpillSlots.end() && V2P.find(vreg) == V2P.end()) {
                    Register phys = allocate(vreg, blockStart, &liveOut);
                    MachineInstr reloadMI(MCOpcode::MOV);
                    reloadMI.addReg(phys, true);
                    reloadMI.addFrameIndex(spillIt->second);
                    it = insts.insert(it, std::move(reloadMI));
                    ++it;
                    // NOTE: do not increment pos — inserted instructions
                    // use the position of the following original instruction
                    inserted = true;
                }
            }
            if (inserted) {
                // After inserting reloads, `it` points past them.
                // Need to continue to the real instruction.
                // But the real instruction is now after the reloads.
                // Just break out and let the main loop handle it.
            }
            break;
        }

        for (auto it = insts.begin(); it != insts.end(); ) {
            MachineInstr &mi = *it;
            freeDeadRegs(pos, &liveOut);

            if (mi.isCopy() && tryCoalesceCopy(mi)) {
                it = insts.erase(it);
                pos++;
                continue;
            }

            // Allocate DEF operands first (may trigger spills)
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                if (!mo.isDef()) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;
                        Register phys = allocate(vreg, pos, &liveOut);
            }

            // Emit spill stores BEFORE this instruction
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
                // NOTE: do not increment pos — inserted instructions
                // share the position of the following original instruction
            }

            // Reload spilled USE operands
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                if (mo.isDef()) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;

                auto spillIt = SpillSlots.find(vreg);
                if (spillIt != SpillSlots.end()) {
                    Register phys = allocate(vreg, pos, &liveOut);
                    MachineInstr reloadMI(MCOpcode::MOV);
                    reloadMI.addReg(phys, true);
                    reloadMI.addFrameIndex(spillIt->second);
                    it = insts.insert(it, std::move(reloadMI));
                    ++it;
                    // NOTE: do not increment pos — inserted instructions
                    // share the position of the following original instruction
                }
            }

            // Allocate remaining USE operands
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                if (mo.isDef()) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;
                auto spillIt = SpillSlots.find(vreg);
                if (spillIt != SpillSlots.end()) continue;
                Register phys = allocate(vreg, pos, &liveOut);
                (void)phys;
            }

            // Emit any remaining spill stores (also share original instruction pos)
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
                // NOTE: do not increment pos — inserted instructions
                // share the position of the following original instruction
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
        }
    }

    std::cerr << "[regalloc] " << MF.getName() << ": " << Spills << " spills, "
              << CopyEliminated << " copies coalesced\n";
}

} // namespace ll1
