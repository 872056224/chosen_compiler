#ifndef LL1_CODEGEN_TARGET8086_TARGET8086_H
#define LL1_CODEGEN_TARGET8086_TARGET8086_H

#include <ll1/CodeGen/Target/TargetMachine.h>
#include <ll1/CodeGen/Target8086/Target8086RegisterInfo.h>
#include <ll1/CodeGen/Target8086/Target8086InstrInfo.h>
#include <ll1/CodeGen/Target8086/Target8086Lowering.h>

namespace ll1 {

class Target8086Machine : public TargetMachine {
public:
    Target8086Machine();

    const TargetRegisterInfo& getRegInfo() const override { return RegInfo; }
    const TargetInstrInfo& getInstrInfo() const override { return InstrInfo; }
    TargetLowering& getTargetLowering() override { return Lowering; }
    const TargetLowering& getTargetLowering() const override { return Lowering; }

    const char* getTargetTriple() const override { return "x86-16-unknown"; }
    const char* getTargetName() const override { return "8086"; }

private:
    Target8086RegisterInfo RegInfo;
    Target8086InstrInfo InstrInfo;
    Target8086Lowering Lowering;
};

} // namespace ll1
#endif
