#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>

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

bool BasicBlock::classof(const Value *v) {
    return v->getKind() == ValueKind::BasicBlock;
}

} // namespace ll1
