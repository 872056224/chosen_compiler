#include <ll1/IR/User.h>

namespace ll1 {

Value *User::getOperand(unsigned i) const {
    return Operands.at(i);
}

void User::setOperand(unsigned i, Value *v) {
    if (Operands[i]) {
        Operands[i]->removeUse(this);
    }
    Operands[i] = v;
    if (v) {
        v->addUse(this);
    }
}

void User::addOperand(Value *v) {
    unsigned idx = Operands.size();
    Operands.push_back(nullptr);
    setOperand(idx, v);
}

} // namespace ll1
