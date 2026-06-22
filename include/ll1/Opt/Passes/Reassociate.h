#ifndef LL1_OPT_PASSES_REASSOCIATE_H
#define LL1_OPT_PASSES_REASSOCIATE_H

#include <ll1/Opt/PassManager.h>

namespace ll1 {

class Reassociate : public FunctionPass {
public:
    const char* passName() const override { return "Reassociate"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;
};

} // namespace ll1

#endif
