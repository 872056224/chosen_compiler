#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Module.h>
#include <ll1/IR/Instruction.h>
#include <cassert>
#include <iostream>

int main() {
    ll1::Module mod("test");
    assert(mod.getModuleName() == "test");

    auto *fn = mod.createFunction(ll1::Type::getInt16Ty(), "main");
    assert(fn->getName() == "main");
    assert(fn->getReturnType() == ll1::Type::getInt16Ty());

    fn->addArg(std::make_unique<ll1::Argument>(ll1::Type::getInt16Ty(), "argc"));
    assert(fn->getArgCount() == 1);
    assert(fn->getArg(0)->getName() == "argc");

    auto *entry = fn->createBasicBlock("entry");
    assert(fn->getEntryBlock() == entry);
    assert(fn->getBasicBlocks().size() == 1);

    entry->pushBack(std::make_unique<ll1::RetInst>());
    assert(entry->getTerminator() != nullptr);

    assert(mod.getFunction("main") == fn);
    assert(mod.getFunction("nonexistent") == nullptr);

    std::cout << "Function/Module tests passed.\n";
    return 0;
}
