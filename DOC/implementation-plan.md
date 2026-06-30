# LL1 Compiler MVP — Implementation Plan

> **For agentic workers:** Each task uses checkbox (`- [ ]`) syntax for tracking. Tasks are ordered by dependency — complete each before moving to the next.

**Goal:** Build an end-to-end compiler from LL(1) language to 8086 assembly, covering variables, expressions, if/else, while loops. Single `main()` function (no function calls in MVP).

**Architecture:** C++17, CMake build, self-implemented LLVM-like IR framework. 8 library modules (Lex, Parse, AST, Sema, IR, IRGen, CodeGen, Driver) + 1 tool (ll1c). Test-driven development per module.

**Tech Stack:** C++17, CMake 3.16+, GoogleTest, Git

---

## Global Constraints

- C++17 standard (`-std=c++17`)
- CMake 3.16+ with `enable_testing()`
- All public headers under `include/ll1/<Module>/`
- All implementation under `lib/<Module>/`
- All tests under `test/<Module>/`
- Each module is a CMake OBJECT library
- LL(1) recursive descent parser — single token lookahead
- IR is SSA form with BasicBlock terminators (ret/br)
- 8086 output targets emu8086-compatible Intel syntax assembly
- YAGNI: Implement only what MVP needs, no over-engineering

---

## File Structure Map

```
LL1-emu8086/
├── CMakeLists.txt                          # Top-level: add_subdirectory for all
├── include/ll1/
│   ├── IR/
│   │   ├── Type.h                          # TypeKind enum + Type class (Void, Int1, Int8, Int16)
│   │   ├── Value.h                         # Value base (Kind, Type*, name, uses)
│   │   ├── User.h                          # User : Value (operands)
│   │   ├── Instruction.h                   # Instruction : User (opcode enum)
│   │   ├── BasicBlock.h                    # BasicBlock : Value (inst list, terminator)
│   │   ├── Function.h                      # Function : Value (args, blocks)
│   │   ├── Module.h                        # Module (name, function list)
│   │   ├── IRBuilder.h                     # IRBuilder (insertion point, factory methods)
│   │   └── LLVMContext.h                   # LLVMContext (type singletons)
│   ├── Lex/
│   │   ├── Token.h                         # TokenKind enum + Token struct
│   │   └── Lexer.h                         # Lexer class
│   ├── AST/
│   │   └── AST.h                           # All AST nodes (single header for MVP)
│   ├── Parse/
│   │   └── Parser.h                        # Recursive descent parser
│   ├── Sema/
│   │   ├── Scope.h                         # Scope + Symbol
│   │   └── Sema.h                          # Sema + DiagnosticEngine
│   ├── IRGen/
│   │   └── IRGen.h                         # AST → IR visitor
│   ├── CodeGen/
│   │   ├── TargetRegisterInfo.h            # Abstract register info
│   │   ├── TargetInstrInfo.h               # Abstract instruction info
│   │   ├── MCInst.h                        # Machine instruction IR
│   │   ├── ISelLowering.h                  # IR → MIR lowering framework
│   │   ├── RegAlloc.h                      # Linear scan register allocator
│   │   ├── AsmEmitter.h                    # Assembly text emitter (abstract)
│   │   └── Target8086/
│   │       ├── Target8086RegisterInfo.h    # 8086 register descriptions
│   │       ├── Target8086InstrInfo.h       # 8086 instruction matching
│   │       ├── Target8086ISelLowering.h    # 8086-specific lowering
│   │       └── Target8086AsmEmitter.h      # 8086 Intel syntax emitter
│   └── Driver/
│       └── Compiler.h                      # Compiler driver pipeline
├── lib/
│   ├── CMakeLists.txt                       # All library targets
│   ├── IR/  (Type.cpp Value.cpp User.cpp Instruction.cpp
│   │         BasicBlock.cpp Function.cpp Module.cpp IRBuilder.cpp)
│   ├── Lex/  (Lexer.cpp)
│   ├── AST/  (AST.cpp)
│   ├── Parse/ (Parser.cpp)
│   ├── Sema/ (Scope.cpp Sema.cpp)
│   ├── IRGen/ (IRGen.cpp)
│   ├── CodeGen/ (MCInst.cpp ISelLowering.cpp RegAlloc.cpp
│   │             Target8086RegisterInfo.cpp Target8086InstrInfo.cpp
│   │             Target8086ISelLowering.cpp Target8086AsmEmitter.cpp)
│   └── Driver/ (Compiler.cpp)
├── tools/ll1c/main.cpp
└── test/
    ├── CMakeLists.txt
    ├── IR/   (test_type.cpp test_value.cpp test_instruction.cpp ...)
    ├── Lex/  (test_lexer.cpp)
    ├── Parse/ (test_parser.cpp)
    ├── Sema/  (test_sema.cpp)
    ├── IRGen/ (test_irgen.cpp)
    ├── CodeGen/ (test_isel.cpp test_regalloc.cpp test_asm.cpp)
    └── Integration/ (test_e2e.cpp)
```

---

## Phase 0: Project Scaffolding

### Task 0.1: Top-level CMake + directory creation

**Files:**
- Create: `CMakeLists.txt`
- Create: `include/ll1/` (empty directory structure)
- Create: `lib/` (empty directory structure)
- Create: `tools/ll1c/` (empty directory)
- Create: `test/` (empty directory)
- Create: `examples/` (empty directory)

**Produces:** Buildable (but empty) project that `cmake --build .` succeeds on.

