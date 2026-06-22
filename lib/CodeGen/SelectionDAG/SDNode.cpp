#include <ll1/CodeGen/SelectionDAG/SDNode.h>
#include <cassert>
#include <cstddef>

namespace ll1 {

// ============================================================
// SDValue delegating methods
// ============================================================
unsigned SDValue::getOpcode() const {
    return Node ? Node->getOpcode() : ISD::DELETED_NODE;
}

unsigned SDValue::getNumOperands() const {
    return Node ? Node->getNumOperands() : 0;
}

SDValue SDValue::getOperand(unsigned i) const {
    assert(Node && i < Node->getNumOperands());
    return Node->getOperand(i);
}

Type* SDValue::getValueType() const {
    return Node ? Node->getValueType(ResNo) : nullptr;
}

// ============================================================
// SDNode implementation
// ============================================================
SDNode::SDNode(unsigned Opc_, unsigned NumOps, unsigned NumVals, Type *Ty)
    : Opc(Opc_), NumOperands(NumOps), NumValues(NumVals) {
    // The OperandList and ResultTypes are placed in trailing memory
    // (allocated by the BumpPtrAllocator right after the SDNode header)
    // The caller (SelectionDAG) handles this.
    OperandList = reinterpret_cast<SDUse*>(this + 1);
    ResultTypes = reinterpret_cast<Type**>(OperandList + NumOps);
    if (NumVals > 0 && Ty) {
        ResultTypes[0] = Ty;
    }
}

void SDNode::initOperand(unsigned i, const SDValue &V) {
    assert(i < NumOperands);
    OperandList[i] = SDUse(V, this);
}

void SDNode::initOperands(const std::vector<SDValue> &Ops) {
    assert(Ops.size() <= NumOperands);
    for (size_t i = 0; i < Ops.size(); ++i) {
        OperandList[i] = SDUse(Ops[i], this);
    }
}

unsigned SDNode::computeHash() const {
    unsigned hash = Opc;
    for (unsigned i = 0; i < NumOperands; ++i) {
        hash = hash * 31 + (uintptr_t)OperandList[i].get().getNode();
    }
    return hash;
}

const char* SDNode::getOpcodeName() const {
    switch (Opc) {
    case ISD::DELETED_NODE:  return "DELETED_NODE";
    case ISD::EntryToken:    return "EntryToken";
    case ISD::BR:            return "BR";
    case ISD::BR_CC:         return "BR_CC";
    case ISD::RET:           return "RET";
    case ISD::CopyFromReg:   return "CopyFromReg";
    case ISD::CopyToReg:     return "CopyToReg";
    case ISD::Register:      return "Register";
    case ISD::Constant:      return "Constant";
    case ISD::LOAD:          return "LOAD";
    case ISD::STORE:         return "STORE";
    case ISD::FrameIndex:    return "FrameIndex";
    case ISD::IndexedLoad:   return "IndexedLoad";
    case ISD::IndexedStore:  return "IndexedStore";
    case ISD::ADD:           return "ADD";
    case ISD::SUB:           return "SUB";
    case ISD::MUL:           return "MUL";
    case ISD::SDIV:          return "SDIV";
    case ISD::SREM:          return "SREM";
    case ISD::AND:           return "AND";
    case ISD::OR:            return "OR";
    case ISD::XOR:           return "XOR";
    case ISD::SHL:           return "SHL";
    case ISD::SHR:           return "SHR";
    case ISD::SIGN_EXTEND:   return "SIGN_EXTEND";
    case ISD::ZERO_EXTEND:   return "ZERO_EXTEND";
    case ISD::TRUNCATE:      return "TRUNCATE";
    case ISD::SETCC:         return "SETCC";
    case ISD::CALL:          return "CALL";
    case ISD::TokenFactor:   return "TokenFactor";
    case ISD::AssertSext:    return "AssertSext";
    case ISD::AssertZext:    return "AssertZext";
    case ISD::ADJCALLSTACKDOWN: return "ADJCALLSTACKDOWN";
    case ISD::ADJCALLSTACKUP:   return "ADJCALLSTACKUP";
    default: return "UNKNOWN";
    }
}

} // namespace ll1
