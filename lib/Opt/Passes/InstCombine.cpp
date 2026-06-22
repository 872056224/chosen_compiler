#include <ll1/Opt/Passes/InstCombine.h>
#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/Type.h>

#include <deque>
#include <vector>

namespace ll1 {

// ============================================================
// Helper: check if a Value is a ConstantInt with a specific value
// ============================================================
static bool isConstantInt(Value *v, int16_t expected) {
    if (auto *c = dynamic_cast<ConstantInt*>(v)) {
        return c->getValue() == expected;
    }
    return false;
}

// ============================================================
// Helper: get ConstantInt from Value, or nullptr
// ============================================================
static ConstantInt *asConstantInt(Value *v) {
    return dynamic_cast<ConstantInt*>(v);
}

// ============================================================
// Helper: remove an instruction from its parent basic block
// ============================================================
static void eraseInstruction(Instruction *inst) {
    BasicBlock *bb = inst->getParent();
    if (!bb) return;
    auto &list = bb->getInstList();
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->get() == inst) {
            list.erase(it);
            return;
        }
    }
}

// ============================================================
// Collect all instructions in the function into a worklist
// ============================================================
static void collectAllInstructions(Function &F, std::deque<Instruction*> &worklist) {
    for (auto &bb : F.getBasicBlocks()) {
        for (auto &inst : bb->getInstList()) {
            worklist.push_back(inst.get());
        }
    }
}

// ============================================================
// Constant folding for binary operations: C1 op C2 -> C1 op C2
// Returns the folded ConstantInt, or nullptr if not both constants
// ============================================================
static ConstantInt *foldBinaryOp(Instruction::Opcode op, Value *lhs, Value *rhs) {
    auto *c1 = asConstantInt(lhs);
    auto *c2 = asConstantInt(rhs);
    if (!c1 || !c2) return nullptr;

    int16_t a = c1->getValue();
    int16_t b = c2->getValue();
    int16_t result = 0;
    Type *int16Ty = Type::getInt16Ty();

    switch (op) {
    case Instruction::Opcode::Add:  result = a + b;  break;
    case Instruction::Opcode::Sub:  result = a - b;  break;
    case Instruction::Opcode::Mul:  result = a * b;  break;
    case Instruction::Opcode::SDiv:
        if (b == 0) return nullptr; // Division by zero
        result = a / b;
        break;
    case Instruction::Opcode::SRem:
        if (b == 0) return nullptr; // Division by zero
        result = a % b;
        break;
    case Instruction::Opcode::And:  result = a & b;  break;
    case Instruction::Opcode::Or:   result = a | b;  break;
    case Instruction::Opcode::Xor:  result = a ^ b;  break;
    default:
        return nullptr;
    }

    return ConstantInt::get(int16Ty, result);
}

