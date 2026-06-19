#include <ll1/CodeGen/CodeGen.h>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace ll1 {

const char *CodeGen::TempRegs[] = {"ax", "bx", "cx", "dx"};
const int CodeGen::NumTempRegs = 4;

CodeGen::CodeGen() {}

std::string CodeGen::allocTempReg() {
    std::string r = TempRegs[NextTempReg];
    NextTempReg = (NextTempReg + 1) % NumTempRegs;
    return r;
}

void CodeGen::freeTempReg(const std::string &) {
    // Simple strategy: always cycle through regs, no tracking needed
}

std::string CodeGen::memOp(const std::string &base, int offset) {
    std::ostringstream ss;
    if (offset == 0) ss << "[" << base << "]";
    else if (offset > 0) ss << "[" << base << "+" << offset << "]";
    else ss << "[" << base << offset << "]";
    return ss.str();
}

std::string CodeGen::memOpBP(int offset) {
    return memOp("bp", offset);
}

std::string CodeGen::getVReg(Value *v) {
    auto it = State.VRegNames.find(v);
    if (it != State.VRegNames.end()) return it->second;
    std::string name = "t" + std::to_string(State.NextVReg++);
    State.VRegNames[v] = name;
    return name;
}

std::string CodeGen::getOperand(Value *v) {
    if (!v) return "0";

    // Constant
    if (auto *ci = dynamic_cast<ConstantInt*>(v)) {
        return std::to_string(ci->getValue());
    }

    // Alloca: compute stack offset
    if (auto *ai = dynamic_cast<AllocaInst*>(v)) {
        if (State.AllocaOffsets.count(v)) {
            return memOpBP(State.AllocaOffsets[v]);
        }
        // First time: allocate stack space
        int size = 2; // i16 = 2 bytes
        if (ai->getAllocatedType() == Type::getInt8Ty()) size = 1;
        State.StackOffset -= size;
        State.AllocaOffsets[v] = State.StackOffset;
        State.MF.StackSize = std::max(State.MF.StackSize, -State.StackOffset);
        return memOpBP(State.StackOffset);
    }

    // Label (BasicBlock)
    if (auto *bb = dynamic_cast<BasicBlock*>(v)) {
        return State.BlockLabels[bb];
    }

    // Virtual register
    return getVReg(v);
}

void CodeGen::emit(const std::string &opcode, const std::string &operands, const std::string &comment) {
    std::string line = "    " + opcode;
    if (!operands.empty()) {
        line += " ";
        // Pad for alignment
        if (opcode.length() < 4) line += "\t";
        line += operands;
    }
    if (!comment.empty()) {
        line += "  ; " + comment;
    }
    Out << line << "\n";
    // Also store in the current MachineBB for emitAssembly
    if (!State.MF.Blocks.empty()) {
        State.MF.Blocks.back().TextLines.push_back(line);
    }
}

// ====== Machine code generation ======
MachineModule CodeGen::generate(Module &mod) {
    MachineModule mm;

    for (auto &fn : mod.getFunctionList()) {
        State = FuncState();
        State.MF.Name = fn->getName();
        generateFunction(*fn);
        mm.Functions.push_back(State.MF);
    }

    return mm;
}

void CodeGen::generateFunction(Function &fn) {
    // Assign labels
    for (auto &bb : fn.getBasicBlocks()) {
        std::string label = "." + fn.getName() + "_" + bb->getName();
        State.BlockLabels[bb.get()] = label;
    }

    // Generate each BB
    for (auto &bb : fn.getBasicBlocks()) {
        generateBB(*bb);
    }
}

void CodeGen::generateBB(BasicBlock &bb) {
    MachineBB mbb;
    mbb.Label = State.BlockLabels[&bb];
    State.MF.Blocks.push_back(mbb);

    for (auto &inst : bb.getInstList()) {
        CurrentLabel = 0; // reset temp label counter per inst
        generateInst(*inst);
    }
}

