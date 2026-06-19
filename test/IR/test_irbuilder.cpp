#include <ll1/IR/IRBuilder.h>
#include <ll1/IR/Module.h>
#include <cassert>
#include <iostream>

int main() {
    ll1::LLVMContext ctx;
    ll1::IRBuilder builder(ctx);

    ll1::Module mod("test");
    auto *fn = mod.createFunction(ll1::Type::getInt16Ty(), "main");
    auto *entry = fn->createBasicBlock("entry");
    builder.setInsertPoint(entry);

    // Test alloca
    auto *allocaInst = builder.CreateAlloca(ctx.getInt16Ty(), "x");
    assert(allocaInst != nullptr);
    assert(allocaInst->getAllocatedType() == ctx.getInt16Ty());

    // Test constant
    auto *c10 = builder.getInt16(10);
    assert(c10->getValue() == 10);
    assert(c10->getType() == ctx.getInt16Ty());

    // Test store
    auto *store = builder.CreateStore(c10, allocaInst);
    assert(store != nullptr);

    // Test load
    auto *load = builder.CreateLoad(ctx.getInt16Ty(), allocaInst, "x_val");
    assert(load != nullptr);

    // Test add
    auto *c20 = builder.getInt16(20);
    auto *add = builder.CreateAdd(load, c20, "sum");
    assert(add != nullptr);
    assert(add->getOpcode() == ll1::Instruction::Opcode::Add);

    // Test icmp
    auto *cmp = builder.CreateICmpSLT(load, c20, "cond");
    assert(cmp != nullptr);
    assert(cmp->getType() == ctx.getInt1Ty());

    // Test cond br
    auto *trueBB = fn->createBasicBlock("true");
    auto *falseBB = fn->createBasicBlock("false");
    auto *br = builder.CreateCondBr(cmp, trueBB, falseBB);
    assert(br->isConditional());

    // Verify entry block contents
    assert(entry->size() == 6); // alloca, store, load, add, icmp, br

    // Test ret in true block
    builder.setInsertPoint(trueBB);
    auto *retInst = builder.CreateRet(c10);
    assert(retInst->getReturnValue() == c10);

    std::cout << "IRBuilder tests passed.\n";
    return 0;
}
