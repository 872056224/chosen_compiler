#ifndef LL1_CODEGEN_TARGET_TARGETLOWERING_H
#define LL1_CODEGEN_TARGET_TARGETLOWERING_H

#include <map>
#include <cstdint>

namespace ll1 {

// Forward declarations (defined in ISDOpcodes.h)
// ISD opcodes are unsigned (defined in ISDOpcodes.h)
class SDValue;
class SelectionDAG;
class TargetRegisterClass;
class Type;

// ============================================================
// LegalizeAction — How to handle an operation on a given type
// ============================================================
enum class LegalizeAction : uint8_t {
    Legal,    // Target natively supports this operation
    Expand,   // Expand to simpler operations
    Custom,   // Call LowerOperation() to handle
};

// ============================================================
// TargetLowering — Abstract target-specific lowering interface
// ============================================================
class TargetLowering {
public:
    virtual ~TargetLowering() = default;

    // Set the action for a given operation.
    void setOperationAction(unsigned Op, LegalizeAction Action);

    // Get the action for a given operation.
    LegalizeAction getOperationAction(unsigned Op) const;

    // Lower a custom operation (override in target).
    virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG);

    // Return the register class for a given type.
    virtual const TargetRegisterClass* getRegClassForType(Type *Ty) const;

    // Get the maximum number of bits the target can represent.
    virtual unsigned getMaxBits() const { return 16; }

protected:
    std::map<unsigned, LegalizeAction> OpActions;
};

} // namespace ll1
#endif
