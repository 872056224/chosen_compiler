#ifndef LL1_CODEGEN_TARGET8086_TARGET8086ISEL_H
#define LL1_CODEGEN_TARGET8086_TARGET8086ISEL_H

#include <ll1/CodeGen/SelectionDAG/SelectionDAG.h>
#include <ll1/CodeGen/MachineFunction.h>
#include <ll1/CodeGen/Target/TargetRegisterInfo.h>
#include <ll1/CodeGen/Target/TargetLowering.h>
#include <unordered_map>

namespace ll1 {

// ============================================================
// Target8086ISel — 8086 instruction selection from SelectionDAG
//
// Walks each SDNode in the DAG, pattern-matches to 8086 MachineInstr,
// and appends to the appropriate MachineBasicBlock.
// ============================================================
class Target8086ISel {
public:
    Target8086ISel(SelectionDAG &dag, const TargetRegisterInfo &tri,
                   TargetLowering &tli);

    void runOnFunction(MachineFunction &MF);

private:
    SelectionDAG &DAG;
    const TargetRegisterInfo &TRI;
    TargetLowering &TLI;

    // Current MBB being selected into
    MachineBasicBlock *CurMBB = nullptr;

    // Map: SDNode* result → virtual register
    std::unordered_map<SDNode*, Register> NodeVRegs;

    // Map: constant value → loaded virtual register
    std::unordered_map<int16_t, Register> ConstVRegs;

    // Register helpers
    Register getOrCreateVReg(SDNode *N);
    Register getVReg(SDNode *N) const;
    Register getOperandVReg(SDValue Val);

    // =========================================================
    // Selection methods (one per ISD opcode category)
    // =========================================================
    void selectNode(SDNode *N);

    // Arithmetic
    void selectBinary(SDNode *N, MCOpcode opc);
    void selectAdd(SDNode *N);
    void selectSub(SDNode *N);
    void selectMul(SDNode *N);
    void selectSDiv(SDNode *N);
    void selectSRem(SDNode *N);
    void selectAnd(SDNode *N);
    void selectOr(SDNode *N);
    void selectXor(SDNode *N);

    // Memory
    void selectLoad(SDNode *N);
    void selectStore(SDNode *N);
    void selectFrameIndex(SDNode *N);
    void selectIndexedLoad(SDNode *N);
    void selectIndexedStore(SDNode *N);

    // Control flow
    void selectBr(SDNode *N);
    void selectBrCC(SDNode *N);
    void selectRet(SDNode *N);

    // Call
    void selectCall(SDNode *N);

    // Copy/register
    void selectCopyToReg(SDNode *N);
    void selectCopyFromReg(SDNode *N);
    void selectCopy(SDNode *N);

    // Cast
    void selectSExt(SDNode *N);
    void selectZExt(SDNode *N);
    void selectTrunc(SDNode *N);

    // Compare
    void selectSetCC(SDNode *N);

    // Constants
    void selectConstant(SDNode *N);

    // Insert instruction into current MBB
    MachineInstr& emitMI(MCOpcode opc, uint32_t flags = 0);
};

} // namespace ll1
#endif
