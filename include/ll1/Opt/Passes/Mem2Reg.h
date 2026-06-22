#ifndef LL1_OPT_PASSES_MEM2REG_H
#define LL1_OPT_PASSES_MEM2REG_H

#include <ll1/Opt/PassManager.h>
#include <ll1/Opt/Passes/Dominators.h>
#include <ll1/IR/IRBuilder.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <set>

namespace ll1 {

class Mem2Reg : public FunctionPass {
public:
    const char* passName() const override { return "Mem2Reg"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;

private:
    std::vector<AllocaInst*> collectPromotableAllocas(Function &F);
    bool isPromotable(AllocaInst *alloca);

    std::set<BasicBlock*> computeIDF(
        std::set<BasicBlock*> &defBlocks,
        DominatorTree::Result &DT);
};

} // namespace ll1

#endif
