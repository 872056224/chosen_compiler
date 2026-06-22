#include <ll1/CodeGen/SelectionDAG/SDNode.h>
#include <ll1/CodeGen/SelectionDAG/SelectionDAG.h>
#include <ll1/CodeGen/MachineFunction.h>
#include <ll1/IR/Instruction.h>
#include <ll1/IR/Type.h>
#include <cassert>
#include <iostream>

using namespace ll1;

int main() {
    // Create a MachineFunction for the DAG
    MachineFunction mf("test");

    SelectionDAG dag(mf);

    // Test 1: EntryToken
    auto token = dag.getEntryToken();
    assert(token.isValid());
    assert(token.getOpcode() == ISD::EntryToken);
    assert(token.getNode()->getNumOperands() == 0);

    // Test 2: Constant node
    auto c = dag.getConstant(42, Type::getInt16Ty());
    assert(c.isValid());
    assert(c.getOpcode() == ISD::Constant);

    // Test 3: FrameIndex
    auto fi = dag.getFrameIndex(2, Type::getInt16Ty());
    assert(fi.isValid());
    assert(fi.getOpcode() == ISD::FrameIndex);
    assert(fi.getNode()->getFrameIndex() == 2);

    // Test 4: Binary node (ADD)
    auto c10 = dag.getConstant(10, Type::getInt16Ty());
    auto c20 = dag.getConstant(20, Type::getInt16Ty());
    auto add = dag.getBinary(ISD::ADD, Type::getInt16Ty(), c10, c20);
    assert(add.isValid());
    assert(add.getOpcode() == ISD::ADD);
    assert(add.getNode()->getNumOperands() == 2);

    // Test 5: LOAD
    auto load = dag.getLoad(Type::getInt16Ty(), token, fi);
    assert(load.isValid());
    assert(load.getOpcode() == ISD::LOAD);

    // Test 6: STORE
    auto store = dag.getStore(token, c10, fi);
    assert(store.isValid());
    assert(store.getOpcode() == ISD::STORE);

    // Test 7: SDValue comparison
    SDValue a = dag.getConstant(1, Type::getInt16Ty());
    SDValue b = dag.getConstant(1, Type::getInt16Ty());
    SDValue c2 = dag.getConstant(2, Type::getInt16Ty());
    assert(a == a);
    assert(!(a == b));  // Different nodes
    assert(a != c2);

    // Test 8: Node count
    size_t nodeCount = dag.getAllNodes().size();
    assert(nodeCount >= 7);  // At least our created nodes

    // Test 9: SDNode payload — FrameIndex
    assert(fi.getNode()->getFrameIndex() == 2);
    fi.getNode()->setFrameIndex(5);
    assert(fi.getNode()->getFrameIndex() == 5);

    // Test 10: SDNode payload — Constant
    c.getNode()->setConstant(99);
    assert(c.getNode()->getConstant() == 99);

    // Test 11: SETCC with predicate
    auto setcc = dag.getSetCC(ICmpInst::SLT, c10, c20);
    assert(setcc.isValid());
    assert(setcc.getOpcode() == ISD::SETCC);
    // Predicate stored in payload
    assert(setcc.getNode()->Payload.CmpPred == static_cast<uint8_t>(ICmpInst::SLT));

    // Test 12: RET
    auto ret = dag.getRet(token, c10);
    assert(ret.isValid());
    assert(ret.getOpcode() == ISD::RET);

    // Test 13: Sign extend
    auto sext = dag.getSExt(c10, Type::getInt16Ty());
    assert(sext.isValid());
    assert(sext.getOpcode() == ISD::SIGN_EXTEND);

    std::cout << "[PASS] test_selection_dag (" << nodeCount << " nodes)" << std::endl;
    return 0;
}
