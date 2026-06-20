#ifndef LL1_CODEGEN_CODEGEN_H
#define LL1_CODEGEN_CODEGEN_H

#include <ll1/CodeGen/MCInst.h>
#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <string>
#include <map>
#include <sstream>

namespace ll1 {

class CodeGen {
public:
    CodeGen();

    // Generate machine code from IR module
    MachineModule generate(Module &mod);

    // Emit assembly text
    std::string emitAssembly(const MachineModule &mm);

private:
    // State per function
    struct FuncState {
        MachineFunction MF;
        std::map<Value*, std::string> VRegNames; // IR Value → physical reg (ax/bx/cx/dx)
        int StackOffset = 0;  // grows downward from bp
        std::map<Value*, int> AllocaOffsets; // alloca → stack offset
        std::map<BasicBlock*, std::string> BlockLabels;
    };

    FuncState State;

    void generateFunction(Function &fn);
    void generateBB(BasicBlock &bb);
    void generateInst(Instruction &inst);

    // Operand helpers
    std::string getOperand(Value *v);
    std::string assignReg(Value *v);
    std::string getReg(Value *v);
    std::string loadToReg(Value *v);

    // Emit helpers
    void emit(const std::string &opcode, const std::string &operands = "", const std::string &comment = "");

    // Output stream
    std::ostringstream Out;

    // 8086 registers for temp values
    static const char *TempRegs[];
    static const int NumTempRegs;
    int NextTempReg = 0;
    std::string allocTempReg();

    // Mem operand
    std::string memOp(const std::string &base, int offset);
    std::string memOpBP(int offset);
};

} // namespace ll1
#endif
