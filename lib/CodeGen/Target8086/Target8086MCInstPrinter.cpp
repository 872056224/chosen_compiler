#include <ll1/CodeGen/Target8086/Target8086MCInstPrinter.h>
#include <ll1/CodeGen/Target/TargetRegisterInfo.h>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace ll1 {

Target8086MCInstPrinter::Target8086MCInstPrinter(const TargetRegisterInfo &tri)
    : TRI(tri) {}

void Target8086MCInstPrinter::setRegMapping(
    const std::unordered_map<Register, Register> &V2P) {
    VRegToPhysReg = V2P;
}

void Target8086MCInstPrinter::setSpillSlots(
    const std::unordered_map<Register, int> &Slots) {
    SpillSlots = Slots;
}

std::string Target8086MCInstPrinter::getRegName(Register Reg) const {
    if (isVirtualRegister(Reg)) {
        // Look up physical mapping
        auto it = VRegToPhysReg.find(Reg);
        if (it != VRegToPhysReg.end()) {
            return TRI.getName(it->second);
        }
        return "vreg" + std::to_string(virtReg2Index(Reg));
    }
    return TRI.getName(Reg);
}

void Target8086MCInstPrinter::emitLine(const std::string &opcode,
                                        const std::string &operands,
                                        const std::string &comment) {
    std::string line = "    " + opcode;
    if (!operands.empty()) {
        // Pad opcode to 4 chars for alignment
        while (line.size() < 8) line += ' ';
        line += operands;
    }
    if (!comment.empty()) {
        line += "  ; " + comment;
    }
    Out << line << "\n";
}

std::string Target8086MCInstPrinter::printOperand(const MachineOperand &MO,
                                                    bool isByte) {
    (void)isByte;
    switch (MO.getType()) {
    case MachineOperandType::MO_Register:
        return getRegName(MO.getReg());

    case MachineOperandType::MO_Immediate:
        return std::to_string(MO.getImm());

    case MachineOperandType::MO_FrameIndex: {
        // Resolve FrameIndex → [bx+offset]
        int fi = MO.getFrameIndex();
        if (MFI && fi >= 0 && fi < MFI->getNumObjects()) {
            int off = MFI->getObjectOffset(fi);
            if (off == 0) return "[bx]";
            return "[bx+" + std::to_string(off) + "]";
        }
        if (fi == 0) return "[bx]";
        if (fi < 0) return "[bx-" + std::to_string(-fi) + "]";
        return "[bx+" + std::to_string(fi) + "]";
    }

    case MachineOperandType::MO_MachineBasicBlock:
        return MO.getMBB() ? MO.getMBB()->getLabel() : "?";

    case MachineOperandType::MO_ExternalSymbol:
        return MO.getSymbolName();

    default:
        return "?";
    }
}

