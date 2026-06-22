#ifndef LL1_CODEGEN_SELECTIONDAG_SDBUILDER_H
#define LL1_CODEGEN_SELECTIONDAG_SDBUILDER_H

#include <ll1/CodeGen/SelectionDAG/SelectionDAG.h>
#include <ll1/CodeGen/SelectionDAG/SDNode.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <unordered_map>

namespace ll1 {

class TargetLowering;
class TargetRegisterInfo;

// ============================================================
// SelectionDAGBuilder — Converts LLVM IR to SelectionDAG nodes
//
// One builder per function. Visits each BB and each instruction,
// creating SDNodes and tracking the chain token.
// ============================================================
class SelectionDAGBuilder {
public:
    SelectionDAGBuilder(SelectionDAG &dag, const TargetRegisterInfo &tri,
                        TargetLowering &tli);

    void visit(Function &F);

private:
    SelectionDAG &DAG;
    const TargetRegisterInfo &TRI;
    TargetLowering &TLI;

    // Map: IR Value → SDValue (the DAG node for this value)
    std::unordered_map<Value*, SDValue> ValueMap;

    // Map: IR Value → virtual register (for values that need registers)
    std::unordered_map<Value*, Register> VRegMap;

    // Map: BasicBlock → MachineBasicBlock
    std::unordered_map<BasicBlock*, MachineBasicBlock*> BBMap;

    // Current chain token (controls memory / side-effect ordering)
    SDValue Chain;

    // Pending phi copies: {DestVReg, SrcValue, PredBB}
    struct PendingCopy {
        Register DestReg;
        Value *SrcVal;
        BasicBlock *PredBB;
    };
    std::vector<PendingCopy> PendingCopies;

    // =========================================================
    // Visitor methods
    // =========================================================
    void visitBB(BasicBlock &BB);
    void visitInst(Instruction &I);

    // Individual instruction visitors
    void visitRet(RetInst &I);
    void visitBr(BranchInst &I);
    void visitAdd(BinaryOpInst &I);
    void visitSub(BinaryOpInst &I);
    void visitMul(BinaryOpInst &I);
    void visitSDiv(BinaryOpInst &I);
    void visitSRem(BinaryOpInst &I);
    void visitAnd(BinaryOpInst &I);
    void visitOr(BinaryOpInst &I);
    void visitXor(BinaryOpInst &I);
    void visitICmp(ICmpInst &I);
    void visitAlloca(AllocaInst &I);
    void visitLoad(LoadInst &I);
    void visitStore(StoreInst &I);
    void visitCall(CallInst &I);
    void visitSExt(Instruction &I);
    void visitZExt(Instruction &I);
    void visitTrunc(Instruction &I);
    void visitPhi(PhiInst &I);
    void visitArrayLoad(ArrayLoadInst &I);
    void visitArrayStore(ArrayStoreInst &I);

    // =========================================================
    // Helpers
    // =========================================================
    // Get or create SDValue for an IR Value
    SDValue getValue(Value *V);

    // Get or create a virtual register for an IR Value
    Register getVReg(Value *V);

    // Map an IR BB to a Machine BB
    MachineBasicBlock* getMBB(BasicBlock *BB);

    // Set the SDValue for an IR Value in the value map
    void setValue(Value *V, SDValue Val);

    // Assign a virtual register to an IR Value
    Register assignVReg(Value *V, Type *Ty);

    // Lower phi nodes: insert COPY instructions for pending copies
    void lowerPendingPhis();
};

} // namespace ll1
#endif
