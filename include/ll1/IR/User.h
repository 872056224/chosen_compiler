#ifndef LL1_IR_USER_H
#define LL1_IR_USER_H

#include <ll1/IR/Value.h>
#include <vector>

namespace ll1 {

class User : public Value {
public:
    User(ValueKind k, Type *ty, const std::string &name = "")
        : Value(k, ty, name) {}

    unsigned getNumOperands() const { return Operands.size(); }
    Value *getOperand(unsigned i) const;
    void setOperand(unsigned i, Value *v);
    void addOperand(Value *v);

protected:
    std::vector<Value *> Operands;
};

} // namespace ll1

#endif
