#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <cassert>
#include <iostream>

int main() {
    ll1::BasicBlock bb("entry");
    assert(bb.getName() == "entry");
    assert(bb.empty());
    assert(bb.getTerminator() == nullptr);

    bb.pushBack(std::make_unique<ll1::RetInst>());
    assert(bb.size() == 1);
    assert(bb.getTerminator() != nullptr);
    assert(bb.getTerminator()->getOpcode() == ll1::Instruction::Opcode::Ret);

    std::cout << "BasicBlock tests passed.\n";
    return 0;
}
