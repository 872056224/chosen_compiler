#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Function.h>

namespace ll1 {

Instruction::Instruction(Opcode op, Type *ty, const std::string &name)
    : User(ValueKind::Instruction, ty, name), Op(op) {}

bool Instruction::classof(const Value *v) {
    return v->getKind() == ValueKind::Instruction;
}

// RetInst
RetInst::RetInst(Value *retVal)
    : Instruction(Opcode::Ret, Type::getVoidTy()) {
    if (retVal) {
        Operands.push_back(nullptr);
        setOperand(0, retVal);
    }
}
Value *RetInst::getReturnValue() const {
    return Operands.empty() ? nullptr : getOperand(0);
}
bool RetInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Ret;
}

// BranchInst
BranchInst::BranchInst(BasicBlock *dest)
    : Instruction(Opcode::Br, Type::getVoidTy()) {
    Operands.push_back(dest);
}
BranchInst::BranchInst(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB)
    : Instruction(Opcode::Br, Type::getVoidTy()) {
    Operands.push_back(nullptr);
    Operands.push_back(trueBB);
    Operands.push_back(falseBB);
    setOperand(0, cond);
}
bool BranchInst::isConditional() const { return Operands.size() == 3; }
bool BranchInst::isUnconditional() const { return Operands.size() == 1; }
Value *BranchInst::getCondition() const { return isConditional() ? getOperand(0) : nullptr; }
BasicBlock *BranchInst::getTrueDest() const {
    return isConditional() ? static_cast<BasicBlock*>(getOperand(1)) : nullptr;
}
BasicBlock *BranchInst::getFalseDest() const {
    return isConditional() ? static_cast<BasicBlock*>(getOperand(2)) : nullptr;
}
BasicBlock *BranchInst::getUnconditionalDest() const {
    return isUnconditional() ? static_cast<BasicBlock*>(getOperand(0)) : nullptr;
}
bool BranchInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Br;
}

// BinaryOpInst
BinaryOpInst::BinaryOpInst(Opcode op, Value *lhs, Value *rhs, const std::string &name)
    : Instruction(op, lhs->getType(), name) {
    Operands.push_back(nullptr);
    Operands.push_back(nullptr);
    setOperand(0, lhs);
    setOperand(1, rhs);
}
bool BinaryOpInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() >= Opcode::Add && I->getOpcode() <= Opcode::SRem;
}

// ICmpInst
ICmpInst::ICmpInst(Pred p, Value *lhs, Value *rhs, const std::string &name)
    : Instruction(Opcode::ICmp, Type::getInt1Ty(), name), P(p) {
    Operands.push_back(nullptr);
    Operands.push_back(nullptr);
    setOperand(0, lhs);
    setOperand(1, rhs);
}
bool ICmpInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::ICmp;
}

// AllocaInst
AllocaInst::AllocaInst(Type *allocatedType, const std::string &name)
    : Instruction(Opcode::Alloca, allocatedType, name), AllocatedTy(allocatedType) {}
bool AllocaInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Alloca;
}

// LoadInst
LoadInst::LoadInst(Type *ty, Value *ptr, const std::string &name)
    : Instruction(Opcode::Load, ty, name) {
    Operands.push_back(nullptr);
    setOperand(0, ptr);
}
bool LoadInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Load;
}

// StoreInst
StoreInst::StoreInst(Value *val, Value *ptr)
    : Instruction(Opcode::Store, Type::getVoidTy()) {
    Operands.push_back(nullptr);
    Operands.push_back(nullptr);
    setOperand(0, val);
    setOperand(1, ptr);
}
bool StoreInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Store;
}

// CallInst
CallInst::CallInst(Function *callee, const std::string &name)
    : Instruction(Opcode::Call, Type::getVoidTy(), name) {
    Operands.push_back(callee);
}
bool CallInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Call;
}

// PhiInst
PhiInst::PhiInst(Type *ty, const std::string &name)
    : Instruction(Opcode::Phi, ty, name) {}
void PhiInst::addIncoming(Value *v, BasicBlock *bb) {
    unsigned idx = Operands.size();
    Operands.push_back(nullptr);
    setOperand(idx, v);
    IncomingBlocks.push_back(bb);
}
bool PhiInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Phi;
}

// ConstantInt
ConstantInt *ConstantInt::get(Type *ty, int16_t val) {
    return new ConstantInt(ty, val);
}
bool ConstantInt::classof(const Value *v) {
    return v->getKind() == ValueKind::Constant;
}

} // namespace ll1
