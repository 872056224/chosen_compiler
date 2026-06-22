#ifndef LL1_OPT_PASSES_DCE_H
#define LL1_OPT_PASSES_DCE_H

#include <ll1/Opt/PassManager.h>
#include <set>
#include <vector>

namespace ll1 {

class DCSPass : public FunctionPass {
public:
    const char* passName() const override { return "DCE"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;
};

} // namespace ll1

#endif
