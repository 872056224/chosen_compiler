#include <ll1/CodeGen/CodeGen.h>
#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/IRBuilder.h>
#include <ll1/IR/LLVMContext.h>
#include <iostream>
#include <cassert>

int main() {
    std::cout << "CodeGen tests:\n";

    // Build a simple IR module: int main() { return 42; }
    {
        ll1::LLVMContext ctx;
        ll1::Module mod("test");
        auto *fn = mod.createFunction(ctx.getInt16Ty(), "main");
        auto *entry = fn->createBasicBlock("entry");

        ll1::IRBuilder builder(ctx);
        builder.setInsertPoint(entry);
        auto *c42 = builder.getInt16(42);
        builder.CreateRet(c42);

        ll1::CodeGen cg;
        auto mm = cg.generate(mod);
        auto asmCode = cg.emitAssembly(mm);

        assert(!asmCode.empty());
        assert(asmCode.find("main:") != std::string::npos);
        assert(asmCode.find("ret") != std::string::npos);
        assert(asmCode.find("42") != std::string::npos);
        std::cout << "  simple return: OK\n";
        std::cout << asmCode << "\n";
    }

    // Build: int main() { int a = 10; int b = 20; return a + b; }
    {
        ll1::LLVMContext ctx;
        ll1::Module mod("test2");
        auto *fn = mod.createFunction(ctx.getInt16Ty(), "main");
        auto *entry = fn->createBasicBlock("entry");

        ll1::IRBuilder builder(ctx);
        builder.setInsertPoint(entry);
        auto *a = builder.CreateAlloca(ctx.getInt16Ty(), "a");
        builder.CreateStore(builder.getInt16(10), a);
        auto *b = builder.CreateAlloca(ctx.getInt16Ty(), "b");
        builder.CreateStore(builder.getInt16(20), b);
        auto *av = builder.CreateLoad(ctx.getInt16Ty(), a, "a_val");
        auto *bv = builder.CreateLoad(ctx.getInt16Ty(), b, "b_val");
        auto *sum = builder.CreateAdd(av, bv, "sum");
        builder.CreateRet(sum);

        ll1::CodeGen cg;
        auto mm = cg.generate(mod);
        auto asmCode = cg.emitAssembly(mm);

        assert(asmCode.find("add") != std::string::npos);
        std::cout << "  add vars: OK\n";
    }

    // Build: if/else
    {
        ll1::LLVMContext ctx;
        ll1::Module mod("test3");
        auto *fn = mod.createFunction(ctx.getInt16Ty(), "main");
        auto *entry = fn->createBasicBlock("entry");
        auto *thenBB = fn->createBasicBlock("then");
        auto *elseBB = fn->createBasicBlock("else");
        auto *mergeBB = fn->createBasicBlock("merge");

        ll1::IRBuilder builder(ctx);
        builder.setInsertPoint(entry);
        auto *c10 = builder.getInt16(10);
        auto *c20 = builder.getInt16(20);
        auto *cmp = builder.CreateICmpSLT(c10, c20, "cond");
        builder.CreateCondBr(cmp, thenBB, elseBB);

        builder.setInsertPoint(thenBB);
        builder.CreateRet(c10);

        builder.setInsertPoint(elseBB);
        builder.CreateRet(c20);

        // merge unused but valid

        ll1::CodeGen cg;
        auto mm = cg.generate(mod);
        auto asmCode = cg.emitAssembly(mm);

        assert(asmCode.find("jl") != std::string::npos || asmCode.find("jmp") != std::string::npos);
        std::cout << "  if/else: OK\n";
    }

    std::cout << "All CodeGen tests passed.\n";
    return 0;
}