// ============================================================
// Simplify a binary operation instruction
// Returns simplified value, or nullptr if no simplification possible
// ============================================================
static Value *simplifyBinaryOp(Instruction *inst) {
    auto op = inst->getOpcode();
    Value *lhs = inst->getOperand(0);
    Value *rhs = inst->getOperand(1);

    // Constant folding always applies first
    if (auto *folded = foldBinaryOp(op, lhs, rhs)) {
        return folded;
    }

    switch (op) {
    case Instruction::Opcode::Add:
        // x + 0 -> x
        if (isConstantInt(rhs, 0)) return lhs;
        // 0 + x -> x
        if (isConstantInt(lhs, 0)) return rhs;
        break;

    case Instruction::Opcode::Sub:
        // x - 0 -> x
        if (isConstantInt(rhs, 0)) return lhs;
        // x - x -> 0
        if (lhs == rhs) return ConstantInt::get(Type::getInt16Ty(), 0);
        // neg(neg x) -> x  (neg is sub 0, x)
        if (isConstantInt(lhs, 0)) {
            if (auto *rhsInst = dynamic_cast<Instruction*>(rhs)) {
                if (rhsInst->getOpcode() == Instruction::Opcode::Sub &&
                    isConstantInt(rhsInst->getOperand(0), 0)) {
                    return rhsInst->getOperand(1);
                }
            }
        }
        break;

    case Instruction::Opcode::Mul:
        // x * 1 -> x
        if (isConstantInt(rhs, 1)) return lhs;
        // 1 * x -> x
        if (isConstantInt(lhs, 1)) return rhs;
        break;

    case Instruction::Opcode::SDiv:
        // x / 1 -> x
        if (isConstantInt(rhs, 1)) return lhs;
        break;

    case Instruction::Opcode::And:
        // x & -1 -> x  (-1 as 16-bit)
        if (isConstantInt(rhs, -1)) return lhs;
        if (isConstantInt(lhs, -1)) return rhs;
        break;

    case Instruction::Opcode::Or:
        // x | 0 -> x, 0 | x -> x
        if (isConstantInt(rhs, 0)) return lhs;
        if (isConstantInt(lhs, 0)) return rhs;
        break;

    case Instruction::Opcode::Xor:
        // x ^ 0 -> x, 0 ^ x -> x
        if (isConstantInt(rhs, 0)) return lhs;
        if (isConstantInt(lhs, 0)) return rhs;
        // not(not x) -> x  (not is xor x, -1; XOR is commutative so
        // -1 can be in either operand position of the inner xor)
        if (isConstantInt(rhs, -1)) {
            if (auto *lhsInst = dynamic_cast<Instruction*>(lhs)) {
                if (lhsInst->getOpcode() == Instruction::Opcode::Xor) {
                    if (isConstantInt(lhsInst->getOperand(0), -1))
                        return lhsInst->getOperand(1);
                    if (isConstantInt(lhsInst->getOperand(1), -1))
                        return lhsInst->getOperand(0);
                }
            }
        }
        if (isConstantInt(lhs, -1)) {
            if (auto *rhsInst = dynamic_cast<Instruction*>(rhs)) {
                if (rhsInst->getOpcode() == Instruction::Opcode::Xor) {
                    if (isConstantInt(rhsInst->getOperand(0), -1))
                        return rhsInst->getOperand(1);
                    if (isConstantInt(rhsInst->getOperand(1), -1))
                        return rhsInst->getOperand(0);
                }
            }
        }
        break;

    default:
        break;
    }

    return nullptr;
}

// ============================================================
// Simplify an ICmp instruction
// ============================================================
static Value *simplifyICmp(Instruction *inst) {
    auto *icmp = static_cast<ICmpInst*>(inst);
    Value *lhs = icmp->getOperand(0);
    Value *rhs = icmp->getOperand(1);
    auto pred = icmp->getPredicate();

    // icmp EQ same operand -> true (ConstantInt 1 of Int1Ty)
    if (pred == ICmpInst::EQ && lhs == rhs) {
        return ConstantInt::get(Type::getInt1Ty(), 1);
    }

    // icmp NE same operand -> false (ConstantInt 0 of Int1Ty)
    if (pred == ICmpInst::NE && lhs == rhs) {
        return ConstantInt::get(Type::getInt1Ty(), 0);
    }

    return nullptr;
}

// ============================================================
// Simplify a cast instruction (SExt, ZExt, Trunc)
// ============================================================
static Value *simplifyCast(Instruction *inst) {
    auto op = inst->getOpcode();
    Value *src = inst->getOperand(0);
    auto *srcInst = dynamic_cast<Instruction*>(src);
    if (!srcInst) return nullptr;

    // trunc(zext x) -> x if types match
    // trunc(sext x) -> x if types match
    if (op == Instruction::Opcode::Trunc) {
        auto srcOp = srcInst->getOpcode();
        if (srcOp == Instruction::Opcode::ZExt || srcOp == Instruction::Opcode::SExt) {
            Value *x = srcInst->getOperand(0);
            if (inst->getType() == x->getType()) {
                return x;
            }
        }
    }

    // zext(trunc x) -> x if types match
    // sext(trunc x) -> x if types match
    if (op == Instruction::Opcode::ZExt || op == Instruction::Opcode::SExt) {
        if (srcInst->getOpcode() == Instruction::Opcode::Trunc) {
            Value *x = srcInst->getOperand(0);
            if (inst->getType() == x->getType()) {
                return x;
            }
        }
    }

    return nullptr;
}

