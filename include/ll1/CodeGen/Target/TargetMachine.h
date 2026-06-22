#ifndef LL1_CODEGEN_TARGET_TARGETMACHINE_H
#define LL1_CODEGEN_TARGET_TARGETMACHINE_H

#include <memory>

namespace ll1 {

class TargetRegisterInfo;
class TargetInstrInfo;
class TargetLowering;

// ============================================================
// TargetMachine — Factory for target-specific components
// ============================================================
class TargetMachine {
public:
    virtual ~TargetMachine() = default;

    virtual const TargetRegisterInfo& getRegInfo() const = 0;
    virtual const TargetInstrInfo& getInstrInfo() const = 0;
    virtual TargetLowering& getTargetLowering() = 0;
    virtual const TargetLowering& getTargetLowering() const = 0;

    virtual const char* getTargetTriple() const = 0;
    virtual const char* getTargetName() const = 0;
};

} // namespace ll1
#endif
