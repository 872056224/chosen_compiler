#include <ll1/IR/Value.h>
#include <ll1/IR/User.h>
#include <cassert>
#include <iostream>

// Minimal concrete User subclass for testing
class TestUser : public ll1::User {
public:
    TestUser(ll1::Value *op1, ll1::Value *op2)
        : User(ll1::Value::ValueKind::Instruction,
               ll1::Type::getInt16Ty()) {
        Operands.push_back(nullptr);
        Operands.push_back(nullptr);
        setOperand(0, op1);
        setOperand(1, op2);
    }
};

int main() {
    // Test: Value creation with name and type
    ll1::Value v(ll1::Value::ValueKind::Argument,
                 ll1::Type::getInt16Ty(), "x");
    assert(v.getName() == "x");
    assert(v.getType() == ll1::Type::getInt16Ty());
    assert(v.getNumUses() == 0);

    // Test: User tracks operands and use-lists
    ll1::Value v2(ll1::Value::ValueKind::Argument,
                  ll1::Type::getInt16Ty(), "y");
    TestUser u(&v, &v2);

    assert(u.getOperand(0) == &v);
    assert(u.getOperand(1) == &v2);
    assert(v.getNumUses() == 1);
    assert(v2.getNumUses() == 1);

    // Test: RAUW (replace all uses with)
    ll1::Value v3(ll1::Value::ValueKind::Argument,
                  ll1::Type::getInt16Ty(), "z");
    v.replaceAllUsesWith(&v3);

    assert(v.getNumUses() == 0);
    assert(u.getOperand(0) == &v3);
    assert(v3.getNumUses() == 1);

    std::cout << "Value tests passed.\n";
    return 0;
}
