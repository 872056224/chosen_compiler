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

int CodeGen::getArrayBaseOffset(Value *base) {
    auto it = State.VarOffsets.find(base);
    if (it != State.VarOffsets.end()) return it->second;
    // If not yet allocated (shouldn't happen), return 0
    return 0;
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

    // Alloca → [bx+offset] memory operand
    if (dynamic_cast<AllocaInst*>(v)) {
        return varLabel(v);
    }

    // Argument → [bp+offset] (parameters on stack)
    if (auto *arg = dynamic_cast<Argument*>(v)) {
        auto it = State.ArgOffsets.find(arg);
        if (it != State.ArgOffsets.end()) {
            return memOpBP(it->second);
        }
        return memOpBP(4); // fallback
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
        State.MF.LocalSize = State.NextVarOffset;  // Total local bytes
        mm.Functions.push_back(State.MF);
    }

    return mm;
}

void CodeGen::generateFunction(Function &fn) {
    // Track argument BP offsets: [bp+4] = arg0, [bp+6] = arg1, ...
    for (unsigned i = 0; i < fn.getArgCount(); ++i) {
        State.ArgOffsets[fn.getArg(i)] = 4 + i * 2;
    }

    int labelId = 0;
    for (auto &bb : fn.getBasicBlocks()) {
        std::string label = fn.getName() + "_" + bb->getName() + "_" + std::to_string(labelId++);
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
        auto *ai = dynamic_cast<AllocaInst*>(&inst);
        int elemSize = (ai && ai->getAllocatedType() == Type::getInt8Ty()) ? 1 : 2;
        int slotSize = (ai && ai->isArrayAlloca()) ? (ai->getArraySize() * elemSize) : elemSize;
        int off = State.NextVarOffset;
        State.NextVarOffset += slotSize;
        State.VarOffsets[&inst] = off;
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
        bool isByte = (srcVal && srcVal->getType() == Type::getInt8Ty());
        // Use byte register for i8 stores
        if (isByte) {
            if (srcReg == "ax") srcReg = "al";
            else if (srcReg == "cx") srcReg = "cl";
            else if (srcReg == "dx") srcReg = "dl";
        }
        emit("mov", dst + ", " + srcReg);
        break;
    }

    case Instruction::Opcode::Load: {
        auto *li = static_cast<LoadInst*>(&inst);
        std::string ptr = getOperand(li->getPointer());
        bool isByte = (li->getType() == Type::getInt8Ty());
        std::string r = allocTempReg();
        if (isByte && r == "ax") {
            emit("mov", "al, " + ptr);
            emit("mov", "ah, 0");
        } else if (isByte && r == "cx") {
            emit("mov", "cl, " + ptr);
            emit("mov", "ch, 0");
        } else if (isByte && r == "dx") {
            emit("mov", "dl, " + ptr);
            emit("mov", "dh, 0");
        } else {
            emit("mov", r + ", " + ptr);
        }
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
        // Epilogue
        emit("mov", "sp, bp");
        emit("pop", "bp");
        emit("ret", "");
        break;
    }

    case Instruction::Opcode::Call: {
        auto *ci = static_cast<CallInst*>(&inst);
        if (auto *fn = dynamic_cast<Function*>(ci->getOperand(0))) {
            // Built-in: __print(value)
            if (fn->getName() == "__print") {
                if (ci->getNumOperands() > 1) {
                    std::string arg = loadToReg(ci->getOperand(1));
                    if (arg != "ax") emit("mov", "ax, " + arg);
                }
                emit("call", fn->getName());
                break;
            }
            // Built-in: __read()
            if (fn->getName() == "__read") {
                emit("call", fn->getName());
                State.VRegNames[&inst] = "ax";
                break;
            }
            // User function: push args right-to-left, call, caller cleanup
            int argCount = ci->getNumOperands() - 1;
            for (int i = argCount; i >= 1; --i) {
                std::string argReg = loadToReg(ci->getOperand(i));
                emit("push", argReg);
            }
            emit("call", fn->getName());
            if (argCount > 0) {
                emit("add", "sp, " + std::to_string(argCount * 2));
            }
            if (inst.getType() != Type::getVoidTy()) {
                State.VRegNames[&inst] = "ax";
            }
        }
        break;
    }

    case Instruction::Opcode::ArrayLoad: {
        auto *al = static_cast<ArrayLoadInst*>(&inst);
        int baseOff = getArrayBaseOffset(al->getBase());
        // Index: use SI directly (avoids temp reg conflicts)
        std::string idxSrc = getOperand(al->getIndex());
        emit("mov", "si, " + idxSrc);
        emit("shl", "si, 1");
        if (baseOff > 0) emit("add", "si, " + std::to_string(baseOff));
        std::string r = allocTempReg();
        emit("mov", r + ", [bx+si]");
        State.VRegNames[&inst] = r;
        break;
    }

    case Instruction::Opcode::ArrayStore: {
        auto *as = static_cast<ArrayStoreInst*>(&inst);
        int baseOff = getArrayBaseOffset(as->getBase());
        // Load value first, THEN compute index in SI
        std::string valReg = loadToReg(as->getValue());
        std::string idxSrc = getOperand(as->getIndex());
        emit("mov", "si, " + idxSrc);
        emit("shl", "si, 1");
        if (baseOff > 0) emit("add", "si, " + std::to_string(baseOff));
        emit("mov", "[bx+si], " + valReg);
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
        // Skip built-in runtime functions — emitted separately at bottom
        if (fn.Name == "__print" || fn.Name == "__read") continue;

        asmOut << "; Function: " << fn.Name << "\n";
        asmOut << fn.Name << ":\n";

        // Standard stack frame prologue
        asmOut << "    push bp\n";
        asmOut << "    mov  bp, sp\n";
        if (fn.LocalSize > 0) {
            asmOut << "    sub  sp, " << fn.LocalSize << "\n";
        }
        // Set BX to base of local area (for [bx+offset] addressing)
        if (fn.LocalSize > 0) {
            asmOut << "    mov  bx, bp\n";
            asmOut << "    sub  bx, " << fn.LocalSize << "\n";
        } else {
            asmOut << "    mov  bx, bp\n";
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

    // Runtime helpers (embedded in every output)
    bool needRuntime = false;

    // Check if any function references __print or __read
    for (auto &fn : mm.Functions) {
        for (auto &bb : fn.Blocks) {
            for (auto &line : bb.TextLines) {
                if (line.find("__print") != std::string::npos ||
                    line.find("__read") != std::string::npos) {
                    needRuntime = true;
                    break;
                }
            }
        }
    }

    if (needRuntime) {
        asmOut << "\n; --- Runtime: __print (AX = value to print) ---\n";
        asmOut << "__print:\n";
        asmOut << "    push bp\n";
        asmOut << "    push bx\n";
        asmOut << "    push cx\n";
        asmOut << "    push dx\n";
        asmOut << "    mov  cx, 0\n";
        asmOut << "    mov  bx, 10\n";
        asmOut << "__print_loop:\n";
        asmOut << "    mov  dx, 0\n";
        asmOut << "    div  bx\n";
        asmOut << "    push dx\n";
        asmOut << "    inc  cx\n";
        asmOut << "    cmp  ax, 0\n";
        asmOut << "    jne  __print_loop\n";
        asmOut << "__print_disp:\n";
        asmOut << "    pop  dx\n";
        asmOut << "    add  dl, '0'\n";
        asmOut << "    mov  ah, 02h\n";
        asmOut << "    int  21h\n";
        asmOut << "    loop __print_disp\n";
        asmOut << "    mov  dl, ' '\n";
        asmOut << "    mov  ah, 02h\n";
        asmOut << "    int  21h\n";
        asmOut << "    pop  dx\n";
        asmOut << "    pop  cx\n";
        asmOut << "    pop  bx\n";
        asmOut << "    pop  bp\n";
        asmOut << "    ret\n";

        asmOut << "\n; --- Runtime: __read (returns in AX) ---\n";
        asmOut << "__read:\n";
        asmOut << "    push bp\n";
        asmOut << "    push bx\n";
        asmOut << "    push cx\n";
        asmOut << "    push dx\n";
        asmOut << "    mov  ax, 0\n";
        asmOut << "    mov  bx, 0\n";
        asmOut << "__read_loop:\n";
        asmOut << "    mov  ah, 01h\n";
        asmOut << "    int  21h\n";
        asmOut << "    cmp  al, 0Dh\n";
        asmOut << "    je   __read_done\n";
        asmOut << "    sub  al, '0'\n";
        asmOut << "    mov  cl, al\n";
        asmOut << "    mov  ch, 0\n";
        asmOut << "    mov  ax, bx\n";
        asmOut << "    mov  dx, 10\n";
        asmOut << "    mul  dx\n";
        asmOut << "    add  ax, cx\n";
        asmOut << "    mov  bx, ax\n";
        asmOut << "    jmp  __read_loop\n";
        asmOut << "__read_done:\n";
        asmOut << "    mov  ax, bx\n";
        asmOut << "    pop  dx\n";
        asmOut << "    pop  cx\n";
        asmOut << "    pop  bx\n";
        asmOut << "    pop  bp\n";
        asmOut << "    ret\n";
    }

    asmOut << "hlt\n";
    return asmOut.str();
}

} // namespace ll1
