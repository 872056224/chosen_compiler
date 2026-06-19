#ifndef LL1_IR_VALUE_H
#define LL1_IR_VALUE_H

#include <ll1/IR/Type.h>
#include <string>
#include <vector>

namespace ll1 {

class User;

class Value {
public:
    enum class ValueKind {
        Argument, BasicBlock, Function, Module,
        Instruction, Constant, GlobalVariable
    };

    Value(ValueKind k, Type *ty, const std::string &name = "");
    virtual ~Value() = default;

    ValueKind getKind() const { return Kind; }
    Type *getType() const { return Ty; }

    const std::string &getName() const { return Name; }
    void setName(const std::string &name) { Name = name; }

    void addUse(User *u);
    void removeUse(User *u);
    const std::vector<User *> &getUses() const { return Uses; }
    unsigned getNumUses() const { return Uses.size(); }

    void replaceAllUsesWith(Value *newVal);

private:
    ValueKind Kind;
    Type *Ty;
    std::string Name;
    std::vector<User *> Uses;
};

} // namespace ll1

#endif
