#ifndef LL1_CODEGEN_SELECTIONDAG_SDNODE_H
#define LL1_CODEGEN_SELECTIONDAG_SDNODE_H

#include <ll1/CodeGen/SelectionDAG/ISDOpcodes.h>
#include <ll1/CodeGen/Register.h>
#include <ll1/IR/Type.h>
#include <vector>
#include <cstdint>

namespace ll1 {

class MachineBasicBlock;

// ============================================================
// SDNodeFlags — Per-node arithmetic flags
// ============================================================
struct SDNodeFlags {
    bool NSW   : 1;   // No Signed Wrap
    bool NUW   : 1;   // No Unsigned Wrap
    bool Exact : 1;   // Exact division
    uint16_t Reserved : 13;

    SDNodeFlags() : NSW(false), NUW(false), Exact(false), Reserved(0) {}
};

// ============================================================
// SDValue — Lightweight reference to a DAG node + result number
// ============================================================
class SDNode; // forward

class SDValue {
    SDNode *Node = nullptr;
    unsigned ResNo = 0;

public:
    SDValue() = default;
    SDValue(SDNode *N, unsigned R = 0) : Node(N), ResNo(R) {}

    SDNode* getNode() const { return Node; }
    unsigned getResNo() const { return ResNo; }

    // Delegates to SDNode
    unsigned getOpcode() const;
    unsigned getNumOperands() const;
    SDValue getOperand(unsigned i) const;
    Type* getValueType() const;

    // Type checking
    Type* getType() const { return getValueType(); }

    // Validity
    bool isValid() const { return Node != nullptr; }
    explicit operator bool() const { return isValid(); }

    // Comparison (for map/set usage)
    bool operator==(const SDValue &O) const { return Node == O.Node && ResNo == O.ResNo; }
    bool operator!=(const SDValue &O) const { return !(*this == O); }
    bool operator<(const SDValue &O) const {
        return Node < O.Node || (Node == O.Node && ResNo < O.ResNo);
    }
};

// ============================================================
// SDUse — Operand link in an SDNode's operand list
// ============================================================
class SDUse {
    SDValue Val;
    SDNode *User = nullptr;

public:
    SDUse() = default;
    SDUse(const SDValue &V, SDNode *U) : Val(V), User(U) {}

    SDValue get() const { return Val; }
    SDNode* getUser() const { return User; }
    void set(const SDValue &V) { Val = V; }
    void setUser(SDNode *U) { User = U; }

    SDValue& valRef() { return Val; }
};

// ============================================================
// SDNode — One node in the SelectionDAG
// ============================================================
class SDNode {
public:
    SDNode(unsigned Opc, unsigned NumOps, unsigned NumVals, Type *Ty);

    unsigned getOpcode() const { return Opc; }
    unsigned getNumOperands() const { return NumOperands; }
    unsigned getNumValues() const { return NumValues; }

    SDValue getOperand(unsigned i) const {
        return OperandList[i].get();
    }

    Type* getValueType(unsigned ResNo = 0) const {
        if (ResNo < NumValues) return ResultTypes[ResNo];
        return nullptr;
    }

    // Payload accessors
    int16_t getConstant() const { return Payload.ConstVal; }
    void setConstant(int16_t v) { Payload.ConstVal = v; }
    int getFrameIndex() const { return Payload.FrameIdx; }
    void setFrameIndex(int v) { Payload.FrameIdx = v; }
    const char* getCallee() const { return Payload.CallCallee; }
    void setCallee(const char* c) { Payload.CallCallee = c; }
    MachineBasicBlock* getTargetMBB() const { return Payload.TargetMBB; }
    void setTargetMBB(MachineBasicBlock* b) { Payload.TargetMBB = b; }

    // Flags
    const SDNodeFlags& getFlags() const { return Flags; }
    SDNodeFlags& getFlags() { return Flags; }
    void setFlags(const SDNodeFlags &F) { Flags = F; }

    // Operand management
    void initOperand(unsigned i, const SDValue &V);
    void initOperands(const std::vector<SDValue> &Ops);

    // Hash for CSE
    unsigned computeHash() const;

    // Debug
    const char* getOpcodeName() const;

    // Check opcode category
    bool isMachineOpcode() const { return ISD::isMachineOpcode(Opc); }

    // Payload data
    union {
        int16_t ConstVal;
        int FrameIdx;
        Register RegNum;
        const char* CallCallee;
        MachineBasicBlock* TargetMBB;
        uint8_t CmpPred;     // ICmpInst::Pred for SETCC nodes
    } Payload;

private:
    unsigned Opc;
    uint16_t NumOperands;
    uint16_t NumValues;
    SDNodeFlags Flags;

    // Variable-length arrays (trailing)
    SDUse *OperandList;   // Array of NumOperands SDUse objects
    Type **ResultTypes;   // Array of NumValues Type pointers
};

} // namespace ll1

#endif