void CodeGen::generateInst(Instruction &inst) {
    switch (inst.getOpcode()) {
    case Instruction::Opcode::Alloca: {
        // Handled lazily in getOperand — just allocate stack slot
        auto *ai = static_cast<AllocaInst*>(&inst);
        getOperand(ai); // triggers allocation
        break;
    }

    case Instruction::Opcode::Store: {
        auto *si = static_cast<StoreInst*>(&inst);
        std::string src = getOperand(si->getValue());
        std::string dst = getOperand(si->getPointer());

        if (src.find('[') == std::string::npos && dst.find('[') != std::string::npos) {
            // reg/imm → mem: need temp reg
            std::string r = allocTempReg();
            if (isdigit(src[0]) || src[0] == '-') {
                emit("mov", r + ", " + src);
            } else {
                emit("mov", r + ", " + src);
            }
            emit("mov", dst + ", " + r);
        } else {
            emit("mov", dst + ", " + src);
        }
        break;
    }

    case Instruction::Opcode::Load: {
        auto *li = static_cast<LoadInst*>(&inst);
        std::string ptr = getOperand(li->getPointer());
        std::string dst = getVReg(&inst);
        if (ptr.find('[') != std::string::npos) {
            std::string r = allocTempReg();
            emit("mov", r + ", " + ptr);
            emit("mov", dst + ", " + r);
        } else {
            emit("mov", dst + ", " + ptr);
        }
        break;
    }

    case Instruction::Opcode::Add: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhs = getOperand(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        std::string r = allocTempReg();
        // mov r, lhs
        if (isdigit(lhs[0]) || lhs[0] == '-') emit("mov", r + ", " + lhs);
        else emit("mov", r + ", " + lhs);
        // add r, rhs
        emit("add", r + ", " + rhs);
        State.VRegNames[&inst] = r;
        break;
    }

    case Instruction::Opcode::Sub: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string r = allocTempReg();
        std::string lhs = getOperand(bo->getLHS());
        if (isdigit(lhs[0]) || lhs[0] == '-') emit("mov", r + ", " + lhs);
        else emit("mov", r + ", " + lhs);
        std::string rhs = getOperand(bo->getRHS());
        emit("sub", r + ", " + rhs);
        State.VRegNames[&inst] = r;
        break;
    }

    case Instruction::Opcode::Mul: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhs = getOperand(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        // mov ax, lhs
        if (isdigit(lhs[0]) || lhs[0] == '-') emit("mov", "ax, " + lhs);
        else emit("mov", "ax, " + lhs);
        // mul rhs → result in ax (or dx:ax)
        if (isdigit(rhs[0]) || rhs[0] == '-') {
            std::string r = allocTempReg();
            emit("mov", r + ", " + rhs);
            emit("mul", r);
        } else {
            emit("mul", rhs);
        }
        State.VRegNames[&inst] = "ax";
        break;
    }

    case Instruction::Opcode::SDiv: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhs = getOperand(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        emit("mov", "ax, " + lhs);
        emit("cwd", ""); // sign-extend ax → dx:ax
        if (isdigit(rhs[0]) || rhs[0] == '-') {
            std::string r = allocTempReg();
            emit("mov", r + ", " + rhs);
            emit("idiv", r);
        } else {
            emit("idiv", rhs);
        }
        State.VRegNames[&inst] = "ax";
        break;
    }

    case Instruction::Opcode::SRem: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhs = getOperand(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        emit("mov", "ax, " + lhs);
        emit("cwd", "");
        if (isdigit(rhs[0]) || rhs[0] == '-') {
            std::string r = allocTempReg();
            emit("mov", r + ", " + rhs);
            emit("idiv", r);
        } else {
            emit("idiv", rhs);
        }
        State.VRegNames[&inst] = "dx"; // remainder in dx
        break;
    }

    case Instruction::Opcode::ICmp: {
        auto *ci = static_cast<ICmpInst*>(&inst);
        std::string lhs = getOperand(ci->getOperand(0));
        std::string rhs = getOperand(ci->getOperand(1));
        std::string r = allocTempReg();
        // mov r, lhs; cmp r, rhs
        if (isdigit(lhs[0]) || lhs[0] == '-') emit("mov", r + ", " + lhs);
        else emit("mov", r + ", " + lhs);
        emit("cmp", r + ", " + rhs);
        // Result goes to a virtual reg — the branch handles it
        State.VRegNames[&inst] = r;  // value is in flags, handled by branch
        break;
    }

    case Instruction::Opcode::Br: {
        auto *bi = static_cast<BranchInst*>(&inst);
        if (bi->isConditional()) {
            // Conditional branch: needs cmp before it
            // The condition's ICmp result sets flags
            // Map predicate to jump instruction
            // Find the ICmp that produced the condition
            auto *cond = bi->getCondition();
            std::string trueLabel = State.BlockLabels[bi->getTrueDest()];
            std::string falseLabel = State.BlockLabels[bi->getFalseDest()];

            if (auto *ci = dynamic_cast<ICmpInst*>(cond)) {
                auto pred = ci->getPredicate();
                switch (pred) {
                case ICmpInst::EQ:  emit("je", trueLabel); break;
                case ICmpInst::NE:  emit("jne", trueLabel); break;
                case ICmpInst::SLT: emit("jl", trueLabel); break;
                case ICmpInst::SLE: emit("jle", trueLabel); break;
                case ICmpInst::SGT: emit("jg", trueLabel); break;
                case ICmpInst::SGE: emit("jge", trueLabel); break;
                }
                emit("jmp", falseLabel);
            } else {
                // Generic: test cond, jnz true
                std::string cr = getOperand(cond);
                emit("cmp", cr + ", 0");
                emit("jne", trueLabel);
                emit("jmp", falseLabel);
            }
        } else {
            emit("jmp", State.BlockLabels[bi->getUnconditionalDest()]);
        }
        break;
    }

    case Instruction::Opcode::Ret: {
        auto *ri = static_cast<RetInst*>(&inst);
        if (ri->getReturnValue()) {
            std::string val = getOperand(ri->getReturnValue());
            if (isdigit(val[0]) || val[0] == '-')
                emit("mov", "ax, " + val);
            else
                emit("mov", "ax, " + val);
        }
        emit("ret", "");
        break;
    }

    case Instruction::Opcode::Call: {
        // For MVP, call is simplified — just emit call <label>
        auto *ci = static_cast<CallInst*>(&inst);
        // The callee is a Function*
        if (auto *fn = dynamic_cast<Function*>(ci->getOperand(0))) {
            emit("call", fn->getName());
        }
        break;
    }

    case Instruction::Opcode::And:
    case Instruction::Opcode::Or:
        // Bitwise ops — for now, treat as arithmetic
        break;

    default:
        break;
    }
}

