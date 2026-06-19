#include <ll1/IR/Value.h>
#include <ll1/IR/User.h>
#include <algorithm>

namespace ll1 {

Value::Value(ValueKind k, Type *ty, const std::string &name)
    : Kind(k), Ty(ty), Name(name) {}

void Value::addUse(User *u) {
    Uses.push_back(u);
}

void Value::removeUse(User *u) {
    Uses.erase(std::remove(Uses.begin(), Uses.end(), u), Uses.end());
}

void Value::replaceAllUsesWith(Value *newVal) {
    auto usesCopy = Uses;
    for (auto *u : usesCopy) {
        for (unsigned i = 0; i < u->getNumOperands(); ++i) {
            if (u->getOperand(i) == this) {
                u->setOperand(i, newVal);
            }
        }
    }
}

} // namespace ll1
