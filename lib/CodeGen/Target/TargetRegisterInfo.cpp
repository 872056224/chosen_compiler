#include <ll1/CodeGen/Target/TargetRegisterInfo.h>

namespace ll1 {

bool TargetRegisterClass::contains(Register Reg) const {
    for (auto r : Regs)
        if (r == Reg) return true;
    return false;
}

const TargetRegisterClass* TargetRegisterInfo::getRegClassById(unsigned Id) const {
    auto &rcs = regClasses();
    if (Id < rcs.size()) return &rcs[Id];
    return nullptr;
}

} // namespace ll1
