#include <ll1/IR/IRPrinter.h>
#include <sstream>

namespace ll1 {

std::string IRPrinter::PrintState::getName(const Value *v) {
    if (!v) return "null";

    // Named values use %name
    if (!v->getName().empty()) {
        return "%" + v->getName();
    }

    // Constants print inline (handled in printOperand)
    // Unnamed: assign %N
    auto it = IDMap.find(v);
    if (it != IDMap.end()) {
        return "%" + std::to_string(it->second);
    }
    int id = NextID++;
    IDMap[v] = id;
    return "%" + std::to_string(id);
}

std::string IRPrinter::printType(const Type *t) {
    switch (t->getKind()) {
    case Type::TypeKind::Void:  return "void";
    case Type::TypeKind::Int1:  return "i1";
    case Type::TypeKind::Int8:  return "i8";
    case Type::TypeKind::Int16: return "i16";
    default: return "???";
    }
}

std::string IRPrinter::printOperand(const Value *v, PrintState &s) {
    if (!v) return "void";

    // Constants → immediate
    if (auto *ci = dynamic_cast<const ConstantInt*>(v)) {
        return std::to_string(ci->getValue());
    }

    // BasicBlocks → %label
    if (dynamic_cast<const BasicBlock*>(v)) {
        const std::string &name = v->getName();
        if (!name.empty()) return "%" + name;
        return "%" + std::to_string(s.IDMap[v]);
    }

    // Functions → @name
    if (dynamic_cast<const Function*>(v)) {
        return "@" + v->getName();
    }

    // Regular values → %name or %N
    return s.getName(v);
}

std::string IRPrinter::print(Module &mod) {
    PrintState s;
    return printModule(mod, s);
}

std::string IRPrinter::printModule(Module &mod, PrintState &s) {
    std::ostringstream out;
    out << "; Module: " << mod.getModuleName() << "\n\n";

    for (auto &fn : mod.getFunctionList()) {
        out << printFunction(*fn, s) << "\n";
    }

    return out.str();
}

std::string IRPrinter::printFunction(Function &fn, PrintState &s) {
    std::ostringstream out;

    // Return type
    out << "define " << printType(fn.getReturnType()) << " @" << fn.getName() << "(";

    // Arguments
    for (unsigned i = 0; i < fn.getArgCount(); ++i) {
        if (i > 0) out << ", ";
        auto *arg = fn.getArg(i);
        out << printType(arg->getType()) << " " << s.getName(arg);
    }
    out << ") {\n";

    // BasicBlocks
    for (auto &bb : fn.getBasicBlocks()) {
        out << printBasicBlock(*bb, s);
    }

    out << "}\n";
    return out.str();
}

std::string IRPrinter::printBasicBlock(BasicBlock &bb, PrintState &s) {
    std::ostringstream out;

    // Label
    const std::string &name = bb.getName();
    if (!name.empty()) {
        out << name << ":\n";
    } else {
        out << s.getName(&bb) << ":\n";
    }

    // Instructions
    for (auto &inst : bb.getInstList()) {
        out << "  " << printInstruction(*inst, s) << "\n";
    }

    return out.str();
}

std::string IRPrinter::printInstruction(const Instruction &inst, PrintState &s) {
    std::ostringstream out;

    // Assign a name if this instruction produces a value (not void)
    bool hasResult = inst.getType()->getKind() != Type::TypeKind::Void;
    if (hasResult) {
        out << s.getName(&inst) << " = ";
    }

    switch (inst.getOpcode()) {

    case Instruction::Opcode::Alloca: {
        auto *ai = static_cast<const AllocaInst*>(&inst);
        out << "alloca " << printType(ai->getAllocatedType());
        break;
    }

    case Instruction::Opcode::Store: {
        auto *si = static_cast<const StoreInst*>(&inst);
        out << "store " << printType(si->getValue()->getType()) << " "
            << printOperand(si->getValue(), s) << ", "
            << printType(si->getPointer()->getType()) << " "
            << printOperand(si->getPointer(), s);
        break;
    }

    case Instruction::Opcode::Load: {
        auto *li = static_cast<const LoadInst*>(&inst);
        out << "load " << printType(li->getType()) << ", "
            << printType(li->getPointer()->getType()) << " "
            << printOperand(li->getPointer(), s);
        break;
    }

    case Instruction::Opcode::Add:
    case Instruction::Opcode::Sub:
    case Instruction::Opcode::Mul:
    case Instruction::Opcode::SDiv:
    case Instruction::Opcode::SRem:
    case Instruction::Opcode::And:
    case Instruction::Opcode::Or:
    case Instruction::Opcode::Xor: {
        static const char *opNames[] = {
            "add", "sub", "mul", "sdiv", "srem", "and", "or", "xor"
        };
        auto *bo = static_cast<const BinaryOpInst*>(&inst);
        int idx = static_cast<int>(inst.getOpcode()) - static_cast<int>(Instruction::Opcode::Add);
        out << opNames[idx] << " " << printType(inst.getType()) << " "
            << printOperand(bo->getLHS(), s) << ", "
            << printOperand(bo->getRHS(), s);
        break;
    }

    case Instruction::Opcode::ICmp: {
        auto *ci = static_cast<const ICmpInst*>(&inst);
        static const char *predNames[] = {"eq", "ne", "slt", "sle", "sgt", "sge"};
        out << "icmp " << predNames[static_cast<int>(ci->getPredicate())]
            << " " << printType(ci->getOperand(0)->getType()) << " "
            << printOperand(ci->getOperand(0), s) << ", "
            << printOperand(ci->getOperand(1), s);
        break;
    }

    case Instruction::Opcode::Br: {
        auto *bi = static_cast<const BranchInst*>(&inst);
        if (bi->isConditional()) {
            out << "br " << printType(bi->getCondition()->getType()) << " "
                << printOperand(bi->getCondition(), s) << ", "
                << printOperand(bi->getTrueDest(), s) << ", "
                << printOperand(bi->getFalseDest(), s);
        } else {
            out << "br " << printOperand(bi->getUnconditionalDest(), s);
        }
        break;
    }

    case Instruction::Opcode::Ret: {
        auto *ri = static_cast<const RetInst*>(&inst);
        if (ri->getReturnValue()) {
            out << "ret " << printType(ri->getReturnValue()->getType()) << " "
                << printOperand(ri->getReturnValue(), s);
        } else {
            out << "ret void";
        }
        break;
    }

    case Instruction::Opcode::Call: {
        auto *ci = static_cast<const CallInst*>(&inst);
        out << "call " << printType(inst.getType()) << " "
            << printOperand(ci->getOperand(0), s) << "(";
        for (unsigned i = 1; i < ci->getNumOperands(); ++i) {
            if (i > 1) out << ", ";
            out << printType(ci->getOperand(i)->getType()) << " "
                << printOperand(ci->getOperand(i), s);
        }
        out << ")";
        break;
    }

    case Instruction::Opcode::Phi: {
        auto *pi = static_cast<const PhiInst*>(&inst);
        out << "phi " << printType(inst.getType()) << " ";
        for (unsigned i = 0; i < pi->getNumIncoming(); ++i) {
            if (i > 0) out << ", ";
            out << "[ " << printOperand(pi->getIncomingValue(i), s) << ", "
                << printOperand(pi->getIncomingBlock(i), s) << " ]";
        }
        break;
    }

    case Instruction::Opcode::ArrayLoad: {
        auto *al = static_cast<const ArrayLoadInst*>(&inst);
        out << "arrayload " << printType(inst.getType()) << " "
            << printOperand(al->getBase(), s) << ", "
            << printOperand(al->getIndex(), s);
        break;
    }

    case Instruction::Opcode::ArrayStore: {
        auto *as = static_cast<const ArrayStoreInst*>(&inst);
        out << "arraystore " << printType(as->getValue()->getType()) << " "
            << printOperand(as->getValue(), s) << ", "
            << printOperand(as->getBase(), s) << ", "
            << printOperand(as->getIndex(), s);
        break;
    }

    case Instruction::Opcode::SExt:
    case Instruction::Opcode::ZExt:
    case Instruction::Opcode::Trunc: {
        static const char *castNames[] = {"sext", "zext", "trunc"};
        int idx = static_cast<int>(inst.getOpcode()) - static_cast<int>(Instruction::Opcode::SExt);
        out << castNames[idx] << " "
            << printType(inst.getOperand(0)->getType()) << " "
            << printOperand(inst.getOperand(0), s) << " to "
            << printType(inst.getType());
        break;
    }

    default:
        out << "; unknown opcode";
        break;
    }

    return out.str();
}

} // namespace ll1
