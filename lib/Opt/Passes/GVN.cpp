#include <ll1/Opt/Passes/GVN.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>

namespace ll1 {

unsigned GVN::getOrCreateVN(Value *v) {
    auto it = valueNumbers.find(v);
    if (it != valueNumbers.end()) return it->second;

    // Assign new value number
    unsigned vn = nextVN++;
    valueNumbers[v] = vn;

    // For constants, hash by value
    if (auto *c = dynamic_cast<ConstantInt*>(v)) {
        // Already assigned above
    }

    return vn;
}

std::string GVN::computeHash(Instruction *inst) {
    std::ostringstream oss;
    auto op = inst->getOpcode();

    // Opcode prefix
    oss << static_cast<int>(op) << ":";

    // Hash operands by their value numbers
    for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
        Value *opnd = inst->getOperand(i);
        if (!opnd) {
            oss << "null,";
        } else {
            unsigned vn = getOrCreateVN(opnd);
            oss << vn << ",";
        }
    }

    return oss.str();
}

// Check if an instruction is "computable" (eligiblie for GVN replacement)
static bool isComputable(Instruction *inst) {
    switch (inst->getOpcode()) {
    case Instruction::Opcode::Add:
    case Instruction::Opcode::Sub:
    case Instruction::Opcode::Mul:
    case Instruction::Opcode::SDiv:
    case Instruction::Opcode::SRem:
    case Instruction::Opcode::And:
    case Instruction::Opcode::Or:
    case Instruction::Opcode::Xor:
    case Instruction::Opcode::ICmp:
    case Instruction::Opcode::SExt:
    case Instruction::Opcode::ZExt:
    case Instruction::Opcode::Trunc:
    case Instruction::Opcode::Load:
        return true;
    default:
        return false;
    }
}

// Recursive rename along the dominator tree
static void gvnRename(
    BasicBlock *BB,
    DominatorTree::Result &DT,
    GVN &gvn,
    std::unordered_map<std::string, Instruction*> &hashToInst,
    std::vector<Instruction*> &toRemove) {

    // Track keys added in this BB for scoped removal
    std::vector<std::string> addedKeys;

    for (auto it = BB->getInstList().begin(); it != BB->getInstList().end(); ) {
        Instruction *inst = it->get();

        if (isComputable(inst)) {
            std::string hash = gvn.computeHash(inst);

            auto hit = hashToInst.find(hash);
            if (hit != hashToInst.end()) {
                Instruction *existing = hit->second;
                // Only replace if existing dominates current
                if (existing && existing->getParent() &&
                    DT.dominates(existing->getParent(), BB)) {
                    inst->replaceAllUsesWith(existing);
                    toRemove.push_back(inst);
                    ++it;
                    continue;
                } else {
                    // Update with potentially more dominating instruction
                    hit->second = inst;
                }
            } else {
                hashToInst[hash] = inst;
                addedKeys.push_back(hash);
            }
        }

        ++it;
    }

    // Process children in dominator tree
    auto childIt = DT.Children.find(BB);
    if (childIt != DT.Children.end()) {
        for (auto *child : childIt->second) {
            gvnRename(child, DT, gvn, hashToInst, toRemove);
        }
    }

    // Scoped removal: remove entries added in this BB
    for (auto &key : addedKeys) {
        // Only remove if it still points to an instruction from this BB
        auto it = hashToInst.find(key);
        if (it != hashToInst.end() && it->second->getParent() == BB) {
            hashToInst.erase(it);
        }
    }
}

PreservedAnalyses GVN::run(Function &F, FunctionAnalysisManager &FAM) {
    auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);

    // Reset state
    valueNumbers.clear();
    hashToInst.clear();
    nextVN = 0;

    // Assign value numbers to arguments first
    for (auto &arg : F.getArgs()) {
        getOrCreateVN(arg.get());
    }

    std::vector<Instruction*> toRemove;

    // Walk dominator tree recursively
    BasicBlock *entry = F.getEntryBlock();
    if (entry) {
        gvnRename(entry, DT, *this, hashToInst, toRemove);
    }

    // Remove replaced instructions
    unsigned removed = 0;
    for (auto *inst : toRemove) {
        auto *bb = inst->getParent();
        if (!bb) continue;
        auto &list = bb->getInstList();
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->get() == inst) {
                list.erase(it);
                removed++;
                break;
            }
        }
    }

    std::cerr << "[opt] GVN: replaced " << removed
              << " redundant instructions in " << F.getName() << std::endl;

    if (removed > 0) {
        PreservedAnalyses PA;
        PA.preserveSet<CFGAnalyses>();
        return PA;
    }
    return PreservedAnalyses::all();
}

} // namespace ll1
