#include <ll1/CodeGen/Register.h>
#include <cassert>
#include <iostream>

using namespace ll1;

int main() {
    // Test 1: NoRegister sentinel
    assert(NoRegister == 0xFFFFFFFFu);
    assert(!isValidRegister(NoRegister));

    // Test 2: Physical register detection
    assert(isPhysicalRegister(X86::AX));
    assert(isPhysicalRegister(X86::CX));
    assert(isPhysicalRegister(X86::DX));
    assert(!isVirtualRegister(X86::AX));

    // Test 3: Virtual register creation and detection
    Register v1 = index2VirtReg(0);  // FirstVirtualRegister
    assert(isVirtualRegister(v1));
    assert(!isPhysicalRegister(v1));
    assert(v1 >= FirstVirtualRegister);

    Register v2 = index2VirtReg(5);
    assert(isVirtualRegister(v2));
    assert(virtReg2Index(v2) == 5);

    // Test 4: Index round-trip
    for (unsigned i = 0; i < 100; ++i) {
        Register v = index2VirtReg(i);
        assert(isVirtualRegister(v));
        assert(virtReg2Index(v) == i);
    }

    // Test 5: Physical register numbering
    assert(X86::AX == 1);
    assert(X86::CX == 2);
    assert(X86::DX == 3);
    assert(X86::BX == 4);
    assert(X86::SP == 5);
    assert(X86::BP == 6);
    assert(X86::SI == 7);
    assert(X86::DI == 8);

    // Test 6: isValidRegister
    assert(isValidRegister(X86::AX));
    assert(!isValidRegister(NoRegister));

    std::cout << "[PASS] test_register" << std::endl;
    return 0;
}