std::string Target8086MCInstPrinter::print(const MachineInstr &MI) {
    // We return the text for one instruction
    MCOpcode opc = MI.getOpcode();

    // Special: check for spill/reload address
    if (opc == MCOpcode::MOV && MI.getNumOperands() >= 2) {
        // Check for [bx+spill_offset] patterns
    }

    switch (opc) {
    // Terminators
    case MCOpcode::RET: {
        return "mov  sp, bp\n    pop  bp\n    ret";
    }
    case MCOpcode::JMP:
        return "jmp " + (MI.getNumOperands() > 0 ? printOperand(MI.getOperand(0)) : "?");
    case MCOpcode::JE: case MCOpcode::JNE:
    case MCOpcode::JL: case MCOpcode::JLE:
    case MCOpcode::JG: case MCOpcode::JGE: {
        std::string jmpName = getMCOpcodeName(opc);
        std::transform(jmpName.begin(), jmpName.end(), jmpName.begin(), ::tolower);
        return "    " + jmpName + " " + (MI.getNumOperands() > 0 ? printOperand(MI.getOperand(0)) : "?");
    }

    // Data movement
    case MCOpcode::MOV:
    case MCOpcode::MOVZX:
    case MCOpcode::MOVSX: {
        std::string mnemonic = (opc == MCOpcode::MOVZX || opc == MCOpcode::MOVSX) ? "mov" : "mov";
        // 3-operand indexed addressing: (Reg, FI, IndexReg) or (FI, IndexReg, SrcReg)
        if (MI.getNumOperands() >= 3) {
            auto &op0 = MI.getOperand(0);
            auto &op1 = MI.getOperand(1);
            auto &op2 = MI.getOperand(2);
            if (op0.getType() == MachineOperandType::MO_Register &&
                op1.getType() == MachineOperandType::MO_FrameIndex &&
                op2.getType() == MachineOperandType::MO_Register) {
                // Indexed load: mov destReg, [bx+offset+si]
                int fi = op1.getFrameIndex();
                std::string offset = "";
                if (MFI && fi >= 0 && fi < MFI->getNumObjects()) {
                    int off = MFI->getObjectOffset(fi);
                    offset = (off == 0) ? "" : "+" + std::to_string(off);
                }
                return mnemonic + " " + printOperand(op0) + ", [bx" + offset + "+" + getRegName(op2.getReg()) + "]";
            }
            if (op0.getType() == MachineOperandType::MO_FrameIndex &&
                op1.getType() == MachineOperandType::MO_Register &&
                op2.getType() == MachineOperandType::MO_Register) {
                // Indexed store: mov [bx+offset+si], srcReg
                int fi = op0.getFrameIndex();
                std::string offset = "";
                if (MFI && fi >= 0 && fi < MFI->getNumObjects()) {
                    int off = MFI->getObjectOffset(fi);
                    offset = (off == 0) ? "" : "+" + std::to_string(off);
                }
                return mnemonic + " [bx" + offset + "+" + getRegName(op1.getReg()) + "], " + printOperand(op2);
            }
        }
        if (MI.getNumOperands() >= 2) {
            std::string dst = printOperand(MI.getOperand(0), false);
            std::string src = printOperand(MI.getOperand(1), true);
            return mnemonic + " " + dst + ", " + src;
        }
        return mnemonic;
    }

    // Arithmetic
    case MCOpcode::ADD: case MCOpcode::SUB:
    case MCOpcode::AND: case MCOpcode::OR: case MCOpcode::XOR:
    case MCOpcode::CMP: {
        std::string mnem = getMCOpcodeName(opc);
        std::transform(mnem.begin(), mnem.end(), mnem.begin(), ::tolower);
        if (MI.getNumOperands() >= 2)
            return mnem + " " + printOperand(MI.getOperand(0)) + ", " + printOperand(MI.getOperand(1));
        return mnem;
    }

    case MCOpcode::MUL: case MCOpcode::DIV: case MCOpcode::IDIV:
    case MCOpcode::NEG: case MCOpcode::NOT: {
        std::string mnem = getMCOpcodeName(opc);
        std::transform(mnem.begin(), mnem.end(), mnem.begin(), ::tolower);
        if (MI.getNumOperands() > 0)
            return mnem + " " + printOperand(MI.getOperand(0));
        return mnem;
    }

    case MCOpcode::CWD:
        return "cwd";

    // Stack
    case MCOpcode::PUSH:
        return "push " + printOperand(MI.getOperand(0));
    case MCOpcode::POP:
        return "pop " + printOperand(MI.getOperand(0));

    // Subroutine
    case MCOpcode::CALL:
        return "call " + (MI.getNumOperands() > 0 ? printOperand(MI.getOperand(0)) : "?");

    // Shift
    case MCOpcode::SHL: case MCOpcode::SHR:
        if (MI.getNumOperands() >= 2)
            return "shl " + printOperand(MI.getOperand(0)) + ", " + printOperand(MI.getOperand(1));
        return "shl";

    // COPY — becomes MOV after reg allocation, or removed if coalesced
    case MCOpcode::COPY:
        return "; COPY (eliminated by coalescer)";

    // Pseudo
    case MCOpcode::ADJCALLSTACKDOWN:
    case MCOpcode::ADJCALLSTACKUP:
        return ""; // No code emitted

    case MCOpcode::LABEL:
        return MI.getNumOperands() > 0 ? printOperand(MI.getOperand(0)) + ":" : "?";

    case MCOpcode::HLT:
        return "hlt";

    default:
        return "; unknown opcode " + std::to_string((int)opc);
    }
}

