#include <ll1/IR/IRBuilder.h>

namespace ll1 {

IRBuilder::IRBuilder(LLVMContext &ctx) : Context(ctx) {}

void IRBuilder::setInsertPoint(BasicBlock *bb) { CurrentBB = bb; }

template<typename T, typename... Args>
static T *insert(IRBuilder *B, Args&&... args) {
    auto inst = std::make_unique<T>(std::forward<Args>(args)...);
    auto *raw = inst.get();
    if (B->getInsertBlock()) {
        B->getInsertBlock()->pushBack(std::move(inst));
    }
    return raw;
}

ConstantInt *IRBuilder::getInt16(int16_t val) {
    return ConstantInt::get(Context.getInt16Ty(), val);
}

BinaryOpInst *IRBuilder::CreateAdd(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Add, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateSub(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Sub, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateMul(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Mul, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateSDiv(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::SDiv, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateSRem(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::SRem, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateAnd(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::And, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateOr(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Or, lhs, rhs, name);
}

Value *IRBuilder::CreateNeg(Value *v, const std::string &name) {
    auto *zero = getInt16(0);
    return CreateSub(zero, v, name);
}

Value *IRBuilder::CreateNot(Value *v, const std::string &name) {
    // !x = (x == 0)
    auto *zero = getInt16(0);
    return CreateICmpEQ(v, zero, name);
}

ICmpInst *IRBuilder::CreateICmpEQ(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::EQ, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpNE(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::NE, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSLT(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SLT, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSLE(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SLE, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSGT(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SGT, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSGE(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SGE, lhs, rhs, name);
}

AllocaInst *IRBuilder::CreateAlloca(Type *ty, const std::string &name) {
    return insert<AllocaInst>(this, ty, name);
}
LoadInst *IRBuilder::CreateLoad(Type *ty, Value *ptr, const std::string &name) {
    return insert<LoadInst>(this, ty, ptr, name);
}
StoreInst *IRBuilder::CreateStore(Value *val, Value *ptr) {
    return insert<StoreInst>(this, val, ptr);
}

RetInst *IRBuilder::CreateRetVoid() { return insert<RetInst>(this, nullptr); }
RetInst *IRBuilder::CreateRet(Value *v) { return insert<RetInst>(this, v); }
BranchInst *IRBuilder::CreateBr(BasicBlock *dest) { return insert<BranchInst>(this, dest); }
BranchInst *IRBuilder::CreateCondBr(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB) {
    return insert<BranchInst>(this, cond, trueBB, falseBB);
}
PhiInst *IRBuilder::CreatePhi(Type *ty, const std::string &name) {
    return insert<PhiInst>(this, ty, name);
}

Value *IRBuilder::CreateSExt(Value *v, Type *destTy, const std::string &name) {
    return insert<Instruction>(this, Instruction::Opcode::SExt, destTy, name);
}
Value *IRBuilder::CreateZExt(Value *v, Type *destTy, const std::string &name) {
    return insert<Instruction>(this, Instruction::Opcode::ZExt, destTy, name);
}
Value *IRBuilder::CreateTrunc(Value *v, Type *destTy, const std::string &name) {
    return insert<Instruction>(this, Instruction::Opcode::Trunc, destTy, name);
}

} // namespace ll1
