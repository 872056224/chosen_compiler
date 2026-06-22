#ifndef LL1_CODEGEN_TARGET8086_TARGET8086MCINSTPRINTER_H
#define LL1_CODEGEN_TARGET8086_TARGET8086MCINSTPRINTER_H

#include <ll1/CodeGen/MachineFunction.h>
#include <ll1/CodeGen/MachineInstr.h>
#include <ll1/CodeGen/MachineFrameInfo.h>
#include <string>
#include <unordered_map>
#include <sstream>

namespace ll1 {

class TargetRegisterInfo;

class Target8086MCInstPrinter {
public:
    explicit Target8086MCInstPrinter(const TargetRegisterInfo &TRI);

    void setRegMapping(const std::unordered_map<Register, Register> &V2P);
    void setSpillSlots(const std::unordered_map<Register, int> &Slots);
    void setFrameInfo(const MachineFrameInfo &FI) { MFI = &FI; }

    std::string print(const MachineFunction &MF);
    std::string print(const MachineInstr &MI);
    std::string getRegName(Register Reg) const;
    std::string printPrologue(const MachineFunction &MF);
    std::string printEpilogue(const MachineFunction &MF);
    void printRuntime();

private:
    const TargetRegisterInfo &TRI;
    std::unordered_map<Register, Register> VRegToPhysReg;
    std::unordered_map<Register, int> SpillSlots;
    const MachineFrameInfo *MFI = nullptr;
    std::ostringstream Out;

    std::string printOperand(const MachineOperand &MO, bool isByte = false);
    void emitLine(const std::string &opcode, const std::string &operands = "",
                  const std::string &comment = "");
};

} // namespace ll1
#endif
