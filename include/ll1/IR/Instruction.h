#ifndef LL1_IR_INSTRUCTION_H
#define LL1_IR_INSTRUCTION_H

#include <ll1/IR/User.h>
#include <cstdint>
#include <vector>

namespace ll1 {

class BasicBlock;
class Function;

class Instruction : public User {
public:
    enum class Opcode {
        Ret, Br,
        Add, Sub, Mul, SDiv, SRem,
        And, Or, Xor,
        ICmp,
        Alloca, Load, Store,
        Call, Phi,
        SExt, ZExt, Trunc,
    };

    Instruction(Opcode op, Type *ty, const std::string &name = "");

    Opcode getOpcode() const { return Op; }
    BasicBlock *getParent() const { return Parent; }
    void setParent(BasicBlock *bb) { Parent = bb; }

    static bool classof(const Value *v);

private:
    Opcode Op;
    BasicBlock *Parent = nullptr;
};

// Concrete instructions
class RetInst : public Instruction {
public:
    RetInst(Value *retVal = nullptr);
    Value *getReturnValue() const;
    static bool classof(const Value *v);
};

class BranchInst : public Instruction {
public:
    BranchInst(BasicBlock *dest);
    BranchInst(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB);

    bool isConditional() const;
    bool isUnconditional() const;
    Value *getCondition() const;
    BasicBlock *getTrueDest() const;
    BasicBlock *getFalseDest() const;
    BasicBlock *getUnconditionalDest() const;
    static bool classof(const Value *v);
};

class BinaryOpInst : public Instruction {
public:
    BinaryOpInst(Opcode op, Value *lhs, Value *rhs, const std::string &name = "");
    Value *getLHS() const { return getOperand(0); }
    Value *getRHS() const { return getOperand(1); }
    static bool classof(const Value *v);
};

class ICmpInst : public Instruction {
public:
    enum Pred { EQ, NE, SLT, SLE, SGT, SGE };
    ICmpInst(Pred p, Value *lhs, Value *rhs, const std::string &name = "");
    Pred getPredicate() const { return P; }
    static bool classof(const Value *v);
private:
    Pred P;
};

class AllocaInst : public Instruction {
public:
    AllocaInst(Type *allocatedType, const std::string &name = "");
    Type *getAllocatedType() const { return AllocatedTy; }
    static bool classof(const Value *v);
private:
    Type *AllocatedTy;
};

class LoadInst : public Instruction {
public:
    LoadInst(Type *ty, Value *ptr, const std::string &name = "");
    Value *getPointer() const { return getOperand(0); }
    static bool classof(const Value *v);
};

class StoreInst : public Instruction {
public:
    StoreInst(Value *val, Value *ptr);
    Value *getValue() const { return getOperand(0); }
    Value *getPointer() const { return getOperand(1); }
    static bool classof(const Value *v);
};

class CallInst : public Instruction {
public:
    CallInst(Function *callee, const std::string &name = "");
    static bool classof(const Value *v);
};

class PhiInst : public Instruction {
public:
    PhiInst(Type *ty, const std::string &name = "");
    void addIncoming(Value *v, BasicBlock *bb);
    unsigned getNumIncoming() const { return IncomingBlocks.size(); }
    Value *getIncomingValue(unsigned i) const { return getOperand(i); }
    BasicBlock *getIncomingBlock(unsigned i) const { return IncomingBlocks[i]; }
    static bool classof(const Value *v);
private:
    std::vector<BasicBlock *> IncomingBlocks;
};

class ConstantInt : public Value {
public:
    ConstantInt(Type *ty, int16_t val) : Value(ValueKind::Constant, ty), Val(val) {}
    int16_t getValue() const { return Val; }
    static ConstantInt *get(Type *ty, int16_t val);
    static bool classof(const Value *v);
private:
    int16_t Val;
};

} // namespace ll1
#endif
