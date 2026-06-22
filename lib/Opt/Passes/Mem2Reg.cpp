#include <ll1/Opt/Passes/Mem2Reg.h>
#include <ll1/Opt/Passes/Dominators.h>
#include <ll1/IR/LLVMContext.h>
#include <iostream>
#include <queue>
#include <set>

namespace ll1 {

// ============================================================
// collectPromotableAllocas & isPromotable
// ============================================================
std::vector<AllocaInst*> Mem2Reg::collectPromotableAllocas(Function &F) {
    std::vector<AllocaInst*> result;
    for (auto &bb : F.getBasicBlocks()) {
        for (auto &inst : bb->getInstList()) {
            if (auto *alloca = dynamic_cast<AllocaInst*>(inst.get())) {
                if (isPromotable(alloca)) {
                    result.push_back(alloca);
                }
            }
        }
    }
    return result;
}

bool Mem2Reg::isPromotable(AllocaInst *alloca) {
    if (alloca->isArrayAlloca()) return false;

    // All uses must be Load or Store
    for (auto *use : alloca->getUses()) {
        auto *inst = dynamic_cast<Instruction*>(use);
        if (!inst) return false;
        auto op = inst->getOpcode();
        if (op != Instruction::Opcode::Load && op != Instruction::Opcode::Store) {
            return false;
        }
    }
    return true;
}

// ============================================================
// computeIDF — Iterated Dominance Frontier
// ============================================================
std::set<BasicBlock*> Mem2Reg::computeIDF(
    std::set<BasicBlock*> &defBlocks,
    DominatorTree::Result &DT) {

    std::set<BasicBlock*> idf;
    std::queue<BasicBlock*> worklist;

    for (auto *bb : defBlocks) worklist.push(bb);

    while (!worklist.empty()) {
        BasicBlock *B = worklist.front(); worklist.pop();
        auto dfIt = DT.DomFrontier.find(B);
        if (dfIt == DT.DomFrontier.end()) continue;
        for (auto *X : dfIt->second) {
            if (idf.insert(X).second) worklist.push(X);
        }
    }
    return idf;
}

// ============================================================
// rename — dominator tree walk for a single alloca
// ============================================================
static void rename(
    BasicBlock *BB,
    DominatorTree::Result &DT,
    AllocaInst *alloca,
    std::stack<Value*> &reachingDef,
    std::vector<Instruction*> &toRemove) {

    unsigned pushedHere = 0;

    auto &instList = BB->getInstList();
    for (auto it = instList.begin(); it != instList.end(); ) {
        Instruction *inst = it->get();
        auto op = inst->getOpcode();

        if (op == Instruction::Opcode::Store) {
            auto *store = static_cast<StoreInst*>(inst);
            if (store->getPointer() == alloca) {
                Value *stored = store->getValue();
                reachingDef.push(stored);
                pushedHere++;
                toRemove.push_back(store);
                ++it;
                continue;
            }
        }
        else if (op == Instruction::Opcode::Load) {
            auto *load = static_cast<LoadInst*>(inst);
            if (load->getPointer() == alloca) {
                if (!reachingDef.empty()) {
                    load->replaceAllUsesWith(reachingDef.top());
                    toRemove.push_back(load);
                    ++it;
                    continue;
                }
            }
        }
        ++it;
    }

    // Fill phi incoming values for successors
    for (auto *succ : BB->getSuccessors()) {
        for (auto &inst : succ->getInstList()) {
            auto *phi = dynamic_cast<PhiInst*>(inst.get());
            if (!phi) break;

            // Check if this phi handles this alloca
            std::string expectedName = alloca->getName() + "_phi";
            if (phi->getName() == expectedName && !reachingDef.empty()) {
                phi->addIncoming(reachingDef.top(), BB);
            }
        }
    }

    // Process children, pushing their phis as reaching defs
    auto childIt = DT.Children.find(BB);
    if (childIt != DT.Children.end()) {
        for (auto *child : childIt->second) {
            // If child has a phi for this alloca, push it
            for (auto &inst : child->getInstList()) {
                auto *phi = dynamic_cast<PhiInst*>(inst.get());
                if (!phi) break;
                std::string expectedName = alloca->getName() + "_phi";
                if (phi->getName() == expectedName) {
                    reachingDef.push(phi);
                    pushedHere++;
                }
            }

            rename(child, DT, alloca, reachingDef, toRemove);

            // Pop child's phi
            for (auto &inst : child->getInstList()) {
                auto *phi = dynamic_cast<PhiInst*>(inst.get());
                if (!phi) break;
                std::string expectedName = alloca->getName() + "_phi";
                if (phi->getName() == expectedName && !reachingDef.empty()) {
                    reachingDef.pop();
                }
            }
        }
    }

    // Pop values pushed in this BB
    while (pushedHere-- > 0 && !reachingDef.empty()) {
        reachingDef.pop();
    }
}

// ============================================================
// run
// ============================================================
PreservedAnalyses Mem2Reg::run(Function &F, FunctionAnalysisManager &FAM) {
    auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);

