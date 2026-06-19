#ifndef LL1_IR_FUNCTION_H
#define LL1_IR_FUNCTION_H

#include <ll1/IR/Value.h>
#include <vector>
#include <memory>
#include <string>

namespace ll1 {

class BasicBlock;

class Argument : public Value {
public:
    Argument(Type *ty, const std::string &name = "");
    static bool classof(const Value *v);
};

class Function : public Value {
public:
    using ArgList = std::vector<std::unique_ptr<Argument>>;
    using BBList = std::vector<std::unique_ptr<BasicBlock>>;

    Function(Type *returnTy, const std::string &name = "");
    ~Function();

    ArgList &getArgs() { return Arguments; }
    Argument *getArg(unsigned i) { return Arguments[i].get(); }
    unsigned getArgCount() const { return Arguments.size(); }
    void addArg(std::unique_ptr<Argument> arg);

    BBList &getBasicBlocks() { return Blocks; }
    BasicBlock *getEntryBlock();
    BasicBlock *createBasicBlock(const std::string &name = "");
    void addBasicBlock(std::unique_ptr<BasicBlock> bb);

    Type *getReturnType() const { return ReturnType; }
    static bool classof(const Value *v);

private:
    ArgList Arguments;
    BBList Blocks;
    Type *ReturnType;
};

} // namespace ll1
#endif