// ============================================================
// Prologue
// ============================================================
std::string Target8086MCInstPrinter::printPrologue(const MachineFunction &MF) {
    std::ostringstream prologue;

    int localSize = MF.getFrameInfo().getStackSize();
    int totalSize = localSize + SpillSize;

    prologue << "    push bp\n";
    prologue << "    mov  bp, sp\n";
    if (totalSize > 0) {
        prologue << "    sub  sp, " << totalSize << "\n";
    }
    prologue << "    mov  bx, bp\n";
    if (totalSize > 0) {
        prologue << "    sub  bx, " << totalSize << "\n";
    }

    return prologue.str();
}

// ============================================================
// Epilogue
// ============================================================
std::string Target8086MCInstPrinter::printEpilogue(const MachineFunction &MF) {
    (void)MF;
    return "    mov  sp, bp\n    pop  bp\n";
}

// ============================================================
// Print whole function
// ============================================================
std::string Target8086MCInstPrinter::print(const MachineFunction &MF) {
    Out.str("");
    Out.clear();

    Out << "; Function: " << MF.getName() << "\n";
    Out << MF.getName() << ":\n";
    Out << printPrologue(MF);
    Out << "\n";

    auto &blocks = const_cast<MachineFunction&>(MF).getBasicBlocks();
    for (auto &mbb : blocks) {
        // Print label
        Out << mbb->getLabel() << ":\n";

        for (auto &mi : mbb->getInstList()) {
            std::string text = print(mi);
            if (!text.empty()) {
                Out << "    " << text << "\n";
            }
        }
        Out << "\n";
    }

    return Out.str();
}

// ============================================================
// Runtime library
// ============================================================
std::string Target8086MCInstPrinter::printRuntime() {
    Out.str("");
    Out.clear();
    Out << "\n"
        << "; --- Runtime: __print (AX = value to print) ---\n"
        << "__print:\n"
        << "    push bp\n"
        << "    push bx\n"
        << "    push cx\n"
        << "    push dx\n"
        << "    mov  cx, 0\n"
        << "    mov  bx, 10\n"
        << "__print_loop:\n"
        << "    mov  dx, 0\n"
        << "    div  bx\n"
        << "    push dx\n"
        << "    inc  cx\n"
        << "    cmp  ax, 0\n"
        << "    jne  __print_loop\n"
        << "__print_disp:\n"
        << "    pop  dx\n"
        << "    add  dl, '0'\n"
        << "    mov  ah, 02h\n"
        << "    int  21h\n"
        << "    loop __print_disp\n"
        << "    mov  dl, ' '\n"
        << "    mov  ah, 02h\n"
        << "    int  21h\n"
        << "    pop  dx\n"
        << "    pop  cx\n"
        << "    pop  bx\n"
        << "    pop  bp\n"
        << "    ret\n"
        << "\n"
        << "; --- Runtime: __read (returns in AX) ---\n"
        << "__read:\n"
        << "    push bp\n"
        << "    mov  bp, sp\n"
        << "    mov  bx, bp\n"
        << "    push bx\n"
        << "    push cx\n"
        << "    push dx\n"
        << "    mov  bx, 0\n"
        << "    mov  cx, 0\n"
        << "__read_loop:\n"
        << "    mov  ah, 01h\n"
        << "    int  21h\n"
        << "    cmp  al, 0Dh\n"
        << "    je   __read_done\n"
        << "    sub  al, '0'\n"
        << "    mov  cl, al\n"
        << "    mov  ch, 0\n"
        << "    mov  ax, bx\n"
        << "    mov  dx, 10\n"
        << "    mul  dx\n"
        << "    add  ax, cx\n"
        << "    mov  bx, ax\n"
        << "    jmp  __read_loop\n"
        << "__read_done:\n"
        << "    mov  ax, bx\n"
        << "    pop  dx\n"
        << "    pop  cx\n"
        << "    pop  bx\n"
        << "    pop  bp\n"
        << "    ret\n";
    return Out.str();
}

} // namespace ll1