    auto allocas = collectPromotableAllocas(F);
    if (allocas.empty()) {
        return PreservedAnalyses::all();
    }

    LLVMContext ctx;
    IRBuilder builder(ctx);

    // Step 1: For each alloca, place phi nodes
    for (auto *alloca : allocas) {
        // Find def blocks
        std::set<BasicBlock*> defBlocks;
        for (auto *use : alloca->getUses()) {
            auto *inst = dynamic_cast<Instruction*>(use);
            if (inst && inst->getOpcode() == Instruction::Opcode::Store && inst->getParent()) {
                defBlocks.insert(inst->getParent());
            }
        }

        if (defBlocks.empty()) continue;

        // Compute IDF of def blocks
        auto idf = computeIDF(defBlocks, DT);

        // Place phi at each IDF block
        for (auto *bb : idf) {
            auto phi = std::make_unique<PhiInst>(alloca->getAllocatedType(),
                                                  alloca->getName() + "_phi");
            bb->pushFront(std::move(phi));
        }
    }

    // Step 2: Rename — walk dominator tree for each alloca
    std::vector<Instruction*> toRemove;
    for (auto *alloca : allocas) {
        std::stack<Value*> reachingDef;
        // Push initial undefined value (zero)
        reachingDef.push(builder.getInt16(0));

        BasicBlock *entry = F.getEntryBlock();
        if (entry) {
            rename(entry, DT, alloca, reachingDef, toRemove);
        }
    }

    // Step 3: Remove dead instructions (loads and stores)
    for (auto *inst : toRemove) {
        auto *bb = inst->getParent();
        if (!bb) continue;
        auto &list = bb->getInstList();
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->get() == inst) {
                list.erase(it);
                break;
            }
        }
    }

    // Step 4: Remove promoted allocas (all loads/stores were replaced)
    unsigned promoted = 0;
    for (auto *alloca : allocas) {
        // Check if any use remains that is NOT in toRemove
        bool allRemoved = true;
        for (auto *use : alloca->getUses()) {
            auto *inst = dynamic_cast<Instruction*>(use);
            if (inst) {
                bool found = false;
                for (auto *dead : toRemove) {
                    if (dead == inst) { found = true; break; }
                }
                if (!found) { allRemoved = false; break; }
            }
        }
        if (allRemoved) {
            auto *bb = alloca->getParent();
            if (bb) {
                auto &list = bb->getInstList();
                for (auto it = list.begin(); it != list.end(); ++it) {
                    if (it->get() == alloca) {
                        list.erase(it);
                        promoted++;
                        break;
                    }
                }
            }
        }
    }

    // Step 5: Remove degenerate phis (all incoming values are identical)
    for (auto &bb : F.getBasicBlocks()) {
        auto &list = bb->getInstList();
        auto it = list.begin();
        while (it != list.end()) {
            auto *phi = dynamic_cast<PhiInst*>(it->get());
            if (!phi) break;

            bool degenerate = false;
            if (phi->getNumIncoming() > 1) {
                Value *first = phi->getIncomingValue(0);
                if (first && first != phi) {
                    degenerate = true;
                    for (unsigned i = 1; i < phi->getNumIncoming(); ++i) {
                        Value *v = phi->getIncomingValue(i);
                        if (v != first && v != phi) { degenerate = false; break; }
                    }
                }
            }

            if (degenerate) {
                phi->replaceAllUsesWith(phi->getIncomingValue(0));
                it = list.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::cerr << "[opt] Mem2Reg: promoted " << promoted
              << " allocas in " << F.getName() << std::endl;

    return PreservedAnalyses::none();
}

} // namespace ll1
