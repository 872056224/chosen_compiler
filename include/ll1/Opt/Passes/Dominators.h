#ifndef LL1_OPT_PASSES_DOMINATORS_H
#define LL1_OPT_PASSES_DOMINATORS_H

#include <ll1/Opt/PassManager.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>
#include <vector>
#include <unordered_map>
#include <set>
#include <string>

namespace ll1 {

class DominatorTree {
public:
    struct Result {
        // IDom[A] = the immediate dominator of A (nullptr for entry)
        std::unordered_map<BasicBlock*, BasicBlock*> IDom;

        // DomFrontier[A] = set of BBs whose dominance frontier includes A
        std::unordered_map<BasicBlock*, std::set<BasicBlock*>> DomFrontier;

        // Children[A] = children of A in the dominator tree
        std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> Children;

        // Level order traversal from root
        std::vector<BasicBlock*> LevelOrder;

        // Dominator tree leaf nodes
        std::set<BasicBlock*> Leaves;

        // Query methods
        bool dominates(BasicBlock *A, BasicBlock *B) const;
        bool strictlyDominates(BasicBlock *A, BasicBlock *B) const;
        BasicBlock* findNearestCommonDominator(BasicBlock *A, BasicBlock *B) const;

        // Check if A properly dominates B
        bool properlyDominates(BasicBlock *A, BasicBlock *B) const;
    };

    Result run(Function &F, FunctionAnalysisManager &FAM);

private:
    // Compute reverse post-order traversal
    std::vector<BasicBlock*> computeReversePostOrder(Function &F);

    // Intersect: find LCA in the dominator tree under construction
    BasicBlock* intersect(BasicBlock *A, BasicBlock *B,
                          std::unordered_map<BasicBlock*, int> &order,
                          std::unordered_map<BasicBlock*, BasicBlock*> &idom);

    // Compute dominance frontiers
    void computeDomFrontier(Function &F, Result &res);

    // Build tree structure from IDom
    void buildTree(Function &F, Result &res);
};

// Analysis pass wrapper
class DominatorTreeAnalysis {
public:
    static AnalysisKey* ID() { return &Key; }
    using Result = DominatorTree::Result;

    Result run(Function &F, FunctionAnalysisManager &FAM) {
        return DominatorTree().run(F, FAM);
    }

private:
    static AnalysisKey Key;
};

} // namespace ll1

#endif
