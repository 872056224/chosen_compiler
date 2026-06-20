#include <ll1/CodeGen/CodeGen.h>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace ll1 {

// BX reserved as base pointer, only AX/CX/DX for temps
const char *CodeGen::TempRegs[] = {"ax", "cx", "dx"};
const int CodeGen::NumTempRegs = 3;

CodeGen::CodeGen() {}

std::string CodeGen::allocTempReg() {
    std::string r = TempRegs[NextTempReg];
    NextTempReg = (NextTempReg + 1) % NumTempRegs;
    return r;
}

// Each alloca gets a [bx+offset] memory operand.
// BX is the dedicated base pointer, initialized at function entry.
std::string CodeGen::varLabel(Value *v) {
    auto it = State.VarOffsets.find(v);
    if (it != State.VarOffsets.end()) {
        int off = it->second;
        if (off == 0) return "[bx]";
        return "[bx+" + std::to_string(off) + "]";
    }
    int off = State.NextVarOffset;
    State.NextVarOffset += 2;    // 16-bit variables
    State.VarOffsets[v] = off;
    if (off == 0) return "[bx]";
    return "[bx+" + std::to_string(off) + "]";
}

std::string CodeGen::assignReg(Value *v) {
    auto it = State.VRegNames.find(v);
    if (it != State.VRegNames.end()) return it->second;
    std::string r = allocTempReg();
    State.VRegNames[v] = r;
    return r;
}

std::string CodeGen::getReg(Value *v) {
    auto it = State.VRegNames.find(v);
    if (it != State.VRegNames.end()) return it->second;
    return "";
}

// Check if string looks like an immediate (number, possibly negative)
static bool isImmediate(const std::string &s) {
    if (s.empty()) return false;
    if (s[0] == '-') return std::isdigit(s[1]);
    return std::isdigit(s[0]);
}

// Check if string is a real 8086 register name
static bool isRealReg(const std::string &s) {
    return s == "ax" || s == "bx" || s == "cx" || s == "dx" ||
           s == "si" || s == "di" || s == "bp" || s == "sp";
}

std::string CodeGen::getOperand(Value *v) {
    if (!v) return "0";

    // Constant → immediate
    if (auto *ci = dynamic_cast<ConstantInt*>(v)) {
        return std::to_string(ci->getValue());
    }

    // Alloca → data label (no brackets)
    if (dynamic_cast<AllocaInst*>(v)) {
        return varLabel(v);
    }

    // BasicBlock → code label
    if (auto *bb = dynamic_cast<BasicBlock*>(v)) {
        return State.BlockLabels[bb];
    }

    // Instruction result → lookup assigned physical register
    std::string r = getReg(v);
    if (!r.empty()) return r;

    return assignReg(v);
}

// Load a value into a real register, return the register name
std::string CodeGen::loadToReg(Value *v) {
    if (auto *ci = dynamic_cast<ConstantInt*>(v)) {
        std::string r = allocTempReg();
        emit("mov", r + ", " + std::to_string(ci->getValue()));
        return r;
    }

    std::string op = getOperand(v);

    // Already a real register
    if (isRealReg(op)) return op;

    // Immediate → load into register
    if (isImmediate(op)) {
        std::string r = allocTempReg();
        emit("mov", r + ", " + op);
        return r;
    }

    // Data label (memory) → load into register
    std::string r = allocTempReg();
    emit("mov", r + ", " + op);
    return r;
}

void CodeGen::emit(const std::string &opcode, const std::string &operands, const std::string &comment) {
    std::string line = "    " + opcode;
    if (!operands.empty()) {
        line += "  ";
        line += operands;
    }
    (void)comment;
    Out << line << "\n";

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
    for (auto &bb : fn.getBasicBlocks()) {
        std::string label = fn.getName() + "_" + bb->getName();
        State.BlockLabels[bb.get()] = label;
    }

    for (auto &bb : fn.getBasicBlocks()) {
        generateBB(*bb);
    }
}

void CodeGen::generateBB(BasicBlock &bb) {
    MachineBB mbb;
    mbb.Label = State.BlockLabels[&bb];
    State.MF.Blocks.push_back(mbb);

    for (auto &inst : bb.getInstList()) {
        generateInst(*inst);
    }
}

