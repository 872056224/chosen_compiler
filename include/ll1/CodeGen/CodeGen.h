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
#include <memory>

namespace ll1 {

class TargetMachine;
class Target8086MCInstPrinter;
class MachineFunction;  // forward (new MachineFunction from MachineFunction.h)

class CodeGen {
public:
    CodeGen();

    // New pipeline: takes a TargetMachine for SDAG→ISel→RA→Print
    explicit CodeGen(TargetMachine &TM);
    ~CodeGen();

    // Generate machine code from IR module (old pipeline — backward compat)
    LegacyMachineModule generate(Module &mod);

    // Generate assembly using the new pipeline (SDAG → ISel → RegAlloc → Print)
    std::string generateNew(Module &mod);

    // Emit assembly text
    std::string emitAssembly(const LegacyMachineModule &mm);

private:
    // State per function
    struct FuncState {
        LegacyMachineFunction MF;
        std::map<Value*, std::string> VRegNames;  // IR Value → physical reg
        std::map<Value*, int> VarOffsets;          // Alloca → BX offset
        std::map<Value*, int> ArgOffsets;          // Argument → BP offset
        int NextVarOffset = 0;
        std::map<BasicBlock*, std::string> BlockLabels;

        // Linear scan state
        std::map<Value*, int> LastUsePos;          // Value → instruction position of last use
        std::map<Value*, int> StackSlots;          // Spilled value → [bx+offset]
        int SpillSlotOffset = 100;                  // Offset for spill slots (above normal vars)
        int CurrentPos = 0;                         // Current instruction position
    };

    FuncState State;

    void generateFunction(Function &fn);
    void generateBB(BasicBlock &bb);
    void generateInst(Instruction &inst);

    // Liveness pre-scan
    void preScanLiveness(Function &fn);

    // Operand helpers
    std::string getOperand(Value *v);
    std::string assignReg(Value *v);
    std::string getReg(Value *v);
    std::string loadToReg(Value *v);
    std::string varLabel(Value *v);
    int getArrayBaseOffset(Value *base);

    // Linear scan register allocation
    std::string allocRegLinear(Value *v);
    std::string allocTempReg();
    void freeRegsIfDone();
    std::string spillReg(const std::string &reg);
    std::string reloadSpilled(Value *v);
    int getSpillSlot(Value *v);

    // Emit helpers
    void emit(const std::string &opcode, const std::string &operands = "", const std::string &comment = "");

    // Output stream
    std::ostringstream Out;

    // Register pool
    std::vector<std::string> FreeRegs;
    struct RegOccupancy { Value *val = nullptr; int lastUse = -1; };
    std::map<std::string, RegOccupancy> OccupiedRegs;

    // Mem operand
    std::string memOp(const std::string &base, int offset);
    std::string memOpBP(int offset);

    // ============================================================
    // New pipeline members
    // ============================================================
    TargetMachine *TM = nullptr;
    std::unique_ptr<Target8086MCInstPrinter> Printer;
    bool UseNewPipeline = false;

    std::string generateNewImpl(Module &mod);
};

} // namespace ll1
#endif
