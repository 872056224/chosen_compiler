#include <ll1/Opt/Passes/Dominators.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <queue>
#include <stack>

namespace ll1 {

// ============================================================
// Static member
// ============================================================
AnalysisKey DominatorTreeAnalysis::Key;

// ============================================================
// computeReversePostOrder
// ============================================================
std::vector<BasicBlock*> DominatorTree::computeReversePostOrder(Function &F) {
    std::vector<BasicBlock*> rpo;
    std::set<BasicBlock*> visited;
    BasicBlock *entry = F.getEntryBlock();
    if (!entry) return rpo;

    // DFS-based post-order
    std::stack<BasicBlock*> stack;
    std::set<BasicBlock*> inStack;
    stack.push(entry);

    while (!stack.empty()) {
        BasicBlock *cur = stack.top();
        if (!visited.count(cur)) {
            visited.insert(cur);
            inStack.insert(cur);

            // Push successors in reverse order (for deterministic RPO)
            auto *term = cur->getTerminator();
            if (term) {
                if (auto *br = dynamic_cast<BranchInst*>(term)) {
                    if (br->isConditional()) {
                        // Push false first so true is visited first
                        auto *falseBB = br->getFalseDest();
                        auto *trueBB = br->getTrueDest();
                        if (falseBB && !visited.count(falseBB))
                            stack.push(falseBB);
                        if (trueBB && !visited.count(trueBB))
                            stack.push(trueBB);
                    } else if (br->isUnconditional()) {
                        auto *dest = br->getUnconditionalDest();
                        if (dest && !visited.count(dest))
                            stack.push(dest);
                    }
                }
            }
        } else {
            if (inStack.count(cur)) {
                rpo.push_back(cur);  // Post-order: add when all children visited
                inStack.erase(cur);
            }
            stack.pop();
        }
    }

    // Reverse to get RPO
    std::reverse(rpo.begin(), rpo.end());
    return rpo;
}

// ============================================================
// intersect — find LCA in the dom tree under construction
// ============================================================
BasicBlock* DominatorTree::intersect(
    BasicBlock *A, BasicBlock *B,
    std::unordered_map<BasicBlock*, int> &order,
    std::unordered_map<BasicBlock*, BasicBlock*> &idom) {

    // Walk up from both nodes until they meet
    // The "finger" algorithm
    BasicBlock *fingerA = A;
    BasicBlock *fingerB = B;

    while (fingerA != fingerB) {
        while (order[fingerA] > order[fingerB]) {
            fingerA = idom[fingerA];
        }
        while (order[fingerB] > order[fingerA]) {
            fingerB = idom[fingerB];
        }
    }
    return fingerA;
}

// ============================================================
// computeDomFrontier
// ============================================================
void DominatorTree::computeDomFrontier(Function &F, Result &res) {
    // Initialize empty frontier sets
    for (auto &bb : F.getBasicBlocks()) {
        res.DomFrontier[bb.get()] = {};
    }

    // For each BB with multiple predecessors:
    for (auto &bb : F.getBasicBlocks()) {
        BasicBlock *B = bb.get();
        auto preds = B->getPredecessors();
        if (preds.size() < 2) continue;

        for (auto *P : preds) {
            BasicBlock *runner = P;
            // Walk up from P to IDom[B], adding B to frontier of each node
            while (runner && runner != res.IDom[B]) {
                res.DomFrontier[runner].insert(B);
                runner = res.IDom[runner];
            }
        }
    }
}

// ============================================================
// buildTree — construct tree from IDom
// ============================================================
void DominatorTree::buildTree(Function &F, Result &res) {
    BasicBlock *entry = F.getEntryBlock();
    if (!entry) return;

    // Build children map
    for (auto &bb : F.getBasicBlocks()) {
        res.Children[bb.get()] = {};
    }

    for (auto &bb : F.getBasicBlocks()) {
        auto *idom = res.IDom[bb.get()];
        if (idom && idom != bb.get()) {
            res.Children[idom].push_back(bb.get());
        }
    }

    // Level-order traversal
    std::queue<BasicBlock*> q;
    q.push(entry);
    while (!q.empty()) {
        auto *cur = q.front(); q.pop();
        res.LevelOrder.push_back(cur);

        for (auto *child : res.Children[cur]) {
            q.push(child);
        }
    }

    // Compute leaves
    for (auto &bb : F.getBasicBlocks()) {
        if (res.Children[bb.get()].empty()) {
            res.Leaves.insert(bb.get());
        }
    }
}

// ============================================================
// run — Cooper-Harvey-Kennedy iterative algorithm
// ============================================================
DominatorTree::Result DominatorTree::run(Function &F, FunctionAnalysisManager &/*FAM*/) {
    Result res;
    auto &blocks = F.getBasicBlocks();
    BasicBlock *entry = F.getEntryBlock();
    if (!entry || blocks.empty()) return res;

    // Step 1: Reverse post-order
    auto rpo = computeReversePostOrder(F);

    // Step 2: Build RPO index map
    std::unordered_map<BasicBlock*, int> order;
    for (size_t i = 0; i < rpo.size(); ++i) {
        order[rpo[i]] = static_cast<int>(i);
    }

    // Step 3: Initialize IDom
    for (auto &bb : blocks) {
        res.IDom[bb.get()] = nullptr;
    }
    res.IDom[entry] = entry;

    // Step 4: Iterative dataflow
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *bb : rpo) {
            if (bb == entry) continue;

            // Collect IDoms of predecessors
            BasicBlock *newIdom = nullptr;
            auto preds = bb->getPredecessors();
            for (auto *pred : preds) {
                if (!res.IDom[pred]) continue;  // Not yet computed
                if (!newIdom) {
                    newIdom = pred;
                } else {
                    newIdom = intersect(newIdom, pred, order, res.IDom);
                }
            }

            if (newIdom && newIdom != res.IDom[bb]) {
                res.IDom[bb] = newIdom;
                changed = true;
            }
        }
    }

    // Step 5: Compute dominance frontier
    computeDomFrontier(F, res);

    // Step 6: Build tree structures
    buildTree(F, res);

    std::cerr << "[opt] DominatorTree computed for " << F.getName()
              << " (" << blocks.size() << " blocks)" << std::endl;

    return res;
}

