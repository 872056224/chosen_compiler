#ifndef LL1_OPT_PASSES_GVN_H
#define LL1_OPT_PASSES_GVN_H

#include <ll1/Opt/PassManager.h>
#include <ll1/Opt/Passes/Dominators.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <unordered_map>
#include <string>

namespace ll1 {

class GVN : public FunctionPass {
public:
    const char* passName() const override { return "GVN"; }
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) override;

    // Value-to-value-number mapping
    unsigned getOrCreateVN(Value *v);

    // Hash an instruction's computation
    std::string computeHash(Instruction *inst);

private:
    unsigned nextVN = 0;
    std::unordered_map<Value*, unsigned> valueNumbers;

    // Map from hash to dominating instruction with that hash
    std::unordered_map<std::string, Instruction*> hashToInst;
};

} // namespace ll1

#endif
