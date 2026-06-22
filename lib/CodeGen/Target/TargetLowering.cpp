#include <ll1/CodeGen/Target/TargetLowering.h>
#include <ll1/CodeGen/SelectionDAG/SDNode.h>
#include <ll1/CodeGen/SelectionDAG/SelectionDAG.h>

namespace ll1 {

void TargetLowering::setOperationAction(unsigned Op, LegalizeAction Action) {
    OpActions[Op] = Action;
}

LegalizeAction TargetLowering::getOperationAction(unsigned Op) const {
    auto it = OpActions.find(Op);
    if (it != OpActions.end()) return it->second;
    return LegalizeAction::Legal;
}

SDValue TargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) {
    // Default: no lowering — return empty SDValue
    return SDValue();
}

const TargetRegisterClass* TargetLowering::getRegClassForType(Type *Ty) const {
    return nullptr;
}

} // namespace ll1
