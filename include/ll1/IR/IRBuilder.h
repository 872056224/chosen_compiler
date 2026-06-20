#ifndef LL1_IR_IRBUILDER_H
#define LL1_IR_IRBUILDER_H

#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/LLVMContext.h>
#include <memory>
#include <string>

namespace ll1 {

class IRBuilder {
public:
    explicit IRBuilder(LLVMContext &ctx);

    void setInsertPoint(BasicBlock *bb);
    BasicBlock *getInsertBlock() const { return CurrentBB; }

    // Arithmetic
    BinaryOpInst *CreateAdd(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateSub(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateMul(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateSDiv(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateSRem(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateAnd(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateOr(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateNeg(Value *v, const std::string &name = "");
    Value *CreateNot(Value *v, const std::string &name = "");

    // Compare
    ICmpInst *CreateICmpEQ(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpNE(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSLT(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSLE(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSGT(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSGE(Value *lhs, Value *rhs, const std::string &name = "");

    // Memory
    AllocaInst *CreateAlloca(Type *ty, const std::string &name = "");
    LoadInst *CreateLoad(Type *ty, Value *ptr, const std::string &name = "");
    StoreInst *CreateStore(Value *val, Value *ptr);

    // Array access
    ArrayLoadInst *CreateArrayLoad(Type *ty, Value *base, Value *index, const std::string &name = "");
    ArrayStoreInst *CreateArrayStore(Value *val, Value *base, Value *index);

    // Control flow
    RetInst *CreateRetVoid();
    RetInst *CreateRet(Value *v);
    BranchInst *CreateBr(BasicBlock *dest);
    BranchInst *CreateCondBr(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB);

    // Phi
    PhiInst *CreatePhi(Type *ty, const std::string &name = "");

    // Call
    CallInst *CreateCall(Function *callee, const std::vector<Value*> &args, const std::string &name = "");

    // Cast
    Value *CreateSExt(Value *v, Type *destTy, const std::string &name = "");
    Value *CreateZExt(Value *v, Type *destTy, const std::string &name = "");
    Value *CreateTrunc(Value *v, Type *destTy, const std::string &name = "");

    LLVMContext &getContext() { return Context; }

    // Create constant
    ConstantInt *getInt16(int16_t val);

private:
    LLVMContext &Context;
    BasicBlock *CurrentBB = nullptr;
};

} // namespace ll1
#endif
