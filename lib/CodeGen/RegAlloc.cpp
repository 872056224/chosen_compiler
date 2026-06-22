#include <ll1/CodeGen/RegAlloc.h>

namespace ll1 {

LinearScanRegAlloc::LinearScanRegAlloc(const TargetRegisterInfo &tri) : TRI(tri) {
    for (auto reg : TRI.getAllocatableRegs()) {
        FreeRegs.push_back(reg);
    }
}

void LinearScanRegAlloc::computeLiveIntervals(MachineFunction &MF) {
    LiveIntervals.clear();
    unsigned pos = 0;
    for (auto &mbb : MF.getBasicBlocks()) {
        for (auto &mi : mbb->getInstList()) {
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() == MachineOperandType::MO_Register) {
                    Register reg = mo.getReg();
                    if (isVirtualRegister(reg)) {
                        LiveIntervals[reg].LastUse = pos;
                        if (mo.isDef() && LiveIntervals[reg].FirstDef == 0) {
                            LiveIntervals[reg].FirstDef = pos;
                        }
                    }
                }
            }
            pos++;
        }
    }
}

Register LinearScanRegAlloc::allocate(Register VReg, unsigned currentPos) {
    auto it = V2P.find(VReg);
    if (it != V2P.end()) return it->second;

    if (!FreeRegs.empty()) {
        Register phys = FreeRegs.back();
        FreeRegs.pop_back();
        V2P[VReg] = phys;
        unsigned lastUse = LiveIntervals.count(VReg) ? LiveIntervals[VReg].LastUse : 999999;
        Occupied[phys] = {VReg, lastUse};
        return phys;
    }

    // Evict the register with furthest last use
    Register evictPhys = FreeRegs.empty() ? X86::AX : FreeRegs.back();
    unsigned furthestUse = 0;
    for (auto &kv : Occupied) {
        if (kv.second.LastUse > furthestUse) {
            furthestUse = kv.second.LastUse;
            evictPhys = kv.first;
        }
    }

    spill(evictPhys);
    V2P[VReg] = evictPhys;
    unsigned lastUse = LiveIntervals.count(VReg) ? LiveIntervals[VReg].LastUse : 999999;
    Occupied[evictPhys] = {VReg, lastUse};
    return evictPhys;
}

void LinearScanRegAlloc::freeDeadRegs(unsigned currentPos) {
    std::vector<Register> toFree;
    for (auto &kv : Occupied) {
        if (kv.second.LastUse < currentPos) {
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
    getSpillSlot(vreg);  // allocates slot if needed
    PendingSpillStores.push_back({vreg, PhysReg});  // record physreg for store
    V2P.erase(vreg);
    Occupied.erase(it);
    FreeRegs.push_back(PhysReg);
    Spills++;
}

Register LinearScanRegAlloc::reload(Register VReg) {
    return allocate(VReg, 0);
}

int LinearScanRegAlloc::getSpillSlot(Register VReg) {
    auto it = SpillSlots.find(VReg);
    if (it != SpillSlots.end()) return it->second;
    int off = NextSpillSlot;
    NextSpillSlot -= 2;
    SpillSlots[VReg] = off;
    return off;
}

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
// run — main entry point with spill/reload insertion
// ============================================================
void LinearScanRegAlloc::run(MachineFunction &MF) {
    computeLiveIntervals(MF);

    FreeRegs.clear();
    for (auto reg : TRI.getAllocatableRegs()) FreeRegs.push_back(reg);
    Occupied.clear();
    V2P.clear();
    SpillSlots.clear();
    PendingSpillStores.clear();
    // Use negative offsets so spills go below bx/sp (e.g. [bx-20] = [sp-20]).
    // Positive offsets start after frame objects and would hit [bx+N] = [bp+N-52]
    // which overwrites saved BP / return address for N >= 52.
    NextSpillSlot = -20;
    CopyEliminated = 0;
    Spills = 0;

    unsigned pos = 0;
    for (auto &mbb : MF.getBasicBlocks()) {
        auto &insts = mbb->getInstList();
        for (auto it = insts.begin(); it != insts.end(); ) {
            MachineInstr &mi = *it;
            freeDeadRegs(pos);

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
                    // This vreg was spilled — reload into a register now
                    Register phys = allocate(vreg, pos);
                    MachineInstr reloadMI(MCOpcode::MOV);
                    reloadMI.addReg(phys, true);
                    reloadMI.addFrameIndex(spillIt->second);
                    it = insts.insert(it, std::move(reloadMI));
                    ++it; // advance past reload
                    pos++;
                }
            }

            // Allocate registers for ALL vreg operands
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &mo = mi.getOperand(i);
                if (mo.getType() != MachineOperandType::MO_Register) continue;
                Register vreg = mo.getReg();
                if (!isVirtualRegister(vreg)) continue;

                // If vreg was spilled, allocate now (reload above handles USE)
                Register phys = allocate(vreg, pos);
                (void)phys;
            }

            // Emit spill stores for any vreg evicted during allocation above.
            // Use the physreg recorded at eviction time (which holds the value).
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
                auto nextIt = std::next(it);
                it = insts.insert(nextIt, std::move(storeMI));
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
        }
    }
}

} // namespace ll1
