#ifndef LL1_OPT_PASSES_INSTCOMBINE_H
#define LL1_OPT_PASSES_INSTCOMBINE_H

#include <ll1/Opt/PassManager.h>

namespace ll1 {

class InstCombine : public FunctionPass {
public:
    const char* passName() const override { return "InstCombine"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;
};

} // namespace ll1

#endif
