#include <ll1/Opt/Passes/PhiElim.h>
#include <ll1/IR/LLVMContext.h>
#include <iostream>
#include <algorithm>

namespace ll1 {

PreservedAnalyses PhiElimination::run(Function &F, FunctionAnalysisManager &/*FAM*/) {
    LLVMContext ctx;
    IRBuilder builder(ctx);

    unsigned eliminatedCount = 0;

    // First pass: collect all phi instructions in the function
    struct PhiInfo {
        PhiInst *phi;
        BasicBlock *bb;
    };
    std::vector<PhiInfo> phis;

    for (auto &bb : F.getBasicBlocks()) {
        for (auto &inst : bb->getInstList()) {
            auto *phi = dynamic_cast<PhiInst*>(inst.get());
            if (!phi) break;  // Phis are always at the beginning
            phis.push_back({phi, bb.get()});
        }
    }

    if (phis.empty()) {
        return PreservedAnalyses::all();
    }

    // For each phi, create alloca + stores in predecessors + load
    for (auto &info : phis) {
        PhiInst *phi = info.phi;
        BasicBlock *phiBB = info.bb;

        // 1. Create alloca in entry block
        BasicBlock *entry = F.getEntryBlock();
        Type *ty = phi->getType();

        // Insert alloca at beginning of entry block
        auto alloca = std::make_unique<AllocaInst>(ty, phi->getName() + "_slot");
        auto *allocaRaw = alloca.get();
        entry->pushFront(std::move(alloca));

        // 2. Create a load to replace the phi
        auto load = std::make_unique<LoadInst>(ty, allocaRaw, phi->getName() + "_reload");
        auto *loadRaw = load.get();

        // Insert load right after the phi's position
        auto &phiBBList = phiBB->getInstList();
        auto insertAfter = std::find_if(phiBBList.begin(), phiBBList.end(),
            [phi](const auto &p) { return p.get() == phi; });
        if (insertAfter != phiBBList.end()) {
            ++insertAfter;
            phiBBList.insert(insertAfter, std::move(load));
        } else {
            phiBB->pushBack(std::move(load));
        }

        // 3. For each incoming pair, add store in the predecessor
        for (unsigned i = 0; i < phi->getNumIncoming(); ++i) {
            Value *incomingVal = phi->getIncomingValue(i);
            BasicBlock *predBB = phi->getIncomingBlock(i);

            // Insert store before the terminator of predBB
            auto store = std::make_unique<StoreInst>(incomingVal, allocaRaw);

            auto &predList = predBB->getInstList();
            // Find the terminator
            auto termIt = predList.end();
            if (!predList.empty()) {
                --termIt;  // Last instruction is the terminator
                predList.insert(termIt, std::move(store));
            } else {
                predBB->pushBack(std::move(store));
            }
        }

        // 4. Replace phi with the load
        phi->replaceAllUsesWith(loadRaw);

        // 5. Remove phi from its BB
        for (auto it = phiBBList.begin(); it != phiBBList.end(); ++it) {
            if (it->get() == phi) {
                phiBBList.erase(it);
                break;
            }
        }

        eliminatedCount++;
    }

    std::cerr << "[opt] PhiElimination: eliminated " << eliminatedCount
              << " phi nodes in " << F.getName() << std::endl;

    PreservedAnalyses PA;
    PA.preserveSet<CFGAnalyses>();
    return PA;
}

} // namespace ll1
