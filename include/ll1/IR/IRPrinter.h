#ifndef LL1_IR_IRPRINTER_H
#define LL1_IR_IRPRINTER_H

#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <string>
#include <map>

namespace ll1 {

// LLVM IR text printer — outputs .ll format
class IRPrinter {
public:
    // Print entire module to LLVM IR text
    static std::string print(Module &mod);

    // State for value numbering (unnamed values get %0, %1, ...)
    struct PrintState {
        int NextID = 0;
        std::map<const Value*, int> IDMap;
        std::string getName(const Value *v);
    };

    // Print individual function (used by --dump-pass-ir)
    static std::string printFunction(Function &fn, PrintState &s);

private:
    static std::string printModule(Module &mod, PrintState &s);
    static std::string printBasicBlock(BasicBlock &bb, PrintState &s);
    static std::string printInstruction(const Instruction &inst, PrintState &s);
    static std::string printOperand(const Value *v, PrintState &s);
    static std::string printType(const Type *t);
};

} // namespace ll1
#endif
