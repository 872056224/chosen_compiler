#include <ll1/Opt/Passes/LoopUnroll.h>
#include <ll1/IR/LLVMContext.h>
#include <iostream>
#include <unordered_map>

namespace ll1 {

static std::vector<BasicBlock*> getSuccessors(BasicBlock *BB) {
    std::vector<BasicBlock*> succs;
    auto *term = BB->getTerminator();
    if (!term) return succs;
    if (auto *br = dynamic_cast<BranchInst*>(term)) {
        if (br->isConditional()) {
            if (auto *t = br->getTrueDest()) succs.push_back(t);
            if (auto *f = br->getFalseDest()) succs.push_back(f);
        } else if (br->isUnconditional()) {
            if (auto *d = br->getUnconditionalDest()) succs.push_back(d);
        }
    }
    return succs;
}

static void collectLoopBlocks(BasicBlock *Start, BasicBlock *Header,
                               std::set<BasicBlock*> &visited,
                               std::set<BasicBlock*> &blocks) {
    if (Start == Header || visited.count(Start)) return;
    visited.insert(Start);
    blocks.insert(Start);
    for (auto *succ : getSuccessors(Start)) {
        collectLoopBlocks(succ, Header, visited, blocks);
    }
}

std::vector<LoopUnroll::Loop> LoopUnroll::findLoops(Function &F,
                                                       DominatorTree::Result &DT) {
    std::vector<Loop> loops;
    for (auto &bb : F.getBasicBlocks()) {
        BasicBlock *S = bb.get();
        for (auto *H : getSuccessors(S)) {
            if (!DT.dominates(H, S)) continue;
            Loop L;
            L.Header = H;
            L.Latch = S;
            L.Blocks.insert(H);
            std::set<BasicBlock*> visited;
            visited.insert(H);
            for (auto *pred : S->getPredecessors()) {
                collectLoopBlocks(pred, H, visited, L.Blocks);
            }
            if (L.Blocks.count(S) == 0) L.Blocks.insert(S);

            for (auto *pred : H->getPredecessors()) {
                if (L.Blocks.count(pred) == 0) { L.Preheader = pred; break; }
            }
            if (!L.Preheader) continue;

            auto *term = H->getTerminator();
            if (auto *br = dynamic_cast<BranchInst*>(term)) {
                if (br->isConditional()) {
                    if (auto *cond = dynamic_cast<ICmpInst*>(br->getCondition())) {
                        L.ExitCond = cond;
                        L.ExitBr = br;
                    }
                }
            }
            if (!L.ExitCond) continue;
            loops.push_back(L);
        }
    }
    return loops;
}

unsigned LoopUnroll::getTripCount(Loop &L) {
    if (!L.ExitCond) return 0;
    auto *lhs = L.ExitCond->getOperand(0);
    auto *rhs = L.ExitCond->getOperand(1);
    auto pred = L.ExitCond->getPredicate();

    Value *iv = nullptr;
    ConstantInt *bound = nullptr;
    if (auto *ci = dynamic_cast<ConstantInt*>(rhs)) {
        iv = lhs; bound = ci;
    } else if (auto *ci = dynamic_cast<ConstantInt*>(lhs)) {
        iv = rhs; bound = ci;
        pred = (pred == ICmpInst::SLT) ? ICmpInst::SGT : ICmpInst::SGE;
    }
    if (!iv || !bound) return 0;

    if (pred == ICmpInst::SLT) {
        int16_t n = bound->getValue();
        if (n <= 0 || (unsigned)n > MaxTripCount) return 0;
        return (unsigned)n;
    }
    if (pred == ICmpInst::SLE) {
        unsigned count = (unsigned)(bound->getValue() + 1);
        return (count <= MaxTripCount) ? count : 0;
    }
    return 0;
}

void LoopUnroll::unroll(Function &F, Loop &L, unsigned tripCount, LLVMContext &ctx) {
    if (tripCount < 2) return;

    // Find exit block from header's branch
    BasicBlock *exitBlock = nullptr;
    BasicBlock *bodyEntry = nullptr;
    auto *term = L.Header->getTerminator();
    if (auto *br = dynamic_cast<BranchInst*>(term)) {
        if (br->isConditional()) {
            auto *t = br->getTrueDest();
            auto *f = br->getFalseDest();
            if (L.Blocks.count(t)) { bodyEntry = t; exitBlock = f; }
            else { bodyEntry = f; exitBlock = t; }
        }
    }
    if (!exitBlock || !bodyEntry) return;

    // Clone the loop body (tripCount - 1) times using IRBuilder
    // Each clone is a copy of bodyEntry's instructions with remapped operands
    IRBuilder builder(ctx);
    std::vector<BasicBlock*> clonedBodies;

    for (unsigned i = 1; i < tripCount; ++i) {
        auto cloneBB = std::make_unique<BasicBlock>(bodyEntry->getName() + "_uc" +
                                                      std::to_string(i));

        // Map original values to their clones (for operand remapping)
        // For now, instructions within a loop body iteration don't reference
        // values from other iterations, so we can clone without remapping.
        // The terminator (branch) will be rewired below.

        for (auto &inst : bodyEntry->getInstList()) {
            auto op = inst->getOpcode();
            if (op == Instruction::Opcode::Br) {
                // Skip — we'll add a new terminator below
            } else if (op == Instruction::Opcode::Call) {
                auto *call = static_cast<CallInst*>(inst.get());
                auto newCall = std::make_unique<CallInst>(
                    static_cast<Function*>(call->getOperand(0)), "");
                for (unsigned a = 1; a < call->getNumOperands(); ++a)
                    newCall->addArg(call->getOperand(a));
                cloneBB->pushBack(std::move(newCall));
            } else if (op == Instruction::Opcode::ArrayLoad) {
                auto *al = static_cast<ArrayLoadInst*>(inst.get());
                cloneBB->pushBack(std::make_unique<ArrayLoadInst>(
                    inst->getType(), al->getBase(), al->getIndex(), ""));
            } else if (op == Instruction::Opcode::ArrayStore) {
                auto *as = static_cast<ArrayStoreInst*>(inst.get());
                cloneBB->pushBack(std::make_unique<ArrayStoreInst>(
                    as->getValue(), as->getBase(), as->getIndex()));
            } else if (op == Instruction::Opcode::Add ||
                       op == Instruction::Opcode::Sub) {
                cloneBB->pushBack(std::make_unique<BinaryOpInst>(
                    op, inst->getOperand(0), inst->getOperand(1), ""));
            } else if (op == Instruction::Opcode::Load) {
                cloneBB->pushBack(std::make_unique<LoadInst>(
                    inst->getType(), inst->getOperand(0), ""));
            } else if (op == Instruction::Opcode::Store) {
                cloneBB->pushBack(std::make_unique<StoreInst>(
                    inst->getOperand(0), inst->getOperand(1)));
            } else if (op == Instruction::Opcode::ICmp) {
                auto *icmp = static_cast<ICmpInst*>(inst.get());
                cloneBB->pushBack(std::make_unique<ICmpInst>(
                    icmp->getPredicate(), icmp->getOperand(0),
                    icmp->getOperand(1), ""));
            } else {
                std::cerr << "[unroll] unhandled opcode " << (int)op
                          << " in clone\n";
            }
        }

        // Wire clone to next clone or exit
        if (i + 1 < tripCount) {
            // Branch to exit for now (rewired below)
            cloneBB->pushBack(std::make_unique<BranchInst>(exitBlock));
        } else {
            // Last clone: branch to exit
            cloneBB->pushBack(std::make_unique<BranchInst>(exitBlock));
        }

        clonedBodies.push_back(cloneBB.get());
        F.addBasicBlock(std::move(cloneBB));
    }

    // Rewire: replace bodyEntry's back-edge to point to first clone
    auto *bodyTerm = bodyEntry->getTerminator();
    if (auto *br = dynamic_cast<BranchInst*>(bodyTerm)) {
        if (br->isUnconditional() && br->getUnconditionalDest() == L.Header) {
            // Replace bodyEntry's terminator to branch to first clone
            auto newBr = std::make_unique<BranchInst>(clonedBodies[0]);
            // Remove old terminator and add new one
            auto &list = bodyEntry->getInstList();
            for (auto it = list.begin(); it != list.end(); ++it) {
                if (it->get() == br) {
                    list.erase(it);
                    break;
                }
            }
            bodyEntry->pushBack(std::move(newBr));
        }
    }

    // Wire intermediate clones to each other
    for (unsigned i = 0; i < clonedBodies.size(); ++i) {
        if (i + 1 < clonedBodies.size()) {
            auto *ct = clonedBodies[i]->getTerminator();
            if (auto *cbr = dynamic_cast<BranchInst*>(ct)) {
                // Replace placeholder with actual branch
                auto &list = clonedBodies[i]->getInstList();
                for (auto it = list.begin(); it != list.end(); ++it) {
                    if (it->get() == cbr) {
                        list.erase(it);
                        break;
                    }
                }
                clonedBodies[i]->pushBack(
                    std::make_unique<BranchInst>(clonedBodies[i + 1]));
            }
        }
    }

    std::cerr << "[unroll] unrolled loop (header=" << L.Header->getName()
              << ") tripCount=" << tripCount << " → " << clonedBodies.size()
              << " clones\n";
}

PreservedAnalyses LoopUnroll::run(Function &F, FunctionAnalysisManager &FAM) {
    auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);
    auto loops = findLoops(F, DT);
    if (loops.empty()) return PreservedAnalyses::all();

    LLVMContext ctx;
    bool changed = false;
    for (auto &L : loops) {
        unsigned tc = getTripCount(L);
        if (tc == 0) continue;
        unroll(F, L, tc, ctx);
        changed = true;
    }
    if (changed) return PreservedAnalyses::none();
    return PreservedAnalyses::all();
}

} // namespace ll1
