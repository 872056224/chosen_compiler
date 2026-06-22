#ifndef LL1_CODEGEN_TARGET8086_TARGET8086LOWERING_H
#define LL1_CODEGEN_TARGET8086_TARGET8086LOWERING_H

#include <ll1/CodeGen/Target/TargetLowering.h>

namespace ll1 {

// ============================================================
// Target8086Lowering — 8086-specific operation legalization
//
// Most 16-bit operations are Legal for 8086.
// Expansions:
//   - SREM → Expand (8086 idiv produces quotient in AX, remainder in DX)
//   - 8-bit operations → Promote to 16-bit
// ============================================================
class Target8086Lowering : public TargetLowering {
public:
    Target8086Lowering();

    const TargetRegisterClass* getRegClassForType(Type *Ty) const override;

    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) override;
};

} // namespace ll1
#endif