// ====== Assembly emission ======
std::string CodeGen::emitAssembly(const MachineModule &mm) {
    std::ostringstream asmOut;

    // Data section
    if (!mm.DataSection.empty()) {
        asmOut << ".data\n";
        for (auto &d : mm.DataSection) {
            asmOut << d << "\n";
        }
    }

    // Code section
    asmOut << ".code\n";
    for (auto &fn : mm.Functions) {
        asmOut << "\n; Function: " << fn.Name << "\n";

        // Function prologue
        asmOut << fn.Name << " proc\n";
        if (fn.StackSize > 0) {
            asmOut << "    push bp\n";
            asmOut << "    mov  bp, sp\n";
            asmOut << "    sub  sp, " << fn.StackSize << "\n";
        }

        // Blocks
        for (auto &bb : fn.Blocks) {
            // Emit label if it's not just a fall-through
            bool needLabel = true;
            if (&bb == &fn.Blocks.front() && bb.Label.find("entry") != std::string::npos) {
                needLabel = false; // First entry block doesn't need explicit label
            }
            if (needLabel && bb.Label.size() > 0) {
                asmOut << bb.Label << ":\n";
            }
            // Output raw text lines from emit()
            for (auto &line : bb.TextLines) {
                asmOut << line << "\n";
            }
        }

        // Function epilogue
        if (fn.StackSize > 0) {
            asmOut << "    mov  sp, bp\n";
            asmOut << "    pop  bp\n";
        }
        asmOut << fn.Name << " endp\n";
    }

    asmOut << "\nend\n";
    return asmOut.str();
}

} // namespace ll1
