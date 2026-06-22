#include <ll1/Opt/Passes/DCE.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>

#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

namespace ll1 {

// ============================================================
// run — Aggressive Dead Code Elimination (mark-sweep)
// ============================================================
PreservedAnalyses DCSPass::run(Function &F, FunctionAnalysisManager &/*FAM*/) {
    // ============================================================
    // Phase 1: Mark all instructions as dead
    // ============================================================
    std::set<Instruction*> deadInstructions;

    for (auto &bb : F.getBasicBlocks()) {
        for (auto &inst : bb->getInstList()) {
            deadInstructions.insert(inst.get());
        }
    }

    // ============================================================
    // Phase 2: Mark inherently live instructions
    //
    // An instruction is inherently live if it has side effects.
    // Terminators (Ret, Br): control flow — must be kept.
    // Store, ArrayStore: write to memory — side effect.
    // Call: may have arbitrary side effects.
    // ============================================================
    std::queue<Instruction*> worklist;

    for (auto &bb : F.getBasicBlocks()) {
        for (auto &inst : bb->getInstList()) {
            auto op = inst->getOpcode();
            bool isLive = false;

            switch (op) {
            case Instruction::Opcode::Ret:
            case Instruction::Opcode::Br:
            case Instruction::Opcode::Store:
            case Instruction::Opcode::ArrayStore:
            case Instruction::Opcode::Call:
                isLive = true;
                break;
            default:
                break;
            }

            if (isLive) {
                deadInstructions.erase(inst.get());
                worklist.push(inst.get());
            }
        }
    }

    // ============================================================
    // Phase 3: Propagate liveness (worklist-driven)
    //
    // For each live instruction I, its operand instructions are
    // also live because they produce values consumed by I.
    // ============================================================
    while (!worklist.empty()) {
        Instruction *I = worklist.front();
        worklist.pop();

        for (unsigned i = 0; i < I->getNumOperands(); ++i) {
            Value *op = I->getOperand(i);
            if (!op) continue;

            auto *opInst = dynamic_cast<Instruction*>(op);
            if (!opInst) continue;

            // If this operand instruction is still marked dead, make it live
            auto it = deadInstructions.find(opInst);
            if (it != deadInstructions.end()) {
                deadInstructions.erase(it);
                worklist.push(opInst);
            }
        }
    }

    // ============================================================
    // Phase 4: Sweep — remove dead instructions from each BB
    //
    // Walk each BB's instruction list. If an instruction is in the
    // dead set, erase it. Erase returns the next iterator, so only
    // advance when we do NOT erase.
    // ============================================================
    unsigned removedCount = 0;
    for (auto &bb : F.getBasicBlocks()) {
        auto &instList = bb->getInstList();
        for (auto it = instList.begin(); it != instList.end(); ) {
            if (deadInstructions.count(it->get())) {
                it = instList.erase(it);
                ++removedCount;
            } else {
                ++it;
            }
        }
    }

    std::cerr << "[opt] DCE: removed " << removedCount
              << " dead instruction(s) from " << F.getName() << std::endl;

    // DCE does not modify the CFG structure, so CFG analyses are preserved
    return PreservedAnalyses().preserveSet<CFGAnalyses>();
}

} // namespace ll1
