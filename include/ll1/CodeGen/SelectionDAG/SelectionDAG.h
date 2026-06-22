#ifndef LL1_CODEGEN_SELECTIONDAG_SELECTIONDAG_H
#define LL1_CODEGEN_SELECTIONDAG_SELECTIONDAG_H

#include <ll1/CodeGen/SelectionDAG/SDNode.h>
#include <ll1/CodeGen/MachineFunction.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/Instruction.h>
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>

namespace ll1 {

class TargetLowering;
class TargetRegisterInfo;

// ============================================================
// Simple BumpPtrAllocator — fast allocation, no individual frees
// ============================================================
class BumpPtrAllocator {
public:
    BumpPtrAllocator(size_t size = 4096);
    ~BumpPtrAllocator();

    void* Allocate(size_t size, size_t align);

    // Reset for reuse
    void Reset();

private:
    struct Slab {
        char *Data;
        size_t Size;
        size_t Used;
    };
    std::vector<Slab> Slabs;
    size_t DefaultSlabSize;

    void addSlab(size_t minSize);
};

// ============================================================
// SelectionDAG — The DAG builder and node allocator
// ============================================================
class SelectionDAG {
public:
    SelectionDAG(MachineFunction &MF);
    ~SelectionDAG();

    SelectionDAG(const SelectionDAG&) = delete;
    SelectionDAG& operator=(const SelectionDAG&) = delete;

    MachineFunction& getMachineFunction() { return MF; }
    MachineRegisterInfo& getMRI() { return MF.getRegInfo(); }
    MachineFrameInfo& getMFI() { return MF.getFrameInfo(); }

    // =========================================================
    // Node factory methods
    // =========================================================

    // Generic node creation
    SDValue getNode(unsigned Opc, Type *Ty,
                    const std::vector<SDValue> &Ops = {},
                    const SDNodeFlags &Flags = SDNodeFlags());

    // Leaf nodes
    SDValue getConstant(int16_t Val, Type *Ty);
    SDValue getRegister(Register Reg, Type *Ty);
    SDValue getFrameIndex(int FI, Type *Ty);
    SDValue getBasicBlock(MachineBasicBlock *MBB);
    SDValue getEntryToken();

    // Binary ops (shorthand)
    SDValue getBinary(unsigned Opc, Type *Ty, SDValue LHS, SDValue RHS);

    // Memory ops (with chain)
    SDValue getLoad(Type *Ty, SDValue Chain, SDValue Ptr);
    SDValue getStore(SDValue Chain, SDValue Val, SDValue Ptr);
    SDValue getIndexedLoad(Type *Ty, SDValue Chain, SDValue Base, SDValue Index);
    SDValue getIndexedStore(SDValue Chain, SDValue Val, SDValue Base, SDValue Index);

    // Control flow
    SDValue getBr(SDValue Chain, MachineBasicBlock *Dest);
    SDValue getBrCC(SDValue Chain, SDValue Cond,
                    MachineBasicBlock *TrueBB, MachineBasicBlock *FalseBB);
    SDValue getRet(SDValue Chain, SDValue Val = SDValue());

    // Comparison
    SDValue getSetCC(ICmpInst::Pred Pred, SDValue LHS, SDValue RHS);

    // Copy ops
    SDValue getCopyToReg(SDValue Chain, Register DestReg, SDValue Src);
    SDValue getCopyFromReg(SDValue Chain, Register SrcReg, Type *Ty);

    // Merge / token
    SDValue getTokenFactor(SDValue Chain1, SDValue Chain2);

    // Call
    SDValue getCall(SDValue Chain, const std::string &Callee,
                    const std::vector<SDValue> &Args, Type *RetTy);
    SDValue getCALLSEQ_START(SDValue Chain);
    SDValue getCALLSEQ_END(SDValue Chain);

    // Cast
    SDValue getSExt(SDValue Val, Type *DestTy);
    SDValue getZExt(SDValue Val, Type *DestTy);
    SDValue getTrunc(SDValue Val, Type *DestTy);

    // =========================================================
    // DAG management
    // =========================================================
    void clear();
    std::vector<SDNode*>& getAllNodes() { return AllNodes; }

private:
    MachineFunction &MF;
    BumpPtrAllocator Allocator;
    std::vector<SDNode*> AllNodes;
    std::deque<std::string> StringPool;  // Keeps callee names alive (deque: no realloc)

    // Allocate and register a new SDNode
    SDNode* newSDNode(unsigned Opc, unsigned NumOps, unsigned NumVals, Type *Ty);

    // Allocate raw memory for an SDNode + its trailing arrays
    void* allocateNodeMemory(unsigned NumOps, unsigned NumVals);
};

} // namespace ll1
#endif