// ============================================================
// Query methods
// ============================================================
bool DominatorTree::Result::dominates(BasicBlock *A, BasicBlock *B) const {
    if (A == B) return true;
    // Walk up the IDom chain from B
    BasicBlock *cur = B;
    auto it = IDom.find(cur);
    while (it != IDom.end() && it->second) {
        if (it->second == A) return true;
        if (it->second == cur) break;  // Reached entry
        cur = it->second;
        it = IDom.find(cur);
    }
    return false;
}

bool DominatorTree::Result::strictlyDominates(BasicBlock *A, BasicBlock *B) const {
    return A != B && dominates(A, B);
}

bool DominatorTree::Result::properlyDominates(BasicBlock *A, BasicBlock *B) const {
    return strictlyDominates(A, B);
}

BasicBlock* DominatorTree::Result::findNearestCommonDominator(BasicBlock *A, BasicBlock *B) const {
    // Walk up from both to find common dominator
    std::set<BasicBlock*> seen;
    BasicBlock *cur = A;
    while (cur) {
        seen.insert(cur);
        auto it = IDom.find(cur);
        if (it == IDom.end() || it->second == cur) break;
        cur = it->second;
    }
    cur = B;
    while (cur) {
        if (seen.count(cur)) return cur;
        auto it = IDom.find(cur);
        if (it == IDom.end() || it->second == cur) break;
        cur = it->second;
    }
    return nullptr;
}

} // namespace ll1
