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

// assignReg: allocate a physical register for a Value and remember it
std::string CodeGen::assignReg(Value *v) {
    auto it = State.VRegNames.find(v);
    if (it != State.VRegNames.end()) return it->second;
    std::string r = allocTempReg();
    State.VRegNames[v] = r;
    return r;
}

// getReg: return the physical register already assigned, or empty if none
std::string CodeGen::getReg(Value *v) {
    auto it = State.VRegNames.find(v);
    if (it != State.VRegNames.end()) return it->second;
    return "";
}

// resolve operand to assembly text.
// If it's already a real register, return it.
// If it's memory ref or immediate, return as-is.
// If it's a virtual reg → look up assigned physical reg.
std::string CodeGen::getOperand(Value *v) {
    if (!v) return "0";

    // Constant → immediate
    if (dynamic_cast<ConstantInt*>(v)) {
        return std::to_string(static_cast<ConstantInt*>(v)->getValue());
    }

    // Alloca → stack slot [bp+offset]
    if (auto *ai = dynamic_cast<AllocaInst*>(v)) {
        if (State.AllocaOffsets.count(v)) {
            return memOpBP(State.AllocaOffsets[v]);
        }
        int size = 2;
        if (ai->getAllocatedType() == Type::getInt8Ty()) size = 1;
        State.StackOffset -= size;
        State.AllocaOffsets[v] = State.StackOffset;
        State.MF.StackSize = std::max(State.MF.StackSize, -State.StackOffset);
        return memOpBP(State.StackOffset);
    }

    // BasicBlock → label
    if (auto *bb = dynamic_cast<BasicBlock*>(v)) {
        return State.BlockLabels[bb];
    }

    // Instruction result → lookup assigned physical register
    std::string r = getReg(v);
    if (!r.empty()) return r;

    // Unmapped: assign one now
    return assignReg(v);
}

// load value into a register → returns the register name
std::string CodeGen::loadToReg(Value *v) {
    // Already a constant → need a register
    if (auto *ci = dynamic_cast<ConstantInt*>(v)) {
        std::string r = allocTempReg();
        emit("mov", r + ", " + std::to_string(ci->getValue()));
        return r;
    }

    // Memory operand → load into register
    std::string op = getOperand(v);
    if (op.find('[') != std::string::npos) {
        std::string r = allocTempReg();
        emit("mov", r + ", " + op);
        return r;
    }

    // May be a virtual reg reference
    if (op == "ax" || op == "bx" || op == "cx" || op == "dx" ||
        op == "si" || op == "di" || op == "bp" || op == "sp") {
        return op;
    }

    // It's something else (already a register or immediate) — move to temp
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
    // No inline comments in output — cleaner for simple emulators
    (void)comment;
    Out << line << "\n";

    // Store raw text for emitAssembly
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
        std::string label = "." + fn.getName() + "_" + bb->getName();
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
        getOperand(&inst); // trigger stack slot allocation
        assignReg(&inst);  // remember this alloca for later use
        break;
    }

    case Instruction::Opcode::Store: {
        auto *si = static_cast<StoreInst*>(&inst);
        Value *srcVal = si->getValue();
        std::string dst = getOperand(si->getPointer()); // should be [bp+offset]

        // Load source into a register, then store to memory
        std::string srcReg;
        if (auto *ci = dynamic_cast<ConstantInt*>(srcVal)) {
            srcReg = allocTempReg();
            emit("mov", srcReg + ", " + std::to_string(ci->getValue()));
        } else {
            std::string srcOp = getOperand(srcVal);
            if (srcOp.find('[') != std::string::npos) {
                // mem → mem: need intermediate register
                std::string tmp = allocTempReg();
                emit("mov", tmp + ", " + srcOp);
                srcReg = tmp;
            } else {
                srcReg = srcOp;
            }
        }
        if (srcReg != dst) {
            emit("mov", dst + ", " + srcReg);
        }
        break;
    }

    case Instruction::Opcode::Load: {
        auto *li = static_cast<LoadInst*>(&inst);
        std::string ptr = getOperand(li->getPointer());
        // Load from memory into a real register
        std::string r = allocTempReg();
        emit("mov", r + ", " + ptr, "load " + li->getName());
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
        // Multiply: ax = ax * src
        std::string lhsReg = loadToReg(bo->getLHS());
        std::string rhs = getOperand(bo->getRHS());
        // Move lhs to ax
        if (lhsReg != "ax") emit("mov", "ax, " + lhsReg);
        // Move rhs to temp if it's immediate or mem
        if (rhs.find('[') != std::string::npos || isdigit(rhs[0]) || rhs[0] == '-') {
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
        emit("cwd", ""); // sign-extend ax → dx:ax
        if (rhs.find('[') != std::string::npos || isdigit(rhs[0]) || rhs[0] == '-') {
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
        if (rhs.find('[') != std::string::npos || isdigit(rhs[0]) || rhs[0] == '-') {
            std::string tmp = allocTempReg();
            emit("mov", tmp + ", " + rhs);
            emit("idiv", tmp);
        } else {
            emit("idiv", rhs);
        }
        State.VRegNames[&inst] = "dx"; // remainder in dx
        break;
    }

    case Instruction::Opcode::ICmp: {
        auto *ci = static_cast<ICmpInst*>(&inst);
        std::string lhsReg = loadToReg(ci->getOperand(0));
        std::string rhs = getOperand(ci->getOperand(1));
        emit("cmp", lhsReg + ", " + rhs);
        State.VRegNames[&inst] = lhsReg; // flags set, handled by branch
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
        // Epilogue before return
        if (State.MF.StackSize > 0) {
            emit("mov", "sp, bp");
            emit("pop", "bp");
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

    case Instruction::Opcode::And:
    case Instruction::Opcode::Or:
    case Instruction::Opcode::Xor:
        break;

    default:
        break;
    }
}

// ====== Assembly emission ======
std::string CodeGen::emitAssembly(const MachineModule &mm) {
    std::ostringstream asmOut;

    // Simple format: labels + instructions, compatible with online emulators.
    // No .code / proc / endp / end — those are MASM/TASM directives.
    for (auto &fn : mm.Functions) {
        asmOut << "; Function: " << fn.Name << "\n";

        // Entry label
        asmOut << fn.Name << ":\n";

        // Prologue
        if (fn.StackSize > 0) {
            asmOut << "    push bp\n";
            asmOut << "    mov  bp, sp\n";
            asmOut << "    sub  sp, " << fn.StackSize << "\n";
        }

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
