#include <ll1/CodeGen/MachineOperand.h>
#include <cassert>

namespace ll1 {

MachineOperand MachineOperand::CreateReg(Register Reg, bool isDef, bool isDead, bool isKill) {
    MachineOperand MO;
    MO.Kind = MachineOperandType::MO_Register;
    MO.Reg = Reg;
    MO.RegFlags = (isDef ? 1 : 0) | (isDead ? 2 : 0) | (isKill ? 4 : 0);
    return MO;
}

MachineOperand MachineOperand::CreateImm(int16_t Val) {
    MachineOperand MO;
    MO.Kind = MachineOperandType::MO_Immediate;
    MO.ImmVal = Val;
    return MO;
}

MachineOperand MachineOperand::CreateFI(int Index) {
    MachineOperand MO;
    MO.Kind = MachineOperandType::MO_FrameIndex;
    MO.FrameIdx = Index;
    return MO;
}

MachineOperand MachineOperand::CreateMBB(MachineBasicBlock *MBB) {
    MachineOperand MO;
    MO.Kind = MachineOperandType::MO_MachineBasicBlock;
    MO.MBB = MBB;
    return MO;
}

MachineOperand MachineOperand::CreateES(const std::string &Name) {
    MachineOperand MO;
    MO.Kind = MachineOperandType::MO_ExternalSymbol;
    MO.SymbolName = Name;
    return MO;
}

Register MachineOperand::getReg() const {
    assert(Kind == MachineOperandType::MO_Register);
    return Reg;
}

int16_t MachineOperand::getImm() const {
    assert(Kind == MachineOperandType::MO_Immediate);
    return ImmVal;
}

int MachineOperand::getFrameIndex() const {
    assert(Kind == MachineOperandType::MO_FrameIndex);
    return FrameIdx;
}

MachineBasicBlock* MachineOperand::getMBB() const {
    assert(Kind == MachineOperandType::MO_MachineBasicBlock);
    return MBB;
}

const std::string& MachineOperand::getSymbolName() const {
    assert(Kind == MachineOperandType::MO_ExternalSymbol);
    return SymbolName;
}

bool MachineOperand::isDef() const  { return RegFlags & 1; }
bool MachineOperand::isDead() const { return RegFlags & 2; }
bool MachineOperand::isKill() const { return RegFlags & 4; }

void MachineOperand::setIsDef(bool v)  { if (v) RegFlags |= 1; else RegFlags &= ~1; }
void MachineOperand::setIsDead(bool v) { if (v) RegFlags |= 2; else RegFlags &= ~2; }
void MachineOperand::setIsKill(bool v) { if (v) RegFlags |= 4; else RegFlags &= ~4; }

void MachineOperand::setReg(Register R) { Reg = R; }

} // namespace ll1
