#include <ll1/CodeGen/MachineFunction.h>

namespace ll1 {

// ============================================================
// MachineBasicBlock
// ============================================================
void MachineBasicBlock::push_back(MachineInstr MI) {
    MI.setParent(this);
    Instructions.push_back(std::move(MI));
}

void MachineBasicBlock::push_front(MachineInstr MI) {
    MI.setParent(this);
    Instructions.insert(Instructions.begin(), std::move(MI));
}

void MachineBasicBlock::insertBeforeTerminator(MachineInstr MI) {
    MI.setParent(this);
    if (Instructions.empty()) {
        Instructions.push_back(std::move(MI));
        return;
    }
    // Insert before the last instruction (the terminator)
    auto it = Instructions.end();
    --it;
    Instructions.insert(it, std::move(MI));
}

MachineInstr* MachineBasicBlock::getTerminator() {
    if (Instructions.empty()) return nullptr;
    auto &last = Instructions.back();
    if (last.isTerminator()) return &last;
    return nullptr;
}

const MachineInstr* MachineBasicBlock::getTerminator() const {
    return const_cast<MachineBasicBlock*>(this)->getTerminator();
}

// ============================================================
// MachineFunction
// ============================================================
MachineBasicBlock* MachineFunction::createMachineBasicBlock(const std::string &name) {
    auto mbb = std::make_unique<MachineBasicBlock>(name);
    mbb->setParent(this);
    auto *raw = mbb.get();
    Blocks.push_back(std::move(mbb));
    return raw;
}

void MachineFunction::computeCFG() {
    // Build successor/predecessor lists from branch instructions
    for (auto &mbb : Blocks) {
        mbb->getSuccessors().clear();
        mbb->getPredecessors().clear();
    }

    for (auto &mbb : Blocks) {
        for (auto &mi : mbb->getInstList()) {
            for (unsigned i = 0; i < mi.getNumOperands(); ++i) {
                auto &op = mi.getOperand(i);
                if (op.getType() == MachineOperandType::MO_MachineBasicBlock) {
                    auto *succ = op.getMBB();
                    mbb->addSuccessor(succ);
                    succ->addPredecessor(mbb.get());
                }
            }
        }
    }
}

} // namespace ll1
