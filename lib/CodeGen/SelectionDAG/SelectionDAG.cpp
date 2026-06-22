#include <ll1/CodeGen/SelectionDAG/SelectionDAG.h>
#include <ll1/CodeGen/Target/TargetRegisterInfo.h>
#include <cstring>
#include <cassert>

namespace ll1 {

// ============================================================
// BumpPtrAllocator
// ============================================================
BumpPtrAllocator::BumpPtrAllocator(size_t size) : DefaultSlabSize(size) {}

BumpPtrAllocator::~BumpPtrAllocator() {
    for (auto &slab : Slabs) delete[] slab.Data;
}

void BumpPtrAllocator::addSlab(size_t minSize) {
    size_t size = std::max(DefaultSlabSize, minSize);
    Slab slab;
    slab.Data = new char[size];
    slab.Size = size;
    slab.Used = 0;
    Slabs.push_back(slab);
}

void* BumpPtrAllocator::Allocate(size_t size, size_t align) {
    if (Slabs.empty()) addSlab(size);

    Slab &cur = Slabs.back();
    // Align current position
    size_t aligned = (cur.Used + align - 1) & ~(align - 1);
    if (aligned + size > cur.Size) {
        addSlab(size + align);
        aligned = 0;
    }
    void *ptr = Slabs.back().Data + aligned;
    Slabs.back().Used = aligned + size;
    return ptr;
}

void BumpPtrAllocator::Reset() {
    for (auto &slab : Slabs) delete[] slab.Data;
    Slabs.clear();
}

// ============================================================
// SelectionDAG
// ============================================================
SelectionDAG::SelectionDAG(MachineFunction &mf) : MF(mf) {}

SelectionDAG::~SelectionDAG() { clear(); }

void SelectionDAG::clear() {
    AllNodes.clear();
    Allocator.Reset();
}

// Allocate memory for an SDNode + its trailing arrays
void* SelectionDAG::allocateNodeMemory(unsigned NumOps, unsigned NumVals) {
    // Layout: SDNode header + SDUse[NumOps] + Type*[NumVals]
    size_t nodeSize = sizeof(SDNode);
    size_t usesSize = NumOps * sizeof(SDUse);
    size_t typesSize = NumVals * sizeof(Type*);
    size_t total = nodeSize + usesSize + typesSize;
    return Allocator.Allocate(total, alignof(SDNode));
}

SDNode* SelectionDAG::newSDNode(unsigned Opc, unsigned NumOps, unsigned NumVals, Type *Ty) {
    void *mem = allocateNodeMemory(NumOps, NumVals);
    // Placement new for the SDNode
    auto *node = new (mem) SDNode(Opc, NumOps, NumVals, Ty);
    AllNodes.push_back(node);
    return node;
}

// ============================================================
// Node factory methods
// ============================================================
SDValue SelectionDAG::getNode(unsigned Opc, Type *Ty,
                               const std::vector<SDValue> &Ops,
                               const SDNodeFlags &Flags) {
    SDNode *node = newSDNode(Opc, Ops.size(), 1, Ty);
    node->setFlags(Flags);
    node->initOperands(Ops);
    return SDValue(node, 0);
}

SDValue SelectionDAG::getConstant(int16_t Val, Type *Ty) {
    SDNode *node = newSDNode(ISD::Constant, 0, 1, Ty);
    node->Payload.ConstVal = Val;
    return SDValue(node, 0);
}

SDValue SelectionDAG::getRegister(Register Reg, Type *Ty) {
    SDNode *node = newSDNode(ISD::Register, 0, 1, Ty);
    return SDValue(node, 0);
}

SDValue SelectionDAG::getFrameIndex(int FI, Type *Ty) {
    SDNode *node = newSDNode(ISD::FrameIndex, 0, 1, Ty);
    node->Payload.FrameIdx = FI;
    return SDValue(node, 0);
}

SDValue SelectionDAG::getBasicBlock(MachineBasicBlock *MBB) {
    SDNode *node = newSDNode(ISD::BR, 0, 0, nullptr);
    node->setTargetMBB(MBB);
    return SDValue(node, 0);
}

SDValue SelectionDAG::getEntryToken() {
    return getNode(ISD::EntryToken, nullptr, {});
}

SDValue SelectionDAG::getBinary(unsigned Opc, Type *Ty, SDValue LHS, SDValue RHS) {
    return getNode(Opc, Ty, {LHS, RHS});
}

SDValue SelectionDAG::getLoad(Type *Ty, SDValue Chain, SDValue Ptr) {
    return getNode(ISD::LOAD, Ty, {Chain, Ptr});
}

SDValue SelectionDAG::getStore(SDValue Chain, SDValue Val, SDValue Ptr) {
    return getNode(ISD::STORE, nullptr, {Chain, Val, Ptr});
}

SDValue SelectionDAG::getIndexedLoad(Type *Ty, SDValue Chain, SDValue Base, SDValue Index) {
    return getNode(ISD::IndexedLoad, Ty, {Chain, Base, Index});
}

SDValue SelectionDAG::getIndexedStore(SDValue Chain, SDValue Val, SDValue Base, SDValue Index) {
    return getNode(ISD::IndexedStore, nullptr, {Chain, Val, Base, Index});
}

SDValue SelectionDAG::getBr(SDValue Chain, MachineBasicBlock *Dest) {
    SDValue result = getNode(ISD::BR, nullptr, {Chain});
    result.getNode()->setTargetMBB(Dest);
    return result;
}

SDValue SelectionDAG::getBrCC(SDValue Chain, SDValue Cond,
                               MachineBasicBlock *TrueBB, MachineBasicBlock *FalseBB) {
    auto *node = newSDNode(ISD::BR_CC, 4, 0, nullptr);
    SDValue tbb = getBasicBlock(TrueBB);
    SDValue fbb = getBasicBlock(FalseBB);
    node->initOperands({Chain, Cond, tbb, fbb});
    return SDValue(node, 0);
}

SDValue SelectionDAG::getRet(SDValue Chain, SDValue Val) {
    if (Val.isValid())
        return getNode(ISD::RET, nullptr, {Chain, Val});
    return getNode(ISD::RET, nullptr, {Chain});
}

SDValue SelectionDAG::getSetCC(ICmpInst::Pred Pred, SDValue LHS, SDValue RHS) {
    SDValue result = getNode(ISD::SETCC, Type::getInt1Ty(), {LHS, RHS});
    result.getNode()->Payload.CmpPred = static_cast<uint8_t>(Pred);
    return result;
}

SDValue SelectionDAG::getCopyToReg(SDValue Chain, Register DestReg, SDValue Src) {
    auto *node = newSDNode(ISD::CopyToReg, 2, 1, Src.getType());
    return SDValue(node, 0);
}

SDValue SelectionDAG::getCopyFromReg(SDValue Chain, Register SrcReg, Type *Ty) {
    auto *node = newSDNode(ISD::CopyFromReg, 1, 1, Ty);
    return SDValue(node, 0);
}

SDValue SelectionDAG::getTokenFactor(SDValue Chain1, SDValue Chain2) {
    return getNode(ISD::TokenFactor, nullptr, {Chain1, Chain2});
}

SDValue SelectionDAG::getCall(SDValue Chain, const std::string &Callee,
                               const std::vector<SDValue> &Args, Type *RetTy) {
    std::vector<SDValue> ops = {Chain};
    ops.insert(ops.end(), Args.begin(), Args.end());
    SDValue result = getNode(ISD::CALL, RetTy, ops);
    StringPool.push_back(Callee);
    result.getNode()->setCallee(StringPool.back().c_str());
    return result;
}

SDValue SelectionDAG::getCALLSEQ_START(SDValue Chain) {
    return getNode(ISD::ADJCALLSTACKDOWN, nullptr, {Chain});
}

SDValue SelectionDAG::getCALLSEQ_END(SDValue Chain) {
    return getNode(ISD::ADJCALLSTACKUP, nullptr, {Chain});
}

SDValue SelectionDAG::getSExt(SDValue Val, Type *DestTy) {
    return getNode(ISD::SIGN_EXTEND, DestTy, {Val});
}

SDValue SelectionDAG::getZExt(SDValue Val, Type *DestTy) {
    return getNode(ISD::ZERO_EXTEND, DestTy, {Val});
}

SDValue SelectionDAG::getTrunc(SDValue Val, Type *DestTy) {
    return getNode(ISD::TRUNCATE, DestTy, {Val});
}

} // namespace ll1
