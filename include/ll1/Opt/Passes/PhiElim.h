#ifndef LL1_OPT_PASSES_PHIELIM_H
#define LL1_OPT_PASSES_PHIELIM_H

#include <ll1/Opt/PassManager.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/IRBuilder.h>

namespace ll1 {

// Converts phi instructions into alloca+store+load patterns
// Runs after Mem2Reg to make the IR acceptable to CodeGen
class PhiElimination : public FunctionPass {
public:
    const char* passName() const override { return "PhiElimination"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;
};

} // namespace ll1

#endif