// ============================================================
// Simplify a Branch instruction
// Branch with constant condition -> unconditional branch.
// This function handles the replacement internally (erase + insert)
// because branches don't produce SSA values.
// Returns true if simplification was performed.
// ============================================================
static bool simplifyBranch(Instruction *inst) {
    auto *br = static_cast<BranchInst*>(inst);

    if (!br->isConditional()) return false;

    Value *cond = br->getCondition();
    auto *c = asConstantInt(cond);
    if (!c) return false;

    // Determine which target to branch to
    BasicBlock *target = nullptr;
    if (c->getValue() == 0) {
        target = br->getFalseDest();
    } else {
        target = br->getTrueDest();
    }

    if (!target) return false;

    // Create a new unconditional branch
    auto newBr = std::make_unique<BranchInst>(target);

    // Replace the old conditional branch with the new unconditional one
    BasicBlock *bb = inst->getParent();
    if (!bb) return false;

    auto &list = bb->getInstList();
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->get() == inst) {
            // Erase old and insert new at the same position
            it = list.erase(it);
            newBr->setParent(bb);
            list.insert(it, std::move(newBr));
            return true;
        }
    }

    return false;
}

// ============================================================
// Main simplification dispatcher (for value-producing instructions)
// Returns simplified value, or nullptr if no simplification possible
// ============================================================
static Value *simplifyInstruction(Instruction *inst) {
    auto op = inst->getOpcode();

    switch (op) {
    case Instruction::Opcode::Add:
    case Instruction::Opcode::Sub:
    case Instruction::Opcode::Mul:
    case Instruction::Opcode::SDiv:
    case Instruction::Opcode::SRem:
    case Instruction::Opcode::And:
    case Instruction::Opcode::Or:
    case Instruction::Opcode::Xor:
        return simplifyBinaryOp(inst);

    case Instruction::Opcode::ICmp:
        return simplifyICmp(inst);

    case Instruction::Opcode::SExt:
    case Instruction::Opcode::ZExt:
    case Instruction::Opcode::Trunc:
        return simplifyCast(inst);

    default:
        return nullptr;
    }
}

// ============================================================
// run — Main entry point (worklist-driven peephole optimization)
// ============================================================
PreservedAnalyses InstCombine::run(Function &F, FunctionAnalysisManager &/*FAM*/) {
    std::deque<Instruction*> worklist;
    collectAllInstructions(F, worklist);
    unsigned simplifiedCount = 0;

    while (!worklist.empty()) {
        Instruction *inst = worklist.front();
        worklist.pop_front();

        // Skip instructions that have been removed
        if (!inst->getParent()) continue;

        // Branch simplification is handled separately because branches
        // are terminators (void-typed, no SSA users). The replacement
        // (erase old + insert new) is done inside simplifyBranch.
        if (inst->getOpcode() == Instruction::Opcode::Br) {
            if (simplifyBranch(inst)) {
                ++simplifiedCount;
            }
            continue;
        }

        // Try to simplify value-producing instructions
        Value *simplified = simplifyInstruction(inst);
        if (!simplified) continue;

        // Collect users of the original instruction before replacement.
        // After replaceAllUsesWith the original has no users, so we must
        // capture them here.
        auto users = inst->getUses();

        // Replace all uses of this instruction with the simplified value
        inst->replaceAllUsesWith(simplified);

        // Remove the now-dead instruction from its parent block
        eraseInstruction(inst);

        // Add the affected users to the worklist — they may now be
        // simplifiable with the replacement value as an operand.
        for (auto *u : users) {
            if (auto *userInst = dynamic_cast<Instruction*>(u)) {
                if (userInst->getParent()) {
                    worklist.push_back(userInst);
                }
            }
        }

        ++simplifiedCount;
    }

    return PreservedAnalyses::none();
}

} // namespace ll1
