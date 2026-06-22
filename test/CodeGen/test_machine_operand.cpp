#include <ll1/CodeGen/MachineOperand.h>
#include <cassert>
#include <iostream>

using namespace ll1;

int main() {
    // Test 1: Create Register operand
    auto regOp = MachineOperand::CreateReg(X86::AX, true, false, false);
    assert(regOp.getType() == MachineOperandType::MO_Register);
    assert(regOp.getReg() == X86::AX);
    assert(regOp.isDef());
    assert(!regOp.isDead());
    assert(!regOp.isKill());

    // Test 2: Register operand flags
    auto defOp = MachineOperand::CreateReg(X86::CX, true, false, false);
    assert(defOp.isDef());
    auto useOp = MachineOperand::CreateReg(X86::DX, false, false, true);
    assert(!useOp.isDef());
    assert(useOp.isKill());

    // Test 3: Create Immediate operand
    auto immOp = MachineOperand::CreateImm(42);
    assert(immOp.getType() == MachineOperandType::MO_Immediate);
    assert(immOp.getImm() == 42);

    auto negImm = MachineOperand::CreateImm(-10);
    assert(negImm.getImm() == -10);

    // Test 4: Create FrameIndex operand
    auto fiOp = MachineOperand::CreateFI(3);
    assert(fiOp.getType() == MachineOperandType::MO_FrameIndex);
    assert(fiOp.getFrameIndex() == 3);

    // Test 5: Create ExternalSymbol operand
    auto esOp = MachineOperand::CreateES("__print");
    assert(esOp.getType() == MachineOperandType::MO_ExternalSymbol);
    assert(esOp.getSymbolName() == "__print");

    // Test 6: Flag manipulation
    auto flagOp = MachineOperand::CreateReg(X86::BX, false, false, false);
    assert(!flagOp.isDef());
    flagOp.setIsDef(true);
    assert(flagOp.isDef());
    flagOp.setIsKill(true);
    assert(flagOp.isKill());
    flagOp.setIsDead(true);
    assert(flagOp.isDead());

    // Test 7: setReg
    flagOp.setReg(X86::SI);
    assert(flagOp.getReg() == X86::SI);

    std::cout << "[PASS] test_machine_operand" << std::endl;
    return 0;
}
