#ifndef LL1_IR_BASICBLOCK_H
#define LL1_IR_BASICBLOCK_H

#include <ll1/IR/Value.h>
#include <list>
#include <memory>
#include <string>

namespace ll1 {

class Instruction;
class Function;

class BasicBlock : public Value {
public:
    using InstList = std::list<std::unique_ptr<Instruction>>;
    using iterator = InstList::iterator;
    using const_iterator = InstList::const_iterator;

    explicit BasicBlock(const std::string &name = "");
    ~BasicBlock();

    InstList &getInstList() { return Instructions; }
    Instruction *getTerminator();
    const Instruction *getTerminator() const;

    void pushBack(std::unique_ptr<Instruction> inst);
    void pushFront(std::unique_ptr<Instruction> inst);

    iterator begin() { return Instructions.begin(); }
    iterator end() { return Instructions.end(); }
    const_iterator begin() const { return Instructions.begin(); }
    const_iterator end() const { return Instructions.end(); }

    bool empty() const { return Instructions.empty(); }
    size_t size() const { return Instructions.size(); }

    Function *getParent() const { return Parent; }
    void setParent(Function *f) { Parent = f; }

    // Get predecessor blocks (computed from use-def chain)
    std::vector<BasicBlock*> getPredecessors() const;

    // Get successor blocks (from terminator)
    std::vector<BasicBlock*> getSuccessors() const;

    static bool classof(const Value *v);

private:
    InstList Instructions;
    Function *Parent = nullptr;
};

} // namespace ll1
#endif