void CodeGen::generateInst(Instruction &inst) {
    switch (inst.getOpcode()) {

    case Instruction::Opcode::Alloca: {
        varLabel(&inst);    // register variable label
        assignReg(&inst);
        break;
    }

    case Instruction::Opcode::Store: {
        auto *si = static_cast<StoreInst*>(&inst);
        Value *srcVal = si->getValue();
        std::string dst = getOperand(si->getPointer()); // data label

        std::string srcReg;
        if (auto *ci = dynamic_cast<ConstantInt*>(srcVal)) {
            srcReg = allocTempReg();
            emit("mov", srcReg + ", " + std::to_string(ci->getValue()));
        } else {
            std::string srcOp = getOperand(srcVal);
            if (isRealReg(srcOp)) {
                srcReg = srcOp;
            } else if (isImmediate(srcOp)) {
                srcReg = allocTempReg();
                emit("mov", srcReg + ", " + srcOp);
            } else {
                // Memory → memory: intermediate register
                std::string tmp = allocTempReg();
                emit("mov", tmp + ", " + srcOp);
                srcReg = tmp;
            }
        }
        emit("mov", dst + ", " + srcReg);
        break;
    }

    case Instruction::Opcode::Load: {
        auto *li = static_cast<LoadInst*>(&inst);
        std::string ptr = getOperand(li->getPointer());
        std::string r = allocTempReg();
        emit("mov", r + ", " + ptr);
        State.VRegNames[&inst] = r;
        break;
    }

    case Instruction::Opcode::Add: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhsReg = loadToReg(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        emit("add", lhsReg + ", " + rhs);
        State.VRegNames[&inst] = lhsReg;
        break;
    }

    case Instruction::Opcode::Sub: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhsReg = loadToReg(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        emit("sub", lhsReg + ", " + rhs);
        State.VRegNames[&inst] = lhsReg;
        break;
    }

    case Instruction::Opcode::Mul: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhsReg = loadToReg(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        if (lhsReg != "ax") emit("mov", "ax, " + lhsReg);
        if (!isRealReg(rhs)) {
            std::string tmp = allocTempReg();
            emit("mov", tmp + ", " + rhs);
            emit("mul", tmp);
        } else {
            emit("mul", rhs);
        }
        State.VRegNames[&inst] = "ax";
        break;
    }

    case Instruction::Opcode::SDiv: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhsReg = loadToReg(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        if (lhsReg != "ax") emit("mov", "ax, " + lhsReg);
        emit("cwd", "");
        if (!isRealReg(rhs)) {
            std::string tmp = allocTempReg();
            emit("mov", tmp + ", " + rhs);
            emit("idiv", tmp);
        } else {
            emit("idiv", rhs);
        }
        State.VRegNames[&inst] = "ax";
        break;
    }

    case Instruction::Opcode::SRem: {
        auto *bo = static_cast<BinaryOpInst*>(&inst);
        std::string lhsReg = loadToReg(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        if (lhsReg != "ax") emit("mov", "ax, " + lhsReg);
        emit("cwd", "");
        if (!isRealReg(rhs)) {
            std::string tmp = allocTempReg();
            emit("mov", tmp + ", " + rhs);
            emit("idiv", tmp);
        } else {
            emit("idiv", rhs);
        }
        State.VRegNames[&inst] = "dx";
        break;
    }

    case Instruction::Opcode::ICmp: {
        auto *ci = static_cast<ICmpInst*>(&inst);
        std::string lhsReg = loadToReg(ci->getOperand(0));
        std::string rhs = getOperand(ci->getOperand(1));
        emit("cmp", lhsReg + ", " + rhs);
        State.VRegNames[&inst] = lhsReg;
        break;
    }

    case Instruction::Opcode::Br: {
        auto *bi = static_cast<BranchInst*>(&inst);
        if (bi->isConditional()) {
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
                std::string cr = loadToReg(cond);
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
            std::string val = loadToReg(ri->getReturnValue());
            if (val != "ax") emit("mov", "ax, " + val);
        }
        emit("ret", "");
        break;
    }

    case Instruction::Opcode::Call: {
        auto *ci = static_cast<CallInst*>(&inst);
        if (auto *fn = dynamic_cast<Function*>(ci->getOperand(0))) {
            emit("call", fn->getName());
        }
        break;
    }

    default:
        break;
    }
}

// ====== Assembly emission ======
std::string CodeGen::emitAssembly(const MachineModule &mm) {
    std::ostringstream asmOut;

    for (auto &fn : mm.Functions) {
        asmOut << "; Function: " << fn.Name << "\n";
        asmOut << fn.Name << ":\n";

        // Initialize BX as base pointer for variables
        asmOut << "    mov  bx, 8000h\n";

        for (auto &bb : fn.Blocks) {
            bool isEntry = (&bb == &fn.Blocks.front());
            if (!isEntry) {
                asmOut << bb.Label << ":\n";
            }
            for (auto &line : bb.TextLines) {
                asmOut << line << "\n";
            }
        }
        asmOut << "\n";
    }

    asmOut << "hlt\n";
    return asmOut.str();
}

} // namespace ll1
