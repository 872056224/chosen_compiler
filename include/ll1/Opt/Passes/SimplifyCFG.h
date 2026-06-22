#ifndef LL1_OPT_PASSES_SIMPLIFYCFG_H
#define LL1_OPT_PASSES_SIMPLIFYCFG_H

#include <ll1/Opt/PassManager.h>

namespace ll1 {

class SimplifyCFG : public FunctionPass {
public:
    const char* passName() const override { return "SimplifyCFG"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;
};

} // namespace ll1

#endif
