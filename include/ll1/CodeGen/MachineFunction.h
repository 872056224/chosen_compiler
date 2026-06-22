#ifndef LL1_CODEGEN_MACHINEFUNCTION_H
#define LL1_CODEGEN_MACHINEFUNCTION_H

#include <ll1/CodeGen/MachineInstr.h>
#include <ll1/CodeGen/MachineRegisterInfo.h>
#include <ll1/CodeGen/MachineFrameInfo.h>
#include <ll1/IR/BasicBlock.h>
#include <vector>
#include <memory>
#include <string>
#include <set>

namespace ll1 {

class MachineBasicBlock;

// ============================================================
// MachineBasicBlock
// ============================================================
class MachineBasicBlock {
public:
    explicit MachineBasicBlock(const std::string &name = "") : Label(name) {}

    const std::string& getLabel() const { return Label; }
    void setLabel(const std::string &name) { Label = name; }

    // Instruction list
    std::vector<MachineInstr>& getInstList() { return Instructions; }
    const std::vector<MachineInstr>& getInstList() const { return Instructions; }
    bool empty() const { return Instructions.empty(); }
    size_t size() const { return Instructions.size(); }

    void push_back(MachineInstr MI);
    void push_front(MachineInstr MI);
    MachineInstr& back() { return Instructions.back(); }

    // Insert before position
    void insertBeforeTerminator(MachineInstr MI);

    // Terminator access
    MachineInstr* getTerminator();
    const MachineInstr* getTerminator() const;

    // Successor/Predecessor tracking
    std::vector<MachineBasicBlock*>& getSuccessors() { return Successors; }
    std::vector<MachineBasicBlock*>& getPredecessors() { return Predecessors; }
    void addSuccessor(MachineBasicBlock *Succ) { Successors.push_back(Succ); }
    void addPredecessor(MachineBasicBlock *Pred) { Predecessors.push_back(Pred); }

    // Parent function
    class MachineFunction* getParent() const { return Parent; }
    void setParent(MachineFunction *F) { Parent = F; }

    // IR BasicBlock that this MBB corresponds to
    BasicBlock* getIRBasicBlock() const { return IRBB; }
    void setIRBasicBlock(BasicBlock *bb) { IRBB = bb; }

    // Phi COPY records (filled by SDBuilder, lowered by ISel)
    struct PhiCopy {
        Register DestReg;
        Register SrcReg;
        MachineBasicBlock *PredBB;
    };
    std::vector<PhiCopy>& getPhiCopies() { return PhiCopies; }

    // Block number for printing
    unsigned getNumber() const { return BlockNumber; }
    void setNumber(unsigned n) { BlockNumber = n; }

private:
    std::string Label;
    std::vector<MachineInstr> Instructions;
    std::vector<MachineBasicBlock*> Successors;
    std::vector<MachineBasicBlock*> Predecessors;
    MachineFunction *Parent = nullptr;
    BasicBlock *IRBB = nullptr;
    std::vector<PhiCopy> PhiCopies;
    unsigned BlockNumber = 0;
};

// ============================================================
// MachineFunction
// ============================================================
class MachineFunction {
public:
    explicit MachineFunction(const std::string &name = "") : Name(name) {}

    const std::string& getName() const { return Name; }

    // Basic blocks
    std::vector<std::unique_ptr<MachineBasicBlock>>& getBasicBlocks() { return Blocks; }
    MachineBasicBlock* createMachineBasicBlock(const std::string &name = "");

    // Register and frame info
    MachineRegisterInfo& getRegInfo() { return RegInfo; }
    const MachineRegisterInfo& getRegInfo() const { return RegInfo; }

    MachineFrameInfo& getFrameInfo() { return FrameInfo; }
    const MachineFrameInfo& getFrameInfo() const { return FrameInfo; }

    unsigned getLocalSize() const { return FrameInfo.getStackSize(); }

    // Compute CFG (successors/predecessors from branch instructions)
    void computeCFG();

    // Instruction position counter (for liveness pre-scan)
    unsigned& getNextInstrPos() { return NextInstrPos; }

private:
    std::string Name;
    std::vector<std::unique_ptr<MachineBasicBlock>> Blocks;
    MachineRegisterInfo RegInfo;
    MachineFrameInfo FrameInfo;
    unsigned NextInstrPos = 0;
};

} // namespace ll1
#endif
