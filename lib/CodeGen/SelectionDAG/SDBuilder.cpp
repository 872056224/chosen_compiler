#include <ll1/CodeGen/SelectionDAG/SDBuilder.h>
#include <ll1/CodeGen/Target/TargetLowering.h>
#include <ll1/CodeGen/Target/TargetRegisterInfo.h>
#include <ll1/CodeGen/Target8086/Target8086RegisterInfo.h>
#include <cassert>
#include <iostream>

namespace ll1 {

SelectionDAGBuilder::SelectionDAGBuilder(SelectionDAG &dag,
                                          const TargetRegisterInfo &tri,
                                          TargetLowering &tli)
    : DAG(dag), TRI(tri), TLI(tli) {}

// ============================================================
// visit(Function) — main entry
// ============================================================
void SelectionDAGBuilder::visit(Function &F) {
    // Get the root chain
    Chain = DAG.getEntryToken();

    // Create MachineBasicBlocks for each IR BasicBlock (globally unique labels)
    unsigned labelId = 0;
    for (auto &bb : F.getBasicBlocks()) {
        std::string uniqueName = F.getName() + "_" + bb->getName() + "_" + std::to_string(labelId++);
        auto *mbb = DAG.getMachineFunction().createMachineBasicBlock(uniqueName);
        mbb->setIRBasicBlock(bb.get());
        BBMap[bb.get()] = mbb;
    }

    // Assign virtual registers for function arguments
    for (auto &arg : F.getArgs()) {
        Register vreg = assignVReg(arg.get(), arg->getType());
        // Create CopyFromReg for live-in argument
        SDValue copyFromReg = DAG.getCopyFromReg(Chain, vreg, arg->getType());
        setValue(arg.get(), copyFromReg);
    }

    // Visit basic blocks in order
    for (auto &bb : F.getBasicBlocks()) {
        visitBB(*bb);
    }

    // Lower pending phi copies
    lowerPendingPhis();
}

// ============================================================
// visitBB
// ============================================================
void SelectionDAGBuilder::visitBB(BasicBlock &BB) {
    auto *MBB = getMBB(&BB);
    (void)MBB;

    for (auto &inst : BB.getInstList()) {
        visitInst(*inst);
    }
}

// ============================================================
// visitInst — dispatch
// ============================================================
void SelectionDAGBuilder::visitInst(Instruction &I) {
    switch (I.getOpcode()) {
    case Instruction::Opcode::Ret:    visitRet(static_cast<RetInst&>(I)); break;
    case Instruction::Opcode::Br:     visitBr(static_cast<BranchInst&>(I)); break;
    case Instruction::Opcode::Add:    visitAdd(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::Sub:    visitSub(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::Mul:    visitMul(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::SDiv:   visitSDiv(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::SRem:   visitSRem(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::And:    visitAnd(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::Or:     visitOr(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::Xor:    visitXor(static_cast<BinaryOpInst&>(I)); break;
    case Instruction::Opcode::ICmp:   visitICmp(static_cast<ICmpInst&>(I)); break;
    case Instruction::Opcode::Alloca: visitAlloca(static_cast<AllocaInst&>(I)); break;
    case Instruction::Opcode::Load:   visitLoad(static_cast<LoadInst&>(I)); break;
    case Instruction::Opcode::Store:  visitStore(static_cast<StoreInst&>(I)); break;
    case Instruction::Opcode::Call:   visitCall(static_cast<CallInst&>(I)); break;
    case Instruction::Opcode::SExt:   visitSExt(I); break;
    case Instruction::Opcode::ZExt:   visitZExt(I); break;
    case Instruction::Opcode::Trunc:  visitTrunc(I); break;
    case Instruction::Opcode::Phi:    visitPhi(static_cast<PhiInst&>(I)); break;
    case Instruction::Opcode::ArrayLoad:
        visitArrayLoad(static_cast<ArrayLoadInst&>(I)); break;
    case Instruction::Opcode::ArrayStore:
        visitArrayStore(static_cast<ArrayStoreInst&>(I)); break;
    default: break;
    }
}

// ============================================================
// Individual instruction visitors
// ============================================================

void SelectionDAGBuilder::visitRet(RetInst &I) {
    if (I.getReturnValue()) {
        SDValue retVal = getValue(I.getReturnValue());
        Chain = DAG.getRet(Chain, retVal);
    } else {
        Chain = DAG.getRet(Chain);
    }
}

void SelectionDAGBuilder::visitBr(BranchInst &I) {
    auto *MBB = getMBB(I.getParent());
    if (I.isConditional()) {
        SDValue cond = getValue(I.getCondition());
        auto *trueBB = getMBB(I.getTrueDest());
        auto *falseBB = getMBB(I.getFalseDest());
        Chain = DAG.getBrCC(Chain, cond, trueBB, falseBB);
    } else {
        auto *dest = getMBB(I.getUnconditionalDest());
        Chain = DAG.getBr(Chain, dest);
    }
}

// Binary arithmetic: ADD, SUB, MUL, SDIV, SREM, AND, OR, XOR
static ISD::NodeType mapBinaryOp(Instruction::Opcode op) {
    switch (op) {
    case Instruction::Opcode::Add:  return ISD::ADD;
    case Instruction::Opcode::Sub:  return ISD::SUB;
    case Instruction::Opcode::Mul:  return ISD::MUL;
    case Instruction::Opcode::SDiv: return ISD::SDIV;
    case Instruction::Opcode::SRem: return ISD::SREM;
    case Instruction::Opcode::And:  return ISD::AND;
    case Instruction::Opcode::Or:   return ISD::OR;
    case Instruction::Opcode::Xor:  return ISD::XOR;
    default: return ISD::DELETED_NODE;
    }
}

void SelectionDAGBuilder::visitAdd(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::ADD, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitSub(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::SUB, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitMul(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::MUL, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitSDiv(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::SDIV, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitSRem(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::SREM, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitAnd(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::AND, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitOr(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::OR, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitXor(BinaryOpInst &I) {
    SDValue LHS = getValue(I.getLHS());
    SDValue RHS = getValue(I.getRHS());
    SDValue result = DAG.getBinary(ISD::XOR, I.getLHS()->getType(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitICmp(ICmpInst &I) {
    SDValue LHS = getValue(I.getOperand(0));
    SDValue RHS = getValue(I.getOperand(1));
    SDValue result = DAG.getSetCC(I.getPredicate(), LHS, RHS);
    setValue(&I, result);
}

void SelectionDAGBuilder::visitAlloca(AllocaInst &I) {
    int size = I.isArrayAlloca() ? (I.getArraySize() * 2) : 2;
    if (I.getAllocatedType() == Type::getInt8Ty()) size = I.isArrayAlloca() ? I.getArraySize() : 1;
    int FI = DAG.getMFI().CreateStackObject(size, 2, I.isArrayAlloca(), I.getArraySize());
    SDValue frameIdx = DAG.getFrameIndex(FI, I.getType());
    setValue(&I, frameIdx);
}

void SelectionDAGBuilder::visitLoad(LoadInst &I) {
    SDValue ptr = getValue(I.getPointer());
    SDValue load = DAG.getLoad(I.getType(), Chain, ptr);
    Chain = load; // Load produces a new chain
    setValue(&I, load);
}

void SelectionDAGBuilder::visitStore(StoreInst &I) {
    SDValue val = getValue(I.getValue());
    SDValue ptr = getValue(I.getPointer());
    Chain = DAG.getStore(Chain, val, ptr);
}

void SelectionDAGBuilder::visitCall(CallInst &I) {
    // Gather args
    std::vector<SDValue> args;
    for (unsigned i = 0; i < I.getNumOperands(); ++i) {
        Value *op = I.getOperand(i);
        // Skip the callee function operand (operand 0)
        if (i == 0) continue;
        if (auto *argVal = dynamic_cast<Value*>(op)) {
            args.push_back(getValue(argVal));
        }
    }

    // Get callee name
    std::string calleeName = "unknown";
    if (auto *fn = dynamic_cast<Function*>(I.getOperand(0))) {
        calleeName = fn->getName();
    }

    // Chain: CALLSEQ_START → CALL → CALLSEQ_END
    SDValue callSeqStart = DAG.getCALLSEQ_START(Chain);
    SDValue call = DAG.getCall(callSeqStart, calleeName, args, I.getType());
    Chain = DAG.getCALLSEQ_END(call);

    if (I.getType() != Type::getVoidTy()) {
        setValue(&I, call);
    }
}

void SelectionDAGBuilder::visitSExt(Instruction &I) {
    SDValue src = getValue(I.getOperand(0));
    SDValue ext = DAG.getSExt(src, I.getType());
    setValue(&I, ext);
}

void SelectionDAGBuilder::visitZExt(Instruction &I) {
    SDValue src = getValue(I.getOperand(0));
    SDValue ext = DAG.getZExt(src, I.getType());
    setValue(&I, ext);
}

void SelectionDAGBuilder::visitTrunc(Instruction &I) {
    SDValue src = getValue(I.getOperand(0));
    SDValue trunc = DAG.getTrunc(src, I.getType());
    setValue(&I, trunc);
}

void SelectionDAGBuilder::visitPhi(PhiInst &I) {
    // Create a virtual register for the phi result
    Register destVReg = assignVReg(&I, I.getType());

    // Record pending copies for each incoming edge
    for (unsigned i = 0; i < I.getNumIncoming(); ++i) {
        Value *srcVal = I.getIncomingValue(i);
        BasicBlock *predBB = I.getIncomingBlock(i);
        PendingCopies.push_back({destVReg, srcVal, predBB});
    }

    // The phi value will be available via CopyFromReg after lowering
    SDValue copyFromReg = DAG.getCopyFromReg(Chain, destVReg, I.getType());
    setValue(&I, copyFromReg);
}

void SelectionDAGBuilder::visitArrayLoad(ArrayLoadInst &I) {
    SDValue base = getValue(I.getBase());
    SDValue idx = getValue(I.getIndex());
    SDValue load = DAG.getIndexedLoad(I.getType(), Chain, base, idx);
    Chain = load;
    setValue(&I, load);
}

void SelectionDAGBuilder::visitArrayStore(ArrayStoreInst &I) {
    SDValue val = getValue(I.getValue());
    SDValue base = getValue(I.getBase());
    SDValue idx = getValue(I.getIndex());
    Chain = DAG.getIndexedStore(Chain, val, base, idx);
}

// ============================================================
// Helpers
// ============================================================
SDValue SelectionDAGBuilder::getValue(Value *V) {
    if (!V) return SDValue();

    // Check the value map first
    auto it = ValueMap.find(V);
    if (it != ValueMap.end()) return it->second;

    // Handle constants
    if (auto *ci = dynamic_cast<ConstantInt*>(V)) {
        SDValue c = DAG.getConstant(ci->getValue(), V->getType());
        ValueMap[V] = c;
        return c;
    }

    // Handle arguments — they should already have CopyFromReg nodes
    if (dynamic_cast<Argument*>(V)) {
        // Should have been set during visit(Function)
        return SDValue();
    }

    // Handle basic blocks — return chain reference
    if (auto *bb = dynamic_cast<BasicBlock*>(V)) {
        auto *MBB = getMBB(const_cast<BasicBlock*>(bb));
        return DAG.getBasicBlock(MBB);
    }

    // Unknown value
    std::cerr << "[sdbuilder] warning: unhandled value " << V->getName() << std::endl;
    return SDValue();
}

Register SelectionDAGBuilder::getVReg(Value *V) {
    auto it = VRegMap.find(V);
    if (it != VRegMap.end()) return it->second;
    return NoRegister;
}

MachineBasicBlock* SelectionDAGBuilder::getMBB(BasicBlock *BB) {
    auto it = BBMap.find(BB);
    if (it != BBMap.end()) return it->second;
    return nullptr;
}

void SelectionDAGBuilder::setValue(Value *V, SDValue Val) {
    ValueMap[V] = Val;
}

Register SelectionDAGBuilder::assignVReg(Value *V, Type *Ty) {
    const TargetRegisterClass *RC = TLI.getRegClassForType(Ty);
    if (!RC) {
        // Fallback: use GR16 (should not normally happen)
        RC = &static_cast<const Target8086RegisterInfo&>(TRI).getGR16ABCDClass();
    }
    Register vreg = DAG.getMRI().createVirtualRegister(RC);
    VRegMap[V] = vreg;
    return vreg;
}

void SelectionDAGBuilder::lowerPendingPhis() {
    if (PendingCopies.empty()) return;

    for (auto &copy : PendingCopies) {
        Register destReg = copy.DestReg;
        Value *srcVal = copy.SrcVal;
        BasicBlock *predBB = copy.PredBB;

        // Get the source virtual register
        Register srcReg = getVReg(srcVal);
        if (srcReg == NoRegister) {
            // Source value might be a constant — create a vreg and COPY
            srcReg = assignVReg(srcVal, srcVal->getType());
        }

        // Get the predecessor MBB
        auto *predMBB = getMBB(predBB);
        if (!predMBB) continue;

        // Insert COPY instruction before terminator in predecessor
        MachineInstr copyMI(MCOpcode::COPY, IsCopy);
        copyMI.addReg(destReg, true);  // def
        copyMI.addReg(srcReg);          // use
        predMBB->insertBeforeTerminator(std::move(copyMI));
    }

    PendingCopies.clear();
}

} // namespace ll1