- [ ] **Step 1: Create top-level CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(ll1-compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Subdirectories
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(test)

# Examples directory (not built, just data)
```

- [ ] **Step 2: Create lib/CMakeLists.txt (scaffold only)**

```cmake
# Placeholder — libraries added as we implement modules
```

- [ ] **Step 3: Create tools/ll1c/CMakeLists.txt (scaffold only)**

```cmake
add_executable(ll1c main.cpp)
target_link_libraries(ll1c PRIVATE ll1Driver)
```

- [ ] **Step 4: Create placeholder tools/ll1c/main.cpp**

```cpp
int main(int argc, char **argv) {
    // Placeholder — will wire up the full pipeline
    return 0;
}
```

- [ ] **Step 5: Create test/CMakeLists.txt (scaffold only)**

```cmake
enable_testing()
# Placeholder — test targets added as we implement modules
```

- [ ] **Step 6: Create empty file markers for directory structure**

```bash
mkdir -p include/ll1/IR
mkdir -p include/ll1/Lex
mkdir -p include/ll1/AST
mkdir -p include/ll1/Parse
mkdir -p include/ll1/Sema
mkdir -p include/ll1/IRGen
mkdir -p include/ll1/CodeGen/Target8086
mkdir -p include/ll1/Opt/Passes
mkdir -p include/ll1/Driver
mkdir -p lib/IR lib/Lex lib/AST lib/Parse lib/Sema
mkdir -p lib/IRGen lib/CodeGen/Target8086 lib/Opt lib/Driver
mkdir -p test/IR test/Lex test/Parse test/Sema
mkdir -p test/IRGen test/CodeGen test/Integration
mkdir -p tools/ll1c
mkdir -p examples
```

- [ ] **Step 7: Configure and build to verify**

```bash
cd LL1-emu8086
mkdir -p build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

Expected: Build succeeds with empty libraries, produces `ll1c` executable.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat: project scaffolding with CMake build system"
```

---

## Phase 1: IR Core (Foundation)

The IR core is the foundation. Everything else depends on it. We build it first, bottom-up: Type → Value → User → Instruction → BasicBlock → Function → Module → IRBuilder.

### Task 1.1: Type system

**Files:**
- Create: `include/ll1/IR/Type.h`
- Create: `lib/IR/Type.cpp`
- Create: `test/IR/test_type.cpp`

**Interfaces:**
- Produces: `class Type` with `TypeKind { Void, Int1, Int8, Int16 }`, static factory methods `getVoidTy()`, `getInt1Ty()`, `getInt8Ty()`, `getInt16Ty()`

- [ ] **Step 1: Write the failing test**

```cpp
// test/IR/test_type.cpp
#include <ll1/IR/Type.h>
#include <cassert>
#include <iostream>

int main() {
    // Test: integer types exist and are singletons
    auto *i1 = ll1::Type::getInt1Ty();
    auto *i8 = ll1::Type::getInt8Ty();
    auto *i16 = ll1::Type::getInt16Ty();
    auto *v = ll1::Type::getVoidTy();

    assert(i1->getKind() == ll1::Type::TypeKind::Int1);
    assert(i8->getKind() == ll1::Type::TypeKind::Int8);
    assert(i16->getKind() == ll1::Type::TypeKind::Int16);
    assert(v->getKind() == ll1::Type::TypeKind::Void);

    // Test: singletons (same pointer on repeated calls)
    assert(ll1::Type::getInt16Ty() == i16);
    assert(ll1::Type::getVoidTy() == v);

    std::cout << "Type tests passed.\n";
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cd build && cmake --build . --target test_type 2>&1 || true
```

Expected: Compilation error — `Type.h` not found.

- [ ] **Step 3: Write Type.h**

```cpp
#ifndef LL1_IR_TYPE_H
#define LL1_IR_TYPE_H

namespace ll1 {

class Type {
public:
    enum class TypeKind { Void, Int1, Int8, Int16 };

    TypeKind getKind() const { return Kind; }

    static Type *getVoidTy();
    static Type *getInt1Ty();
    static Type *getInt8Ty();
    static Type *getInt16Ty();

private:
    Type(TypeKind k) : Kind(k) {}
    TypeKind Kind;

    static Type VoidTy;
    static Type Int1Ty;
    static Type Int8Ty;
    static Type Int16Ty;
};

} // namespace ll1

#endif
```

- [ ] **Step 4: Write Type.cpp**

```cpp
#include <ll1/IR/Type.h>

namespace ll1 {

Type Type::VoidTy(TypeKind::Void);
Type Type::Int1Ty(TypeKind::Int1);
Type Type::Int8Ty(TypeKind::Int8);
Type Type::Int16Ty(TypeKind::Int16);

Type *Type::getVoidTy() { return &VoidTy; }
Type *Type::getInt1Ty() { return &Int1Ty; }
Type *Type::getInt8Ty() { return &Int8Ty; }
Type *Type::getInt16Ty() { return &Int16Ty; }

} // namespace ll1
```

- [ ] **Step 5: Update lib/CMakeLists.txt to add IR library**

```cmake
add_library(ll1IR OBJECT
    IR/Type.cpp
)
target_include_directories(ll1IR PUBLIC ${CMAKE_SOURCE_DIR}/include)
```

- [ ] **Step 6: Update test/CMakeLists.txt for type test**

```cmake
add_executable(test_type IR/test_type.cpp)
target_link_libraries(test_type PRIVATE ll1IR)
add_test(NAME Type COMMAND test_type)
```

- [ ] **Step 7: Build and run test**

```bash
cd build && cmake .. && cmake --build . && ctest -R Type
```

Expected: Test passes.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(ir): add Type system (Void, Int1, Int8, Int16 singletons)"
```

---

### Task 1.2: Value base class

**Files:**
- Create: `include/ll1/IR/Value.h`
- Create: `lib/IR/Value.cpp`
- Create: `test/IR/test_value.cpp`

**Interfaces:**
- Consumes: `Type`
- Produces: `class Value` with `ValueKind` enum, name, type, use-list, `replaceAllUsesWith()`

- [ ] **Step 1: Write Value.h (first draft — name, type, kind only)**

```cpp
#ifndef LL1_IR_VALUE_H
#define LL1_IR_VALUE_H

#include <ll1/IR/Type.h>
#include <string>
#include <vector>

namespace ll1 {

class User;

class Value {
public:
    enum class ValueKind {
        Argument, BasicBlock, Function, Module,
        Instruction, Constant, GlobalVariable
    };

    Value(ValueKind k, Type *ty, const std::string &name = "");
    virtual ~Value() = default;

    ValueKind getKind() const { return Kind; }
    Type *getType() const { return Ty; }

    const std::string &getName() const { return Name; }
    void setName(const std::string &name) { Name = name; }

    // Use-list management
    void addUse(User *u);
    void removeUse(User *u);
    const std::vector<User *> &getUses() const { return Uses; }
    unsigned getNumUses() const { return Uses.size(); }

    // RAUW — core SSA operation
    void replaceAllUsesWith(Value *newVal);

private:
    ValueKind Kind;
    Type *Ty;
    std::string Name;
    std::vector<User *> Uses;
};

} // namespace ll1

#endif
```

- [ ] **Step 2: Write User.h (needed by Value's use-list)**

```cpp
#ifndef LL1_IR_USER_H
#define LL1_IR_USER_H

#include <ll1/IR/Value.h>
#include <vector>

namespace ll1 {

class User : public Value {
public:
    User(ValueKind k, Type *ty, const std::string &name = "")
        : Value(k, ty, name) {}

    unsigned getNumOperands() const { return Operands.size(); }
    Value *getOperand(unsigned i) const;
    void setOperand(unsigned i, Value *v);

protected:
    std::vector<Value *> Operands;
};

} // namespace ll1

#endif
```

- [ ] **Step 3: Write Value.cpp**

```cpp
#include <ll1/IR/Value.h>
#include <ll1/IR/User.h>
#include <algorithm>

namespace ll1 {

Value::Value(ValueKind k, Type *ty, const std::string &name)
    : Kind(k), Ty(ty), Name(name) {}

void Value::addUse(User *u) {
    Uses.push_back(u);
}

void Value::removeUse(User *u) {
    Uses.erase(std::remove(Uses.begin(), Uses.end(), u), Uses.end());
}

void Value::replaceAllUsesWith(Value *newVal) {
    // Iterate over a copy because setOperand modifies Uses
    auto usesCopy = Uses;
    for (auto *u : usesCopy) {
        for (unsigned i = 0; i < u->getNumOperands(); ++i) {
            if (u->getOperand(i) == this) {
                u->setOperand(i, newVal);
            }
        }
    }
}

} // namespace ll1
```

- [ ] **Step 4: Write User.cpp**

```cpp
#include <ll1/IR/User.h>

namespace ll1 {

Value *User::getOperand(unsigned i) const {
    return Operands.at(i);
}

void User::setOperand(unsigned i, Value *v) {
    // Remove this user from old operand's use-list
    if (Operands[i]) {
        Operands[i]->removeUse(this);
    }
    Operands[i] = v;
    // Add this user to new operand's use-list
    if (v) {
        v->addUse(this);
    }
}

} // namespace ll1
```

- [ ] **Step 5: Write test for Value + User**

```cpp
// test/IR/test_value.cpp
#include <ll1/IR/Value.h>
#include <ll1/IR/User.h>
#include <cassert>
#include <iostream>

// A minimal concrete User subclass for testing
class TestUser : public ll1::User {
public:
    TestUser(ll1::Value *op1, ll1::Value *op2)
        : User(ll1::Value::ValueKind::Instruction,
               ll1::Type::getInt16Ty()) {
        Operands.push_back(nullptr);
        Operands.push_back(nullptr);
        setOperand(0, op1);
        setOperand(1, op2);
    }
};

int main() {
    // Test: Value creation with name and type
    ll1::Value v(ll1::Value::ValueKind::Argument,
                 ll1::Type::getInt16Ty(), "x");
    assert(v.getName() == "x");
    assert(v.getType() == ll1::Type::getInt16Ty());
    assert(v.getNumUses() == 0);

    // Test: User tracks operands and use-lists
    ll1::Value v2(ll1::Value::ValueKind::Argument,
                  ll1::Type::getInt16Ty(), "y");
    TestUser u(&v, &v2);

    assert(u.getOperand(0) == &v);
    assert(u.getOperand(1) == &v2);
    assert(v.getNumUses() == 1);   // v is used by u
    assert(v2.getNumUses() == 1);  // v2 is used by u

    // Test: RAUW
    ll1::Value v3(ll1::Value::ValueKind::Argument,
                  ll1::Type::getInt16Ty(), "z");
    v.replaceAllUsesWith(&v3);

    assert(v.getNumUses() == 0);   // u no longer uses v
    assert(u.getOperand(0) == &v3); // u now uses v3
    assert(v3.getNumUses() == 1);   // v3 is now used by u

    std::cout << "Value tests passed.\n";
    return 0;
}
```

- [ ] **Step 6: Build, run test, commit**

```bash
cd build && cmake .. && cmake --build . && ctest -R Value
```

Expected: Test passes. Then commit.

---

### Task 1.3: Instruction hierarchy

**Files:**
- Create: `include/ll1/IR/Instruction.h`
- Create: `lib/IR/Instruction.cpp`
- Create: `test/IR/test_instruction.cpp`

**Interfaces:**
- Consumes: `Value`, `User`, `Type`
- Produces: `class Instruction : User` with `Opcode` enum, `getParent()`, concrete instruction subclasses

- [ ] **Step 1: Write Instruction.h**

```cpp
#ifndef LL1_IR_INSTRUCTION_H
#define LL1_IR_INSTRUCTION_H

#include <ll1/IR/User.h>
#include <ll1/IR/BasicBlock.h>  // forward needed

namespace ll1 {

class BasicBlock;

class Instruction : public User {
public:
    enum class Opcode {
        // Terminators
        Ret, Br,
        // Binary arithmetic
        Add, Sub, Mul, SDiv, SRem,
        // Bitwise
        And, Or, Xor,
        // Compare
        ICmp,
        // Memory
        Alloca, Load, Store,
        // Other
        Call, Phi,
        // Cast
        SExt, ZExt, Trunc,
    };

    Instruction(Opcode op, Type *ty, const std::string &name = "");

    Opcode getOpcode() const { return Op; }
    BasicBlock *getParent() const { return Parent; }
    void setParent(BasicBlock *bb) { Parent = bb; }

    Instruction *getNext() const { return Next; }
    void setNext(Instruction *next) { Next = next; }

    // LLVM-style isa/cast support via opcode
    static bool classof(const Value *v);

private:
    Opcode Op;
    BasicBlock *Parent = nullptr;
    Instruction *Next = nullptr;
};

// ====== Concrete Instructions ======

class RetInst : public Instruction {
public:
    RetInst(Value *retVal = nullptr);  // nullptr = void ret
    Value *getReturnValue() const;
    static bool classof(const Value *v);
};

class BranchInst : public Instruction {
public:
    // Unconditional
    BranchInst(BasicBlock *dest);
    // Conditional
    BranchInst(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB);

    bool isConditional() const;
    bool isUnconditional() const;
    Value *getCondition() const;
    BasicBlock *getTrueDest() const;
    BasicBlock *getFalseDest() const;
    BasicBlock *getUnconditionalDest() const;
    static bool classof(const Value *v);
};

class BinaryOpInst : public Instruction {
public:
    BinaryOpInst(Opcode op, Value *lhs, Value *rhs, const std::string &name = "");
    Value *getLHS() const { return getOperand(0); }
    Value *getRHS() const { return getOperand(1); }
    static bool classof(const Value *v);
};

class ICmpInst : public Instruction {
public:
    enum Pred { EQ, NE, SLT, SLE, SGT, SGE };

    ICmpInst(Pred p, Value *lhs, Value *rhs, const std::string &name = "");
    Pred getPredicate() const;
    static Pred negate(Pred p);
    static bool classof(const Value *v);
};

class AllocaInst : public Instruction {
public:
    AllocaInst(Type *allocatedType, const std::string &name = "");
    Type *getAllocatedType() const;
    static bool classof(const Value *v);
};

class LoadInst : public Instruction {
public:
    LoadInst(Type *ty, Value *ptr, const std::string &name = "");
    Value *getPointer() const { return getOperand(0); }
    static bool classof(const Value *v);
};

class StoreInst : public Instruction {
public:
    StoreInst(Value *val, Value *ptr);
    Value *getValue() const { return getOperand(0); }
    Value *getPointer() const { return getOperand(1); }
    static bool classof(const Value *v);
};

class PhiInst : public Instruction {
public:
    PhiInst(Type *ty, const std::string &name = "");
    void addIncoming(Value *v, BasicBlock *bb);
    unsigned getNumIncoming() const { return IncomingBlocks.size(); }
    Value *getIncomingValue(unsigned i) const { return getOperand(i); }
    BasicBlock *getIncomingBlock(unsigned i) const { return IncomingBlocks[i]; }
    static bool classof(const Value *v);
private:
    std::vector<BasicBlock *> IncomingBlocks;
};

} // namespace ll1

#endif
```

- [ ] **Step 2: Write Instruction.cpp**

```cpp
#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>

namespace ll1 {

// Instruction base
Instruction::Instruction(Opcode op, Type *ty, const std::string &name)
    : User(ValueKind::Instruction, ty, name), Op(op) {}

bool Instruction::classof(const Value *v) {
    return v->getKind() == ValueKind::Instruction;
}

// RetInst
RetInst::RetInst(Value *retVal)
    : Instruction(Opcode::Ret, Type::getVoidTy()) {
    if (retVal) Operands.push_back(nullptr);
    if (retVal) setOperand(0, retVal);
}
Value *RetInst::getReturnValue() const {
    return Operands.empty() ? nullptr : getOperand(0);
}
bool RetInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Ret;
}

// BranchInst
BranchInst::BranchInst(BasicBlock *dest)
    : Instruction(Opcode::Br, Type::getVoidTy()) {
    Operands.push_back(dest);
}
BranchInst::BranchInst(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB)
    : Instruction(Opcode::Br, Type::getVoidTy()) {
    Operands.push_back(nullptr);  // cond
    Operands.push_back(trueBB);
    Operands.push_back(falseBB);
    setOperand(0, cond);
}
bool BranchInst::isConditional() const { return Operands.size() == 3; }
bool BranchInst::isUnconditional() const { return Operands.size() == 1; }
Value *BranchInst::getCondition() const { return isConditional() ? getOperand(0) : nullptr; }
BasicBlock *BranchInst::getTrueDest() const {
    return isConditional() ? static_cast<BasicBlock*>(getOperand(1)) : nullptr;
}
BasicBlock *BranchInst::getFalseDest() const {
    return isConditional() ? static_cast<BasicBlock*>(getOperand(2)) : nullptr;
}
BasicBlock *BranchInst::getUnconditionalDest() const {
    return isUnconditional() ? static_cast<BasicBlock*>(getOperand(0)) : nullptr;
}
bool BranchInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Br;
}

// BinaryOpInst
BinaryOpInst::BinaryOpInst(Opcode op, Value *lhs, Value *rhs, const std::string &name)
    : Instruction(op, lhs->getType(), name) {
    Operands.push_back(nullptr);
    Operands.push_back(nullptr);
    setOperand(0, lhs);
    setOperand(1, rhs);
}
bool BinaryOpInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && (I->getOpcode() >= Opcode::Add && I->getOpcode() <= Opcode::SRem);
}

// ICmpInst
ICmpInst::ICmpInst(Pred p, Value *lhs, Value *rhs, const std::string &name)
    : Instruction(Opcode::ICmp, Type::getInt1Ty(), name) {
    (void)p; // Pred stored implicitly via operand types for now
    Operands.push_back(nullptr);
    Operands.push_back(nullptr);
    setOperand(0, lhs);
    setOperand(1, rhs);
}
bool ICmpInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::ICmp;
}

// AllocaInst
AllocaInst::AllocaInst(Type *allocatedType, const std::string &name)
    : Instruction(Opcode::Alloca, allocatedType, name) {}
Type *AllocaInst::getAllocatedType() const { return getType(); }
bool AllocaInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Alloca;
}

// LoadInst
LoadInst::LoadInst(Type *ty, Value *ptr, const std::string &name)
    : Instruction(Opcode::Load, ty, name) {
    Operands.push_back(nullptr);
    setOperand(0, ptr);
}
bool LoadInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Load;
}

// StoreInst
StoreInst::StoreInst(Value *val, Value *ptr)
    : Instruction(Opcode::Store, Type::getVoidTy()) {
    Operands.push_back(nullptr);
    Operands.push_back(nullptr);
    setOperand(0, val);
    setOperand(1, ptr);
}
bool StoreInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Store;
}

// PhiInst
PhiInst::PhiInst(Type *ty, const std::string &name)
    : Instruction(Opcode::Phi, ty, name) {}
void PhiInst::addIncoming(Value *v, BasicBlock *bb) {
    unsigned idx = Operands.size();
    Operands.push_back(nullptr);
    setOperand(idx, v);
    IncomingBlocks.push_back(bb);
}
bool PhiInst::classof(const Value *v) {
    auto *I = dynamic_cast<const Instruction*>(v);
    return I && I->getOpcode() == Opcode::Phi;
}

} // namespace ll1
```

- [ ] **Step 3: Write test for instructions**

```cpp
// test/IR/test_instruction.cpp
#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Function.h>
#include <cassert>
#include <iostream>

int main() {
    // Test: BinaryOpInst
    ll1::Value lhs(ll1::Value::ValueKind::Argument, ll1::Type::getInt16Ty(), "a");
    ll1::Value rhs(ll1::Value::ValueKind::Argument, ll1::Type::getInt16Ty(), "b");
    ll1::BinaryOpInst add(ll1::Instruction::Opcode::Add, &lhs, &rhs, "tmp");
    assert(add.getOpcode() == ll1::Instruction::Opcode::Add);
    assert(add.getLHS() == &lhs);
    assert(add.getRHS() == &rhs);
    assert(add.getType() == ll1::Type::getInt16Ty());

    // Test: RetInst (void)
    ll1::RetInst retVoid;
    assert(retVoid.getReturnValue() == nullptr);

    // Test: RetInst (with value)
    ll1::RetInst retVal(&lhs);
    assert(retVal.getReturnValue() == &lhs);

    // Test: AllocaInst
    ll1::AllocaInst alloca(ll1::Type::getInt16Ty(), "x");
    assert(alloca.getAllocatedType() == ll1::Type::getInt16Ty());

    // Test: Store + Load
    // Note: alloca returns a pointer-typed value; for now type is the allocated type
    ll1::StoreInst store(&rhs, &alloca);
    assert(store.getValue() == &rhs);
    assert(store.getPointer() == &alloca);
    ll1::LoadInst load(ll1::Type::getInt16Ty(), &alloca, "loaded");
    assert(load.getPointer() == &alloca);
    assert(load.getType() == ll1::Type::getInt16Ty());

    std::cout << "Instruction tests passed.\n";
    return 0;
}
```

- [ ] **Step 4: Forward-declare BasicBlock for Instruction.cpp to compile**

Since Instruction.cpp needs `BasicBlock` only as pointer type, add forward decl in Instruction.h or include a minimal BasicBlock forward header. For now, add to Instruction.h:

```cpp
namespace ll1 { class BasicBlock; }
```

- [ ] **Step 5: Build, run test, commit**

```bash
cd build && cmake .. && cmake --build . && ctest -R Instruction
```

Expected: Test passes (BasicBlock link will be satisfied in next task — for now, keep test_instruction standalone, or add a stub BasicBlock class first).

**Note:** Instruction.cpp references `BasicBlock` via pointer only, so forward declaration is sufficient. The test creates instructions without BasicBlock parents.

- [ ] **Step 6: Commit**

```bash
git commit -m "feat(ir): add Instruction hierarchy (Ret, Br, Binary, ICmp, Alloca, Load, Store, Phi)"
```

---

### Task 1.4: BasicBlock, Function, Module

**Files:**
- Create: `include/ll1/IR/BasicBlock.h`
- Create: `include/ll1/IR/Function.h`
- Create: `include/ll1/IR/Module.h`
- Create: `lib/IR/BasicBlock.cpp`
- Create: `lib/IR/Function.cpp`
- Create: `lib/IR/Module.cpp`
- Create: `test/IR/test_basicblock.cpp`
- Create: `test/IR/test_function.cpp`

**Interfaces:**
- Consumes: `Value`, `User`, `Instruction`, `Type`
- Produces: `BasicBlock` (inst list + terminator), `Function` (args + BBs), `Module` (function list)

- [ ] **Step 1: Write BasicBlock.h**

```cpp
#ifndef LL1_IR_BASICBLOCK_H
#define LL1_IR_BASICBLOCK_H

#include <ll1/IR/Value.h>
#include <list>
#include <memory>
#include <string>

namespace ll1 {

class Instruction;
class Function;

class BasicBlock : public Value {
public:
    using InstList = std::list<std::unique_ptr<Instruction>>;
    using iterator = InstList::iterator;
    using const_iterator = InstList::const_iterator;

    explicit BasicBlock(const std::string &name = "");

    // Instruction list management
    InstList &getInstList() { return Instructions; }
    Instruction *getTerminator();
    const Instruction *getTerminator() const;

    // LLVM-style: push_back inserts before terminator if present
    void pushBack(std::unique_ptr<Instruction> inst);
    void pushFront(std::unique_ptr<Instruction> inst);

    iterator begin() { return Instructions.begin(); }
    iterator end() { return Instructions.end(); }
    const_iterator begin() const { return Instructions.begin(); }
    const_iterator end() const { return Instructions.end(); }

    bool empty() const { return Instructions.empty(); }
    size_t size() const { return Instructions.size(); }

    Function *getParent() const { return Parent; }
    void setParent(Function *f) { Parent = f; }

    static bool classof(const Value *v);

private:
    InstList Instructions;
    Function *Parent = nullptr;
};

} // namespace ll1

#endif
```

- [ ] **Step 2: Write BasicBlock.cpp**

```cpp
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>

namespace ll1 {

BasicBlock::BasicBlock(const std::string &name)
    : Value(ValueKind::BasicBlock, Type::getVoidTy(), name) {}

Instruction *BasicBlock::getTerminator() {
    if (Instructions.empty()) return nullptr;
    auto *last = Instructions.back().get();
    auto op = last->getOpcode();
    if (op == Instruction::Opcode::Ret || op == Instruction::Opcode::Br) {
        return last;
    }
    return nullptr;
}

const Instruction *BasicBlock::getTerminator() const {
    return const_cast<BasicBlock*>(this)->getTerminator();
}

void BasicBlock::pushBack(std::unique_ptr<Instruction> inst) {
    inst->setParent(this);
    Instructions.push_back(std::move(inst));
}

void BasicBlock::pushFront(std::unique_ptr<Instruction> inst) {
    inst->setParent(this);
    Instructions.push_front(std::move(inst));
}

bool BasicBlock::classof(const Value *v) {
    return v->getKind() == ValueKind::BasicBlock;
}

} // namespace ll1
```

- [ ] **Step 3: Write Function.h + Function.cpp**

```cpp
// Function.h
#ifndef LL1_IR_FUNCTION_H
#define LL1_IR_FUNCTION_H

#include <ll1/IR/Value.h>
#include <vector>
#include <memory>
#include <string>

namespace ll1 {

class BasicBlock;
class Argument;

class Argument : public Value {
public:
    Argument(Type *ty, const std::string &name = "");
    static bool classof(const Value *v);
};

class Function : public Value {
public:
    using ArgList = std::vector<std::unique_ptr<Argument>>;
    using BBList = std::vector<std::unique_ptr<BasicBlock>>;

    Function(Type *returnTy, const std::string &name = "");

    // Arguments
    ArgList &getArgs() { return Arguments; }
    Argument *getArg(unsigned i) { return Arguments[i].get(); }
    unsigned getArgCount() const { return Arguments.size(); }
    void addArg(std::unique_ptr<Argument> arg);

    // BasicBlocks
    BBList &getBasicBlocks() { return Blocks; }
    BasicBlock *getEntryBlock();
    BasicBlock *createBasicBlock(const std::string &name = "");
    void addBasicBlock(std::unique_ptr<BasicBlock> bb);

    Type *getReturnType() const { return ReturnType; }

    static bool classof(const Value *v);

private:
    ArgList Arguments;
    BBList Blocks;
    Type *ReturnType;
};

} // namespace ll1

#endif
```

```cpp
// Function.cpp
#include <ll1/IR/Function.h>
#include <ll1/IR/BasicBlock.h>

namespace ll1 {

Argument::Argument(Type *ty, const std::string &name)
    : Value(ValueKind::Argument, ty, name) {}
bool Argument::classof(const Value *v) { return v->getKind() == ValueKind::Argument; }

Function::Function(Type *returnTy, const std::string &name)
    : Value(ValueKind::Function, returnTy, name), ReturnType(returnTy) {}

void Function::addArg(std::unique_ptr<Argument> arg) {
    Arguments.push_back(std::move(arg));
}

BasicBlock *Function::getEntryBlock() {
    return Blocks.empty() ? nullptr : Blocks.front().get();
}

BasicBlock *Function::createBasicBlock(const std::string &name) {
    auto bb = std::make_unique<BasicBlock>(name);
    auto *raw = bb.get();
    raw->setParent(this);
    Blocks.push_back(std::move(bb));
    return raw;
}

void Function::addBasicBlock(std::unique_ptr<BasicBlock> bb) {
    bb->setParent(this);
    Blocks.push_back(std::move(bb));
}

bool Function::classof(const Value *v) {
    return v->getKind() == ValueKind::Function;
}

} // namespace ll1
```

- [ ] **Step 4: Write Module.h + Module.cpp**

```cpp
// Module.h
#ifndef LL1_IR_MODULE_H
#define LL1_IR_MODULE_H

#include <ll1/IR/Value.h>
#include <vector>
#include <memory>
#include <string>

namespace ll1 {

class Function;
class GlobalVariable;

class Module {
public:
    explicit Module(const std::string &name = "");

    const std::string &getModuleName() const { return Name; }

    // Function management
    const std::vector<std::unique_ptr<Function>> &getFunctionList() const { return Functions; }
    Function *getFunction(const std::string &name);
    Function *createFunction(Type *returnTy, const std::string &name);
    void addFunction(std::unique_ptr<Function> fn);

private:
    std::string Name;
    std::vector<std::unique_ptr<Function>> Functions;
};

} // namespace ll1

#endif
```

```cpp
// Module.cpp
#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>

namespace ll1 {

Module::Module(const std::string &name) : Name(name) {}

Function *Module::getFunction(const std::string &name) {
    for (auto &fn : Functions) {
        if (fn->getName() == name) return fn.get();
    }
    return nullptr;
}

Function *Module::createFunction(Type *returnTy, const std::string &name) {
    auto fn = std::make_unique<Function>(returnTy, name);
    auto *raw = fn.get();
    Functions.push_back(std::move(fn));
    return raw;
}

void Module::addFunction(std::unique_ptr<Function> fn) {
    Functions.push_back(std::move(fn));
}

} // namespace ll1
```

- [ ] **Step 5: Write tests**

```cpp
// test/IR/test_basicblock.cpp
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Instruction.h>
#include <cassert>
#include <iostream>

int main() {
    ll1::BasicBlock bb("entry");
    assert(bb.getName() == "entry");
    assert(bb.empty());
    assert(bb.getTerminator() == nullptr);

    // Add a ret instruction
    bb.pushBack(std::make_unique<ll1::RetInst>());
    assert(bb.size() == 1);
    assert(bb.getTerminator() != nullptr);
    assert(bb.getTerminator()->getOpcode() == ll1::Instruction::Opcode::Ret);

    std::cout << "BasicBlock tests passed.\n";
    return 0;
}
```

```cpp
// test/IR/test_function.cpp
#include <ll1/IR/Function.h>
#include <ll1/IR/Module.h>
#include <ll1/IR/Instruction.h>
#include <cassert>
#include <iostream>

int main() {
    // Test Module
    ll1::Module mod("test");
    assert(mod.getModuleName() == "test");

    // Test Function
    auto *fn = mod.createFunction(ll1::Type::getInt16Ty(), "main");
    assert(fn->getName() == "main");
    assert(fn->getReturnType() == ll1::Type::getInt16Ty());

    // Add argument
    fn->addArg(std::make_unique<ll1::Argument>(ll1::Type::getInt16Ty(), "argc"));
    assert(fn->getArgCount() == 1);
    assert(fn->getArg(0)->getName() == "argc");

    // Add basic blocks
    auto *entry = fn->createBasicBlock("entry");
    assert(fn->getEntryBlock() == entry);
    assert(fn->getBasicBlocks().size() == 1);

    // Add instruction to block
    entry->pushBack(std::make_unique<ll1::RetInst>());
    assert(entry->getTerminator() != nullptr);

    // Module lookup
    assert(mod.getFunction("main") == fn);
    assert(mod.getFunction("nonexistent") == nullptr);

    std::cout << "Function/Module tests passed.\n";
    return 0;
}
```

- [ ] **Step 6: Build, test, commit**

```bash
cd build && cmake .. && cmake --build . && ctest
```

---

### Task 1.5: IRBuilder and LLVMContext

**Files:**
- Create: `include/ll1/IR/LLVMContext.h`
- Create: `include/ll1/IR/IRBuilder.h`
- Create: `lib/IR/IRBuilder.cpp`
- Create: `test/IR/test_irbuilder.cpp`

**Interfaces:**
- Consumes: All IR types
- Produces: `LLVMContext` (type cache), `IRBuilder` (insertion-point-based instruction factory)

- [ ] **Step 1: Write LLVMContext.h**

```cpp
#ifndef LL1_IR_LLVMCONTEXT_H
#define LL1_IR_LLVMCONTEXT_H

#include <ll1/IR/Type.h>

namespace ll1 {

class LLVMContext {
public:
    Type *getVoidTy()  { return Type::getVoidTy(); }
    Type *getInt1Ty()  { return Type::getInt1Ty(); }
    Type *getInt8Ty()  { return Type::getInt8Ty(); }
    Type *getInt16Ty() { return Type::getInt16Ty(); }
};

} // namespace ll1
#endif
```

- [ ] **Step 2: Write IRBuilder.h (core methods only for MVP)**

```cpp
#ifndef LL1_IR_IRBUILDER_H
#define LL1_IR_IRBUILDER_H

#include <ll1/IR/Instruction.h>
#include <ll1/IR/BasicBlock.h>
#include <ll1/IR/Function.h>
#include <ll1/IR/LLVMContext.h>
#include <memory>
#include <string>

namespace ll1 {

class IRBuilder {
public:
    explicit IRBuilder(LLVMContext &ctx);

    void setInsertPoint(BasicBlock *bb);

    BasicBlock *getInsertBlock() const { return CurrentBB; }

    // Arithmetic
    BinaryOpInst *CreateAdd(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateSub(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateMul(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateSDiv(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateSRem(Value *lhs, Value *rhs, const std::string &name = "");

    // Bitwise
    BinaryOpInst *CreateAnd(Value *lhs, Value *rhs, const std::string &name = "");
    BinaryOpInst *CreateOr(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateNeg(Value *v, const std::string &name = "");
    Value *CreateNot(Value *v, const std::string &name = "");

    // Compare
    ICmpInst *CreateICmpEQ(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpNE(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSLT(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSLE(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSGT(Value *lhs, Value *rhs, const std::string &name = "");
    ICmpInst *CreateICmpSGE(Value *lhs, Value *rhs, const std::string &name = "");

    // Memory
    AllocaInst *CreateAlloca(Type *ty, const std::string &name = "");
    LoadInst *CreateLoad(Type *ty, Value *ptr, const std::string &name = "");
    StoreInst *CreateStore(Value *val, Value *ptr);

    // Control flow
    RetInst *CreateRetVoid();
    RetInst *CreateRet(Value *v);
    BranchInst *CreateBr(BasicBlock *dest);
    BranchInst *CreateCondBr(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB);

    // Phi
    PhiInst *CreatePhi(Type *ty, const std::string &name = "");

    // Cast
    Value *CreateSExt(Value *v, Type *destTy, const std::string &name = "");
    Value *CreateZExt(Value *v, Type *destTy, const std::string &name = "");
    Value *CreateTrunc(Value *v, Type *destTy, const std::string &name = "");

    LLVMContext &getContext() { return Context; }

private:
    LLVMContext &Context;
    BasicBlock *CurrentBB = nullptr;
};

} // namespace ll1

#endif
```

- [ ] **Step 3: Write IRBuilder.cpp**

```cpp
#include <ll1/IR/IRBuilder.h>

namespace ll1 {

IRBuilder::IRBuilder(LLVMContext &ctx) : Context(ctx) {}

void IRBuilder::setInsertPoint(BasicBlock *bb) { CurrentBB = bb; }

// Helper to auto-insert into current block
template<typename T, typename... Args>
static T *insert(IRBuilder *B, Args&&... args) {
    auto inst = std::make_unique<T>(std::forward<Args>(args)...);
    auto *raw = inst.get();
    if (B->getInsertBlock()) {
        B->getInsertBlock()->pushBack(std::move(inst));
    }
    return raw;
}

BinaryOpInst *IRBuilder::CreateAdd(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Add, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateSub(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Sub, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateMul(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Mul, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateSDiv(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::SDiv, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateSRem(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::SRem, lhs, rhs, name);
}

BinaryOpInst *IRBuilder::CreateAnd(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::And, lhs, rhs, name);
}
BinaryOpInst *IRBuilder::CreateOr(Value *lhs, Value *rhs, const std::string &name) {
    return insert<BinaryOpInst>(this, Instruction::Opcode::Or, lhs, rhs, name);
}

Value *IRBuilder::CreateNeg(Value *v, const std::string &name) {
    auto *zero = CreateAnd(v, v, name + ".zero"); // hack — use ConstantInt later
    (void)zero;
    return insert<BinaryOpInst>(this, Instruction::Opcode::Sub,
        /* use i16 0 constant — placeholder for now */ nullptr, v, name);
}
Value *IRBuilder::CreateNot(Value *v, const std::string &name) {
    // !x = (x == 0)
    // Placeholder: CreateICmpEQ(v, 0, name)
    return nullptr; // FIXME: implement with constant
}

ICmpInst *IRBuilder::CreateICmpEQ(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::EQ, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpNE(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::NE, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSLT(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SLT, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSLE(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SLE, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSGT(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SGT, lhs, rhs, name);
}
ICmpInst *IRBuilder::CreateICmpSGE(Value *lhs, Value *rhs, const std::string &name) {
    return insert<ICmpInst>(this, ICmpInst::SGE, lhs, rhs, name);
}

AllocaInst *IRBuilder::CreateAlloca(Type *ty, const std::string &name) {
    return insert<AllocaInst>(this, ty, name);
}
LoadInst *IRBuilder::CreateLoad(Type *ty, Value *ptr, const std::string &name) {
    return insert<LoadInst>(this, ty, ptr, name);
}
StoreInst *IRBuilder::CreateStore(Value *val, Value *ptr) {
    return insert<StoreInst>(this, val, ptr);
}

RetInst *IRBuilder::CreateRetVoid() {
    return insert<RetInst>(this, nullptr);
}
RetInst *IRBuilder::CreateRet(Value *v) {
    return insert<RetInst>(this, v);
}
BranchInst *IRBuilder::CreateBr(BasicBlock *dest) {
    return insert<BranchInst>(this, dest);
}
BranchInst *IRBuilder::CreateCondBr(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB) {
    return insert<BranchInst>(this, cond, trueBB, falseBB);
}

PhiInst *IRBuilder::CreatePhi(Type *ty, const std::string &name) {
    return insert<PhiInst>(this, ty, name);
}

// Cast operations (stub for MVP)
Value *IRBuilder::CreateSExt(Value *v, Type *destTy, const std::string &name) {
    return v; // FIXME: implement properly with ConstantInt support
}
Value *IRBuilder::CreateZExt(Value *v, Type *destTy, const std::string &name) {
    return v; // FIXME
}
Value *IRBuilder::CreateTrunc(Value *v, Type *destTy, const std::string &name) {
    return v; // FIXME
}

} // namespace ll1
```

**Note:** The `CreateNeg`, `CreateNot`, and `CreateCast*` methods need `ConstantInt` which we haven't added yet. We'll add a simplified `ConstantInt` class in Task 1.6 before using these. For MVP `CreateNeg` and `CreateNot` can be omitted from initial test coverage.

- [ ] **Step 4: Write test**

```cpp
// test/IR/test_irbuilder.cpp
#include <ll1/IR/IRBuilder.h>
#include <cassert>
#include <iostream>

int main() {
    ll1::LLVMContext ctx;
    ll1::IRBuilder builder(ctx);

    // Create a function with a basic block
    ll1::Module mod("test");
    auto *fn = mod.createFunction(ll1::Type::getInt16Ty(), "main");
    auto *entry = fn->createBasicBlock("entry");

    builder.setInsertPoint(entry);

    // Create alloca
    auto *alloca = builder.CreateAlloca(ctx.getInt16Ty(), "x");
    assert(alloca != nullptr);
    assert(alloca->getAllocatedType() == ctx.getInt16Ty());

    // Create add
    ll1::Value tmp(ll1::Value::ValueKind::Argument, ctx.getInt16Ty());
    auto *add = builder.CreateAdd(alloca, &tmp, "sum");
    assert(add != nullptr);
    assert(add->getOpcode() == ll1::Instruction::Opcode::Add);

    // Create icmp
    auto *cmp = builder.CreateICmpSLT(add, &tmp, "cond");
    assert(cmp != nullptr);
    assert(cmp->getType() == ctx.getInt1Ty());

    // Create branches
    auto *trueBB = fn->createBasicBlock("true");
    auto *falseBB = fn->createBasicBlock("false");
    auto *br = builder.CreateCondBr(cmp, trueBB, falseBB);
    assert(br->isConditional());

    // Verify block has instructions
    assert(entry->size() == 4); // alloca, add, icmp, br

    std::cout << "IRBuilder tests passed.\n";
    return 0;
}
```

- [ ] **Step 5: Add ConstantInt for completeness**

```cpp
// Add to include/ll1/IR/Instruction.h or a new Constant.h
class ConstantInt : public Value {
    int16_t Val;
public:
    ConstantInt(Type *ty, int16_t val) : Value(ValueKind::Constant, ty), Val(val) {}
    int16_t getValue() const { return Val; }
    static ConstantInt *get(Type *ty, int16_t val);
};
```

Implement in `lib/IR/Instruction.cpp` and fix IRBuilder's `CreateNeg`/`CreateNot`.

- [ ] **Step 6: Build, test, commit**

---

## Phase 2: Lexer

### Task 2.1: Token definitions

### Task 2.2: Lexer implementation

*(Phase 2 through Phase 8 tasks continue with the same TDD pattern. See below for summary of remaining phases — detailed task expansion happens during implementation.)*

---

## Phase Summary (Phases 2-9)

### Phase 2: Lexer (2 tasks)
- Task 2.1: Token.h + TokenKind enum + Token struct (source location)
- Task 2.2: Lexer.h/cpp — character-by-character lexer, keyword table, returns Token stream

### Phase 3: AST (2 tasks)
- Task 3.1: AST.h — all node types (Decl: VarDecl/FuncDecl, Stmt: Block/If/While/For/Break/Continue/Return, Expr: Binary/Unary/Call/Literal/Var)
- Task 3.2: AST.cpp — constructors and basic visitors

### Phase 4: Parser (2 tasks)
- Task 4.1: Parser expression parsing (Operator precedence climbing)
- Task 4.2: Parser statement/declaration parsing (recursive descent)

### Phase 5: Sema (2 tasks)
- Task 5.1: Scope + SymbolTable
- Task 5.2: Sema + DiagnosticEngine (two-pass analysis)

### Phase 6: IRGen (3 tasks)
- Task 6.1: Expression codegen (Binary, Unary, Literal, Var)
- Task 6.2: Statement codegen (Block, If, While, Return)
- Task 6.3: Declaration codegen (VarDecl, FuncDef)

### Phase 7: CodeGen (5 tasks)
- Task 7.1: MCInst + TargetRegisterInfo + TargetInstrInfo (abstract)
- Task 7.2: Target8086 register and instruction descriptions
- Task 7.3: ISelLowering (IR → MCInst)
- Task 7.4: RegAlloc (linear scan)
- Task 7.5: Target8086AsmEmitter (Intel syntax output)

### Phase 8: Driver (1 task)
- Task 8.1: Compiler.h/cpp — pipeline orchestration: Lex→Parse→Sema→IRGen→CodeGen

### Phase 9: Integration (2 tasks)
- Task 9.1: End-to-end test: `int main() { int a=1; int b=2; return a+b; }` → .asm
- Task 9.2: While loop test: `int main() { int i=0; while(i<10) i=i+1; return i; }` → .asm

---

## Risk Analysis

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| IR design too heavyweight | Medium | High | Keep Instruction hierarchy flat; defer non-MVP ops |
| Parser left-recursion bugs | Low | Medium | Validated LL(1) grammar; extensive test cases |
| 8086 addressing modes complex | Medium | Medium | Start with simple [bp+offset] only; expand later |
| Phi node insertion difficult | High | High | **Defer mem2reg to Phase 2**; MVP uses alloca/load/store directly |
| Register allocation spills | Medium | Medium | For MVP, reserve AX/BX/CX for temps; spill to stack for complex cases |
| Build system complexity | Low | Low | Started with simple CMake scaffolding |

---

## Scope Boundaries

### MVP IN scope:
- Lexer: All tokens defined in spec (including for, break, continue — parsed but not codegen'd)
- Parser: Full LL(1) grammar (including for, break, continue, function calls)
- AST: Complete node set
- Sema: Full type checking
- IRGen: Variables, expressions, if/else, while — single main() function only
- CodeGen: Direct alloca/load/store → stack-based 8086 code (no mem2reg)
- Driver: Full pipeline

### MVP OUT of scope:
- Function calls / parameters (parsed but IRGen stubbed)
- for loops (parsed but IRGen stubbed)
- break/continue (parsed but IRGen stubbed)
- char/bool types in CodeGen (i8 → i16 sign extension for MVP)
- Optimization passes (Phase 2)
- Register allocation (Phase 2 — MVP uses stack-only mapping)
- Multiple source files

---

**Next:** Expand Phase 2-9 tasks to full detail during implementation. Each task follows the TDD pattern established in Phase 1.
