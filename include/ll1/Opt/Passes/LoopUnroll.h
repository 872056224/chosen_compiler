#ifndef LL1_OPT_PASSES_LOOPUNROLL_H
#define LL1_OPT_PASSES_LOOPUNROLL_H

#include <ll1/Opt/PassManager.h>
#include <ll1/Opt/Passes/Dominators.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <ll1/IR/IRBuilder.h>
#include <vector>
#include <set>
#include <map>

namespace ll1 {

// ============================================================
// LoopUnroll — full unrolling of loops with known small trip count
//
// Detects loops via back-edges in the dominator tree. For each
// natural loop with a constant-compare exit condition against a
// known bound ≤ MaxTripCount, duplicates the loop body N times
// (linear form, no epilogue needed for full unroll).
// ============================================================
class LoopUnroll : public FunctionPass {
public:
    explicit LoopUnroll(unsigned maxTrip = 8) : MaxTripCount(maxTrip) {}

    const char* passName() const override { return "LoopUnroll"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;

private:
    unsigned MaxTripCount;

    // Find all natural loops from DT back-edges
    struct Loop {
        BasicBlock *Header;
        std::set<BasicBlock*> Blocks;   // loop body blocks
        BasicBlock *Latch;              // block with back-edge to header
        BasicBlock *Preheader;          // single predecessor outside loop
        ICmpInst *ExitCond;             // the exit condition (icmp)
        BranchInst *ExitBr;             // the branch using the exit condition
    };

    std::vector<Loop> findLoops(Function &F, DominatorTree::Result &DT);

    // Try to determine the trip count of a loop
    // Returns 0 if unable to determine
    unsigned getTripCount(Loop &L);

    // Fully unroll a loop with known trip count
    void unroll(Function &F, Loop &L, unsigned tripCount, LLVMContext &ctx);
};

} // namespace ll1
#endif
