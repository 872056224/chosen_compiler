#include <ll1/CodeGen/Target8086/Target8086Lowering.h>
#include <ll1/CodeGen/Target8086/Target8086RegisterInfo.h>
#include <ll1/CodeGen/SelectionDAG/SDNode.h>
#include <ll1/CodeGen/SelectionDAG/SelectionDAG.h>
#include <ll1/IR/Type.h>

namespace ll1 {

// Singleton for register class access
static Target8086RegisterInfo The8086TRI;

Target8086Lowering::Target8086Lowering() {
    setOperationAction(ISD::SREM, LegalizeAction::Expand);
}

const TargetRegisterClass* Target8086Lowering::getRegClassForType(Type *Ty) const {
    if (!Ty) return &The8086TRI.getGR16ABCDClass();
    // i16 and i1 → GR16_ABCD
    if (Ty == Type::getInt16Ty() || Ty == Type::getInt1Ty()) {
        return &The8086TRI.getGR16ABCDClass();
    }
    // i8 → GR8 (for now, also use GR16)
    if (Ty == Type::getInt8Ty()) {
        return &The8086TRI.getGR16ABCDClass();
    }
    // Void → no register needed
    if (Ty == Type::getVoidTy()) return nullptr;
    // Default: GR16
    return &The8086TRI.getGR16ABCDClass();
}

SDValue Target8086Lowering::LowerOperation(SDValue Op, SelectionDAG &DAG) {
    switch (Op.getOpcode()) {
    default:
        break;
    }
    return SDValue();
}

} // namespace ll1
