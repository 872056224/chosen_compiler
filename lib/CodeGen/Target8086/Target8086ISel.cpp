#include <ll1/CodeGen/Target8086/Target8086ISel.h>
#include <ll1/CodeGen/Target/TargetInstrInfo.h>
#include <ll1/CodeGen/Target8086/Target8086RegisterInfo.h>
#include <iostream>

namespace ll1 {

Target8086ISel::Target8086ISel(SelectionDAG &dag, const TargetRegisterInfo &tri,
                               TargetLowering &tli)
    : DAG(dag), TRI(tri), TLI(tli) {}

// ============================================================
// Register management
// ============================================================
Register Target8086ISel::getOrCreateVReg(SDNode *N) {
    auto it = NodeVRegs.find(N);
    if (it != NodeVRegs.end()) return it->second;

    // Create a new virtual register for this node's result
    const TargetRegisterClass *RC = &static_cast<const Target8086RegisterInfo&>(TRI).getGR16ABCDClass();
    Register vreg = DAG.getMRI().createVirtualRegister(RC);
    NodeVRegs[N] = vreg;
    return vreg;
}

Register Target8086ISel::getVReg(SDNode *N) const {
    auto it = NodeVRegs.find(N);
    if (it != NodeVRegs.end()) return it->second;
    return NoRegister;
}

Register Target8086ISel::getOperandVReg(SDValue Val) {
    if (!Val.isValid()) return NoRegister;
    SDNode *N = Val.getNode();
    if (!N) return NoRegister;

    // If this node already has a vreg assigned, return it
    auto it = NodeVRegs.find(N);
    if (it != NodeVRegs.end()) return it->second;

    // For constants: load into a vreg
    if (N->getOpcode() == ISD::Constant) {
        Register vreg = getOrCreateVReg(N);
        MachineInstr &mi = emitMI(MCOpcode::MOV, IsMoveImm);
        mi.addReg(vreg, true);
        mi.addImm(0);  // Constant value — handled specially
        return vreg;
    }

    // For FrameIndex: compute address into vreg (lea simulation)
    if (N->getOpcode() == ISD::FrameIndex) {
        return getOrCreateVReg(N);
    }

    return getOrCreateVReg(N);
}

MachineInstr& Target8086ISel::emitMI(MCOpcode opc, uint32_t flags) {
    CurMBB->push_back(MachineInstr(opc, flags));
    return CurMBB->back();
}

// ============================================================
// runOnFunction
// ============================================================
void Target8086ISel::runOnFunction(MachineFunction &MF) {
    NodeVRegs.clear();
    ConstVRegs.clear();

    auto &mbbList = MF.getBasicBlocks();
    unsigned mbbIdx = 0;
    if (!mbbList.empty()) {
        CurMBB = mbbList[mbbIdx].get();
    }

    for (auto *N : DAG.getAllNodes()) {
        if (N->getOpcode() == ISD::DELETED_NODE) continue;
        if (N->getOpcode() >= ISD::BUILTIN_OP_END) continue;

        selectNode(N);

        // After a REAL terminator, advance to next MBB
        // BR nodes from getBasicBlock have 0 operands — skip those
        bool isRealTerm = false;
        if (N->getOpcode() == ISD::BR && N->getNumOperands() > 0) isRealTerm = true;
        if (N->getOpcode() == ISD::BR_CC) isRealTerm = true;
        if (N->getOpcode() == ISD::RET) isRealTerm = true;

        if (isRealTerm) {
            mbbIdx++;
            if (mbbIdx < mbbList.size()) {
                CurMBB = mbbList[mbbIdx].get();
            }
        }
    }
}

void Target8086ISel::selectNode(SDNode *N) {
    switch (N->getOpcode()) {
    // Terminators
    case ISD::BR:        selectBr(N); break;
    case ISD::BR_CC:     selectBrCC(N); break;
    case ISD::RET:       selectRet(N); break;

    // Arithmetic
    case ISD::ADD:       selectAdd(N); break;
    case ISD::SUB:       selectSub(N); break;
    case ISD::MUL:       selectMul(N); break;
    case ISD::SDIV:      selectSDiv(N); break;
    case ISD::SREM:      selectSRem(N); break;
    case ISD::AND:       selectAnd(N); break;
    case ISD::OR:        selectOr(N); break;
    case ISD::XOR:       selectXor(N); break;

    // Memory
    case ISD::LOAD:      selectLoad(N); break;
    case ISD::STORE:     selectStore(N); break;
    case ISD::FrameIndex: selectFrameIndex(N); break;

    // Copy
    case ISD::CopyToReg:   selectCopyToReg(N); break;
    case ISD::CopyFromReg: selectCopyFromReg(N); break;

    // Cast
    case ISD::SIGN_EXTEND: selectSExt(N); break;
    case ISD::ZERO_EXTEND: selectZExt(N); break;
    case ISD::TRUNCATE:    selectTrunc(N); break;

    // Compare
    case ISD::SETCC:       selectSetCC(N); break;

    // Constant
    case ISD::Constant:    selectConstant(N); break;

    // Call
    case ISD::CALL:        selectCall(N); break;

    // Chain / token (no code)
    case ISD::EntryToken:
    case ISD::TokenFactor:
    case ISD::ADJCALLSTACKDOWN:
    case ISD::ADJCALLSTACKUP:
        break;

    default:
        if (!ISD::isMachineOpcode(N->getOpcode())) {
            std::cerr << "[isel] unhandled ISD opcode: " << N->getOpcodeName()
                      << " (" << N->getOpcode() << ")" << std::endl;
        }
        break;
    }
}

// ============================================================
// Binary arithmetic: ADD, SUB, AND, OR, XOR
// Pattern: mov dest, src1; add dest, src2 (two-address)
// ============================================================
void Target8086ISel::selectBinary(SDNode *N, MCOpcode opc) {
    Register dest = getOperandVReg(SDValue(N, 0));
    Register src1 = getOperandVReg(N->getOperand(0));
    Register src2 = getOperandVReg(N->getOperand(1));

    // MOV dest, src1
    MachineInstr &mov = emitMI(MCOpcode::MOV, IsMoveImm);
    mov.addReg(dest, true);
    mov.addReg(src1);

    // ADD/SUB/etc dest, src2
    MachineInstr &arith = emitMI(opc);
    arith.addReg(dest, true);
    arith.addReg(src2);
}

void Target8086ISel::selectAdd(SDNode *N) { selectBinary(N, MCOpcode::ADD); }
void Target8086ISel::selectSub(SDNode *N) { selectBinary(N, MCOpcode::SUB); }
void Target8086ISel::selectAnd(SDNode *N) { selectBinary(N, MCOpcode::AND); }
void Target8086ISel::selectOr(SDNode *N)  { selectBinary(N, MCOpcode::OR); }
void Target8086ISel::selectXor(SDNode *N) { selectBinary(N, MCOpcode::XOR); }

// ============================================================
// MUL — uses AX implicitly
// Pattern: mov ax, src1; mul src2
// ============================================================
void Target8086ISel::selectMul(SDNode *N) {
    Register src1 = getOperandVReg(N->getOperand(0));
    Register src2 = getOperandVReg(N->getOperand(1));
    Register dest = getOperandVReg(SDValue(N, 0));

    // MOV AX, src1
    MachineInstr &mov = emitMI(MCOpcode::MOV, IsMoveImm);
    mov.addReg(X86::AX, true);
    mov.addReg(src1);

    // MUL src2 (implicit: AX = AX * src2, DX = high)
    MachineInstr &mul = emitMI(MCOpcode::MUL);
    mul.addReg(src2);

    // Result is in AX → MOV dest, AX
    MachineInstr &movRes = emitMI(MCOpcode::MOV, IsMoveImm);
    movRes.addReg(dest, true);
    movRes.addReg(X86::AX);
}

// ============================================================
// SDIV — uses AX:DX
// Pattern: mov ax, src1; cwd; idiv src2; mov dest, ax
// ============================================================
void Target8086ISel::selectSDiv(SDNode *N) {
    Register src1 = getOperandVReg(N->getOperand(0));
    Register src2 = getOperandVReg(N->getOperand(1));
    Register dest = getOperandVReg(SDValue(N, 0));

    emitMI(MCOpcode::MOV, IsMoveImm).addReg(X86::AX, true).addReg(src1);
    emitMI(MCOpcode::CWD);  // Sign-extend AX → DX:AX
    emitMI(MCOpcode::IDIV).addReg(src2);
    emitMI(MCOpcode::MOV, IsMoveImm).addReg(dest, true).addReg(X86::AX);
}

// ============================================================
// SREM — remainder in DX after IDIV
// ============================================================
void Target8086ISel::selectSRem(SDNode *N) {
    Register src1 = getOperandVReg(N->getOperand(0));
    Register src2 = getOperandVReg(N->getOperand(1));
    Register dest = getOperandVReg(SDValue(N, 0));

    emitMI(MCOpcode::MOV, IsMoveImm).addReg(X86::AX, true).addReg(src1);
    emitMI(MCOpcode::CWD);
    emitMI(MCOpcode::IDIV).addReg(src2);
    // Remainder is in DX
    emitMI(MCOpcode::MOV, IsMoveImm).addReg(dest, true).addReg(X86::DX);
}

// ============================================================
// LOAD — load from frame index
// ============================================================
void Target8086ISel::selectLoad(SDNode *N) {
    SDValue ptrVal = N->getOperand(1);
    Register dest = getOperandVReg(SDValue(N, 0));

    if (ptrVal.getNode() && ptrVal.getOpcode() == ISD::FrameIndex) {
        int fi = ptrVal.getNode()->getFrameIndex();
        MachineInstr &mi = emitMI(MCOpcode::MOV);
        mi.addReg(dest, true);
        mi.addFrameIndex(fi);
    } else {
        Register ptr = getOperandVReg(ptrVal);
        MachineInstr &mi = emitMI(MCOpcode::MOV);
        mi.addReg(dest, true);
        mi.addReg(ptr);
    }
}

void Target8086ISel::selectStore(SDNode *N) {
    if (N->getNumOperands() < 3) return;
    SDValue val = N->getOperand(1);
    SDValue ptr = N->getOperand(2);

    if (!val.isValid()) return;
    Register srcReg = getOperandVReg(val);

    if (ptr.isValid() && ptr.getOpcode() == ISD::FrameIndex) {
        int fi = ptr.getNode()->getFrameIndex();
        MachineInstr &mi = emitMI(MCOpcode::MOV);
        mi.addFrameIndex(fi);
        mi.addReg(srcReg);
    } else if (ptr.isValid()) {
        Register ptrReg = getOperandVReg(ptr);
        MachineInstr &mi = emitMI(MCOpcode::MOV);
        mi.addReg(ptrReg);
        mi.addReg(srcReg);
    }
}

void Target8086ISel::selectFrameIndex(SDNode *N) {
    // FrameIndex: just create a vreg for it (accessed via [bx+offset] in printer)
    getOrCreateVReg(N);
}

// ============================================================
// Control flow
// ============================================================
void Target8086ISel::selectBr(SDNode *N) {
    // Skip MBB reference nodes (created by getBasicBlock, 0 operands)
    if (N->getNumOperands() == 0) return;

    // Real unconditional branch — MBB stored in payload
    MachineBasicBlock *dest = N->getTargetMBB();
    MachineInstr &jmp = emitMI(MCOpcode::JMP, IsBranch);
    if (dest) jmp.addMBB(dest);
}

void Target8086ISel::selectBrCC(SDNode *N) {
    SDValue cond = N->getOperand(1);
    SDValue trueBBVal = N->getOperand(2);
    SDValue falseBBVal = N->getOperand(3);

    MachineBasicBlock *trueBB = trueBBVal.getNode() ? trueBBVal.getNode()->getTargetMBB() : nullptr;
    MachineBasicBlock *falseBB = falseBBVal.getNode() ? falseBBVal.getNode()->getTargetMBB() : nullptr;

    // Get predicate + operands from SETCC node
    uint8_t pred = ICmpInst::NE;
    Register lhs = NoRegister, rhs = NoRegister;
    bool hasSETCC = false;
    if (cond.getNode() && cond.getOpcode() == ISD::SETCC) {
        pred = cond.getNode()->Payload.CmpPred;
        // SETCC has two operands: LHS(0), RHS(1)
        if (cond.getNumOperands() >= 2) {
            lhs = getOperandVReg(cond.getOperand(0));
            rhs = getOperandVReg(cond.getOperand(1));
            hasSETCC = true;
        }
    }

    if (hasSETCC) {
        // Emit: CMP lhs, rhs; Jcc trueBB; JMP falseBB
        emitMI(MCOpcode::CMP).addReg(lhs).addReg(rhs);
    } else {
        // Generic: CMP condReg, 0
        Register condReg = getOperandVReg(cond);
        emitMI(MCOpcode::CMP).addReg(condReg).addImm(0);
    }

    // Map predicate to 8086 conditional jump
    // After CMP lhs,rhs: flags = lhs - rhs
    // For ICMP_EQ: lhs == rhs → JE (jump if equal, ZF=1)
    // For ICMP_SLT: lhs < rhs → JL (jump if less, SF≠OF)
    MCOpcode jcc;
    switch (pred) {
    case ICmpInst::EQ:  jcc = MCOpcode::JE;  break;
    case ICmpInst::NE:  jcc = MCOpcode::JNE; break;
    case ICmpInst::SLT: jcc = MCOpcode::JL;  break;
    case ICmpInst::SLE: jcc = MCOpcode::JLE; break;
    case ICmpInst::SGT: jcc = MCOpcode::JG;  break;
    case ICmpInst::SGE: jcc = MCOpcode::JGE; break;
    default:            jcc = MCOpcode::JNE; break;
    }

    MachineInstr &jmpCC = emitMI(jcc, IsBranch);
    if (trueBB) jmpCC.addMBB(trueBB);

    MachineInstr &jmp = emitMI(MCOpcode::JMP, IsBranch);
    if (falseBB) jmp.addMBB(falseBB);
}

void Target8086ISel::selectRet(SDNode *N) {
    if (N->getNumOperands() > 1) {
        // Return with value: mov ax, val
        SDValue retVal = N->getOperand(1);
        Register valReg = getOperandVReg(retVal);
        emitMI(MCOpcode::MOV, IsMoveImm).addReg(X86::AX, true).addReg(valReg);
    }
    // Epilogue handled by printer
    emitMI(MCOpcode::RET, IsReturn);
}

// ============================================================
// Copy ops: just pass through — handled by coalescer/regalloc
// ============================================================
void Target8086ISel::selectCopyToReg(SDNode *N) {
    // CopyToReg(Chain, DestVReg, SrcValue)
    // No code emitted — the register allocator handles these
}

void Target8086ISel::selectCopyFromReg(SDNode *N) {
    // CopyFromReg(Chain, SrcVReg, Type)
    // No code emitted — the register allocator handles these
}

void Target8086ISel::selectCopy(SDNode *N) {
    // COPY instruction — keep as COPY for coalescer
    Register dest = getOperandVReg(SDValue(N, 0));
    Register src = getOperandVReg(N->getOperand(0));
    emitMI(MCOpcode::COPY, IsCopy).addReg(dest, true).addReg(src);
}

// ============================================================
// Cast operations
// ============================================================
void Target8086ISel::selectSExt(SDNode *N) {
    // Sign-extend: i8 → i16 = movsx dest, src (but 8086 has cbw for AL→AX)
    // For general case: mov dest, src + extend
    Register dest = getOperandVReg(SDValue(N, 0));
    Register src = getOperandVReg(N->getOperand(0));
    emitMI(MCOpcode::MOVZX, IsMoveImm).addReg(dest, true).addReg(src);
}

void Target8086ISel::selectZExt(SDNode *N) {
    Register dest = getOperandVReg(SDValue(N, 0));
    Register src = getOperandVReg(N->getOperand(0));
    // Zero-extend: mov dest, 0; mov dest(low), src
    emitMI(MCOpcode::MOVZX, IsMoveImm).addReg(dest, true).addReg(src);
}

void Target8086ISel::selectTrunc(SDNode *N) {
    // Truncate i16 → i8: just use the low byte
    Register dest = getOperandVReg(SDValue(N, 0));
    Register src = getOperandVReg(N->getOperand(0));
    emitMI(MCOpcode::MOV, IsMoveImm).addReg(dest, true).addReg(src);
}

// ============================================================
// SETCC — compare, result in vreg
// ============================================================
void Target8086ISel::selectSetCC(SDNode *N) {
    // SETCC itself doesn't emit code — the CMP + Jcc pattern
    // is emitted by selectBrCC which follows.
    // Just create a vreg for the result if needed.
    getOrCreateVReg(N);
}

// ============================================================
// Constant
// ============================================================
void Target8086ISel::selectConstant(SDNode *N) {
    // Constant values are materialized on demand via MOV imm
    getOrCreateVReg(N);
}

// ============================================================
// CALL — cdecl convention: push args right-to-left, call, cleanup
// Node operands: Chain(0), Arg0(1), Arg1(2), ...
// ============================================================
void Target8086ISel::selectCall(SDNode *N) {
    const char *callee = N->getCallee();
    unsigned numArgs = N->getNumOperands() - 1;  // subtract chain operand
    int argBytes = numArgs * 2;  // each arg is 16-bit

    // Push args right-to-left
    for (int i = numArgs - 1; i >= 0; --i) {
        SDValue argVal = N->getOperand(i + 1);  // skip chain
        Register argReg = getOperandVReg(argVal);
        MachineInstr &push = emitMI(MCOpcode::PUSH);
        push.addReg(argReg);
    }

    // CALL callee
    MachineInstr &mi = emitMI(MCOpcode::CALL, IsCall);
    if (callee) {
        mi.addExternalSymbol(std::string(callee));
    }

    // Caller cleanup: add sp, N*2
    if (argBytes > 0) {
        // MOV a temp reg, sp; ADD sp, N — or just ADD sp, imm
        // 8086 doesn't have ADD sp, imm. Use: add sp, N (assembler handles it)
        MachineInstr &cleanup = emitMI(MCOpcode::ADD);
        cleanup.addReg(X86::SP, true);
        cleanup.addImm(argBytes);
    }
}

} // namespace ll1
