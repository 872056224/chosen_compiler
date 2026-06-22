#include <ll1/Opt/Passes/Reassociate.h>
#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/Type.h>
#include <algorithm>
#include <vector>

namespace ll1 {

// ============================================================
// collectChain — flatten a tree of same-opcode binary ops
// ============================================================
static void collectChain(Instruction::Opcode op, Value *v,
                          std::vector<Value*> &operands) {
    if (auto *binOp = dynamic_cast<BinaryOpInst*>(v)) {
        if (binOp->getOpcode() == op) {
            collectChain(op, binOp->getLHS(), operands);
            collectChain(op, binOp->getRHS(), operands);
            return;
        }
    }
    operands.push_back(v);
}

// ============================================================
// getRank — sorting priority: constants first, then args, then
//           other instructions
// ============================================================
static int getRank(Value *v) {
    if (dynamic_cast<ConstantInt*>(v))
        return 0;  // Constants — highest priority
    if (dynamic_cast<Argument*>(v))
        return 1;  // Variable references
    return 2;      // Other instructions
}

// ============================================================
// foldConstants — fold adjacent ConstantInt operands
// ============================================================
static std::vector<Value*> foldConstants(Instruction::Opcode op,
                                           const std::vector<Value*> &operands) {
    std::vector<Value*> result;
    for (size_t i = 0; i < operands.size(); ) {
        if (i + 1 < operands.size()) {
            auto *c1 = dynamic_cast<ConstantInt*>(operands[i]);
            auto *c2 = dynamic_cast<ConstantInt*>(operands[i + 1]);
            if (c1 && c2) {
                int16_t val;
                if (op == Instruction::Opcode::Add) {
                    val = c1->getValue() + c2->getValue();
                } else {  // Mul
                    val = c1->getValue() * c2->getValue();
                }
                result.push_back(ConstantInt::get(Type::getInt16Ty(), val));
                i += 2;
                continue;
            }
        }
        result.push_back(operands[i]);
        ++i;
    }
    return result;
}

// ============================================================
// run — main entry point
// ============================================================
PreservedAnalyses Reassociate::run(Function &F, FunctionAnalysisManager &/*FAM*/) {
    for (auto &bb : F.getBasicBlocks()) {
        auto &instList = bb->getInstList();

        for (auto it = instList.begin(); it != instList.end(); ) {
            auto *inst = it->get();
            auto op = inst->getOpcode();

            // Only handle Add and Mul (skip Sub, SDiv, SRem, etc.)
            if (op != Instruction::Opcode::Add && op != Instruction::Opcode::Mul) {
                ++it;
                continue;
            }

            // Step 1: Collect leaf operands by flattening same-opcode chain
            std::vector<Value*> operands;
            collectChain(op, inst, operands);

            // Step 2: Count how many constants are in the chain
            unsigned constantCount = 0;
            for (auto *v : operands) {
                if (dynamic_cast<ConstantInt*>(v))
                    ++constantCount;
            }

            // Only beneficial if there are at least 2 constants to fold
            if (constantCount <= 1) {
                ++it;
                continue;
            }

            // Step 3: Sort by rank (constants first)
            std::sort(operands.begin(), operands.end(),
                      [](Value *a, Value *b) {
                          return getRank(a) < getRank(b);
                      });

            // Step 4: Fold adjacent constants
            auto folded = foldConstants(op, operands);

            if (folded.empty()) {
                ++it;
                continue;
            }

            // If the entire expression folded to a single value
            if (folded.size() == 1) {
                inst->replaceAllUsesWith(folded[0]);
                it = instList.erase(it);
                continue;
            }

            // Step 5: Rebuild as left-linear tree, inserting new instructions
            //         before the original instruction
            Value *accum = folded[0];
            auto insertPos = it;

            for (size_t i = 1; i < folded.size(); ++i) {
                auto newInst = std::make_unique<BinaryOpInst>(op, accum, folded[i]);
                auto *raw = newInst.get();
                insertPos = instList.insert(insertPos, std::move(newInst));
                ++insertPos;  // Advance past the newly inserted instruction
                accum = raw;
            }

            // Step 6: Replace original instruction with the new root
            inst->replaceAllUsesWith(accum);
            it = instList.erase(it);
        }
    }

    // Reassociation does not invalidate control-flow analyses
    return PreservedAnalyses().preserveSet<CFGAnalyses>();
}

} // namespace ll1
