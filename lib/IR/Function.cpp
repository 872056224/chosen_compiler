#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>

namespace ll1 {

Argument::Argument(Type *ty, const std::string &name)
    : Value(ValueKind::Argument, ty, name) {}
bool Argument::classof(const Value *v) { return v->getKind() == ValueKind::Argument; }

Function::Function(Type *returnTy, const std::string &name)
    : Value(ValueKind::Function, returnTy, name), ReturnType(returnTy) {}
Function::~Function() = default;

void Function::addArg(std::unique_ptr<Argument> arg) {
    Arguments.push_back(std::move(arg));
}

BasicBlock *Function::getEntryBlock() {
    return Blocks.empty() ? nullptr : Blocks.front().get();
}

BasicBlock *Function::createBasicBlock(const std::string &name) {
    auto bb = std::make_unique<BasicBlock>(name);
    auto *raw = bb.get();
    raw->setParent(this);
    Blocks.push_back(std::move(bb));
    return raw;
}

void Function::addBasicBlock(std::unique_ptr<BasicBlock> bb) {
    bb->setParent(this);
    Blocks.push_back(std::move(bb));
}

bool Function::classof(const Value *v) {
    return v->getKind() == ValueKind::Function;
}

} // namespace ll1
