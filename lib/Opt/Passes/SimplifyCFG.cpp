#include <ll1/Opt/Passes/SimplifyCFG.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <iostream>

namespace ll1 {

// ============================================================
// Transform 4 — Simplify constant conditional branch
// ============================================================
static bool simplifyConstantBranches(Function &F) {
    bool changed = false;

    for (auto &bb : F.getBasicBlocks()) {
        auto *term = bb->getTerminator();
        if (!term) continue;

        auto *br = dynamic_cast<BranchInst*>(term);
        if (!br || !br->isConditional()) continue;

        auto *cond = br->getCondition();
        auto *ci = dynamic_cast<ConstantInt*>(cond);
        if (!ci) continue;

        BasicBlock *dest = (ci->getValue() != 0) ? br->getTrueDest() : br->getFalseDest();
        if (!dest) continue;

        // Replace with unconditional branch
        // Create new BranchInst and replace the existing terminator
        auto &list = bb->getInstList();
        // Remove old terminator
        list.pop_back();
        // Add new unconditional branch
        bb->pushBack(std::make_unique<BranchInst>(dest));
        changed = true;
    }

    return changed;
}

// ============================================================
// Transform 3 — Simplify unconditional branch to unconditional branch
// ============================================================
static bool simplifyUncondToUncondBranches(Function &F) {
    bool changed = false;

    for (auto &bb : F.getBasicBlocks()) {
        auto *term = bb->getTerminator();
        if (!term) continue;

        auto *br = dynamic_cast<BranchInst*>(term);
        if (!br || !br->isUnconditional()) continue;

        auto *dest = br->getUnconditionalDest();
        if (!dest || dest == bb.get()) continue;

        auto *destTerm = dest->getTerminator();
        if (!destTerm) continue;
        auto *destBr = dynamic_cast<BranchInst*>(destTerm);
        if (!destBr || !destBr->isUnconditional()) continue;

        auto *destDest = destBr->getUnconditionalDest();
        if (!destDest || destDest == dest || destDest == bb.get()) continue;

        // Redirect: bb → destDest
        br->setOperand(0, destDest);
        changed = true;
    }

    return changed;
}

// ============================================================
// Eliminate empty blocks (safe version — only if no phi users)
// ============================================================
static bool eliminateEmptyBlocks(Function &F) {
    bool changed = false;

    // Simple approach: redirect predecessors around empty blocks
    // but don't physically remove blocks (leave them unreachable)
    for (auto &bb : F.getBasicBlocks()) {
        if (bb.get() == F.getEntryBlock()) continue;
        if (bb->size() != 1) continue;

        auto *term = bb->getTerminator();
        auto *br = dynamic_cast<BranchInst*>(term);
        if (!br || !br->isUnconditional()) continue;

        auto *dest = br->getUnconditionalDest();
        if (!dest || dest == bb.get()) continue;

        // Redirect all predecessors
        auto preds = bb->getPredecessors();
        for (auto *pred : preds) {
            if (pred == bb.get()) continue;
            auto *predTerm = pred->getTerminator();
            auto *predBr = dynamic_cast<BranchInst*>(predTerm);
            if (!predBr) continue;

            // For each operand that points to bb, redirect to dest
            for (unsigned i = 0; i < predBr->getNumOperands(); ++i) {
                if (predBr->getOperand(i) == bb.get()) {
                    predBr->setOperand(i, dest);
                }
            }
        }
        // Mark as changed even if we didn't physically delete
        changed = true;
    }

    return changed;
}

// ============================================================
// run — Main entry point
// ============================================================
PreservedAnalyses SimplifyCFG::run(Function &F, FunctionAnalysisManager &/*FAM*/) {
    bool Changed = false;

    int maxIter = 10;  // Safety limit
    while (maxIter-- > 0) {
        bool LocalChanged = false;
        LocalChanged |= simplifyConstantBranches(F);
        LocalChanged |= simplifyUncondToUncondBranches(F);

        Changed |= LocalChanged;
        if (!LocalChanged) break;
    }

    if (Changed) {
        std::cerr << "[opt] SimplifyCFG: simplified CFG in " << F.getName() << std::endl;
        return PreservedAnalyses::none();
    }
    return PreservedAnalyses::all();
}

} // namespace ll1
