#include <ll1/CodeGen/MachineInstr.h>
#include <sstream>

namespace ll1 {

MachineInstr::MachineInstr(MCOpcode Opc, uint32_t Flags)
    : Opc(Opc), Flags(Flags) {}

MachineInstr& MachineInstr::addReg(Register Reg, bool isDef) {
    Operands.push_back(MachineOperand::CreateReg(Reg, isDef));
    return *this;
}

MachineInstr& MachineInstr::addImm(int16_t Val) {
    Operands.push_back(MachineOperand::CreateImm(Val));
    return *this;
}

MachineInstr& MachineInstr::addFrameIndex(int FI) {
    Operands.push_back(MachineOperand::CreateFI(FI));
    return *this;
}

MachineInstr& MachineInstr::addMBB(MachineBasicBlock *MBB) {
    Operands.push_back(MachineOperand::CreateMBB(MBB));
    return *this;
}

MachineInstr& MachineInstr::addExternalSymbol(const std::string &Name) {
    Operands.push_back(MachineOperand::CreateES(Name));
    return *this;
}

MachineInstr& MachineInstr::addImplicitDef(Register Reg) {
    auto MO = MachineOperand::CreateReg(Reg, true, false, false);
    Operands.push_back(MO);
    return *this;
}

MachineInstr& MachineInstr::addImplicitUse(Register Reg) {
    auto MO = MachineOperand::CreateReg(Reg, false, false, true);
    Operands.push_back(MO);
    return *this;
}

std::string MachineInstr::toString() const {
    std::ostringstream oss;
    oss << getMCOpcodeName(Opc);
    for (unsigned i = 0; i < getNumOperands(); ++i) {
        oss << " ";
        const auto &op = getOperand(i);
        switch (op.getType()) {
        case MachineOperandType::MO_Register:
            oss << (op.isDef() ? "def:" : "") << "vreg" << op.getReg();
            if (op.isKill()) oss << "(kill)";
            break;
        case MachineOperandType::MO_Immediate:
            oss << op.getImm();
            break;
        case MachineOperandType::MO_FrameIndex:
            oss << "fi#" << op.getFrameIndex();
            break;
        case MachineOperandType::MO_MachineBasicBlock:
            oss << "mbb<" << (void*)op.getMBB() << ">";
            break;
        case MachineOperandType::MO_ExternalSymbol:
            oss << "@" << op.getSymbolName();
            break;
        }
    }
    return oss.str();
}

} // namespace ll1
