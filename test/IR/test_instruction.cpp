#include <ll1/IR/Instruction.h>
#include <cassert>
#include <iostream>

int main() {
    // BinaryOpInst
    ll1::Value lhs(ll1::Value::ValueKind::Argument, ll1::Type::getInt16Ty(), "a");
    ll1::Value rhs(ll1::Value::ValueKind::Argument, ll1::Type::getInt16Ty(), "b");
    ll1::BinaryOpInst add(ll1::Instruction::Opcode::Add, &lhs, &rhs, "tmp");
    assert(add.getOpcode() == ll1::Instruction::Opcode::Add);
    assert(add.getLHS() == &lhs);
    assert(add.getRHS() == &rhs);
    assert(add.getType() == ll1::Type::getInt16Ty());

    // RetInst void
    ll1::RetInst retVoid;
    assert(retVoid.getReturnValue() == nullptr);

    // RetInst with value
    ll1::RetInst retVal(&lhs);
    assert(retVal.getReturnValue() == &lhs);

    // AllocaInst
    ll1::AllocaInst allocaInst(ll1::Type::getInt16Ty(), "x");
    assert(allocaInst.getAllocatedType() == ll1::Type::getInt16Ty());

    // Store + Load
    ll1::StoreInst store(&rhs, &allocaInst);
    assert(store.getValue() == &rhs);
    assert(store.getPointer() == &allocaInst);
    ll1::LoadInst load(ll1::Type::getInt16Ty(), &allocaInst, "loaded");
    assert(load.getPointer() == &allocaInst);
    assert(load.getType() == ll1::Type::getInt16Ty());

    // ICmpInst
    ll1::ICmpInst cmp(ll1::ICmpInst::SLT, &lhs, &rhs, "cond");
    assert(cmp.getPredicate() == ll1::ICmpInst::SLT);
    assert(cmp.getType() == ll1::Type::getInt1Ty());

    std::cout << "Instruction tests passed.\n";
    return 0;
}
