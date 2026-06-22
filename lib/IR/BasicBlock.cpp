#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <ll1/IR/Function.h>

namespace ll1 {

BasicBlock::BasicBlock(const std::string &name)
    : Value(ValueKind::BasicBlock, Type::getVoidTy(), name) {}

BasicBlock::~BasicBlock() = default;

Instruction *BasicBlock::getTerminator() {
    if (Instructions.empty()) return nullptr;
    auto *last = Instructions.back().get();
    auto op = last->getOpcode();
    if (op == Instruction::Opcode::Ret || op == Instruction::Opcode::Br) {
        return last;
    }
    return nullptr;
}

const Instruction *BasicBlock::getTerminator() const {
    return const_cast<BasicBlock*>(this)->getTerminator();
}

void BasicBlock::pushBack(std::unique_ptr<Instruction> inst) {
    inst->setParent(this);
    Instructions.push_back(std::move(inst));
}

void BasicBlock::pushFront(std::unique_ptr<Instruction> inst) {
    inst->setParent(this);
    Instructions.push_front(std::move(inst));
}

std::vector<BasicBlock*> BasicBlock::getPredecessors() const {
    std::vector<BasicBlock*> preds;
    // Scan all BBs in the parent function for terminators that reference this BB
    auto *fn = getParent();
    if (!fn) return preds;
    for (auto &otherBB : fn->getBasicBlocks()) {
        if (otherBB.get() == this) continue;
        auto *term = otherBB->getTerminator();
        if (!term) continue;
        if (auto *br = dynamic_cast<BranchInst*>(term)) {
            bool referencesThis = false;
            for (unsigned i = 0; i < br->getNumOperands(); ++i) {
                if (br->getOperand(i) == this) { referencesThis = true; break; }
            }
            if (referencesThis) preds.push_back(otherBB.get());
        }
    }
    return preds;
}

std::vector<BasicBlock*> BasicBlock::getSuccessors() const {
    std::vector<BasicBlock*> succs;
    auto *term = getTerminator();
    if (!term) return succs;

    if (auto *br = dynamic_cast<const BranchInst*>(term)) {
        if (br->isConditional()) {
            succs.push_back(br->getTrueDest());
            succs.push_back(br->getFalseDest());
        } else if (br->isUnconditional()) {
            succs.push_back(br->getUnconditionalDest());
        }
    }
    return succs;
}

bool BasicBlock::classof(const Value *v) {
    return v->getKind() == ValueKind::BasicBlock;
}

} // namespace ll1
