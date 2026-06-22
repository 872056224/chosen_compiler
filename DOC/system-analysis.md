# LL1 Compiler — 系统分析文档

> **面向 8086 的 LL(1) 教学型编译器设计与实现**
>
> 版本：MVP v2.0 | 日期：2026-06-22

---

## 摘要

本项目设计并实现了一个完整的编译器，将自设计的 LL(1) 高级语言（LL1 Language）编译为 Intel 8086 汇编代码，可在 emu8086 模拟器中运行。编译器采用 **LLVM 架构风格**，自实现了精简的中间表示（IR）框架、类 New PassManager 的优化管线（7 个 Pass）、**SelectionDAG 指令选择框架**、**虚拟寄存器系统**、线性扫描寄存器分配器、**可扩展 Target 抽象**以及 8086 代码生成器，覆盖了从词法分析、语法分析、语义分析、IR 生成、中端优化到目标代码生成的完整编译流程。

**关键词**：编译器、LL(1)文法、递归下降、中间表示、SSA、New PassManager、SelectionDAG、虚拟寄存器、线性扫描寄存器分配、8086汇编、代码生成

---

## 1. 项目概况

### 1.1 项目规模

| 指标 | 数值 |
|------|------|
| C++ 头文件 | 50 个 |
| C++ 源文件 | 44 个（库 39 + 工具 1 + 测试 17） |
| 总代码行数 | ~10,800 行（库/工具）+ ~1,200 行（测试） |
| CMake 库模块 | 12 个 |
| 单元测试 | 17 套（100% 通过） |
| Git 提交 | 50+ 次 |
| 示例程序 | 9 个 |
| 优化 Pass | 7 个 (Mem2Reg, PhiElim, InstCombine, Reassociate, GVN, SimplifyCFG, DCE) |

### 1.2 技术栈

| 层次 | 技术选择 |
|------|---------|
| 实现语言 | C++17 |
| 构建系统 | CMake 3.16+ |
| 编译器 | MinGW GCC 14.2.0 |
| 测试框架 | CTest (CMake 内置) |
| 目标平台 | Intel 8086 (16-bit) |
| 模拟器 | emu8086 |
| 版本控制 | Git + GitHub |

### 1.3 设计哲学

借鉴 LLVM 的三段式架构，将编译器划分为前端、中端、后端：

```
  LL1 源码
     │
     ▼
┌─────────┐
│  前端    │  Lexer → Parser → AST → Sema
└────┬────┘
     │ 语义 AST
     ▼
┌─────────┐
│  中端    │  IRGen → LLVM IR → Optimizer (New-PM, 7 Passes)
└────┬────┘
     │ 优化后 IR
     ▼
┌─────────┐
│  后端    │  SelectionDAG → ISel → RegAlloc → Printer
└────┬────┘
     │
     ▼
  8086 .asm
```

**优化管线（Optimizer）**：采用类 LLVM New PassManager 架构
```
ModulePassManager
  └── ModuleToFunctionPassAdaptor
        └── FunctionPassManager (对每个函数执行)
              └── Mem2Reg → PhiElimination → InstCombine
                    → Reassociate → GVN → SimplifyCFG → DCE
```

**后端管线（CodeGen）**：仿 LLVM 的 SelectionDAG 指令选择架构，双管线并存
```
旧管线: IR → CodeGen::generate() → 8086 .asm (直接文本生成)
新管线: IR → SDBuilder → SelectionDAG → Target8086ISel → MachineInstr
           → LinearScanRegAlloc → MCInstPrinter → 8086 .asm
```

**寄存器分配**：线性扫描算法（Next-Use 启发式），3 个通用寄存器 (AX/CX/DX)，支持 Spill/Reload。

遵循 **最小化**、**增量实现**、**可运行优先** 原则，每个阶段产出可验证的中间产物。

---

## 2. 源语言设计：LL1 Language

### 2.1 设计目标

设计一种语法满足 LL(1) 约束的教学型语言，支持基本的数据类型、控制流、函数以及输入输出，适合作为编译原理课程的实验对象。

### 2.2 类型系统

| 类型 | 关键字 | 宽度 | 说明 |
|------|--------|------|------|
| 整数 | `int` | 16-bit | 8086 原生宽度 |
| 字符 | `char` | 8-bit | ASCII 字符 |
| 布尔 | `bool` | 1-bit | true / false |
| 空 | `void` | — | 函数返回 |

隐式类型转换：`char → int`、`bool → int`。

### 2.3 语法特性

| 特性 | 语法 | 状态 |
|------|------|------|
| 变量声明 | `int a = 10;` | ✅ |
| 数组 | `int a[10]; a[i] = x;` | ✅ |
| 算术运算 | `+ - * / %` | ✅ |
| 比较运算 | `< > <= >= == !=` | ✅ |
| 逻辑运算 | `&& \|\| !` | ✅ |
| if/else | `if (cond) {...} else {...}` | ✅ |
| while 循环 | `while (cond) {...}` | ✅ |
| for 循环 | `for (init; cond; step) {...}` | ✅ |
| break/continue | `break;` / `continue;` | ✅ |
| 函数 | `fn int add(int a, int b) {...}` | ✅ |
| 输入 | `read()` | ✅ |
| 输出 | `print(expr);` | ✅ |
| 注释 | `// 单行注释` | ✅ |

### 2.4 运算符优先级

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 | `()` 函数调用, `[]` 下标 | 左 |
| 2 | `-` 取负, `!` 逻辑非 | 右 |
| 3 | `* / %` | 左 |
| 4 | `+ -` | 左 |
| 5 | `< > <= >=` | 左 |
| 6 | `== !=` | 左 |
| 7 | `&&` | 左 |
| 8 | `=` 赋值 | 右 |

---

## 3. 编译器架构

### 3.1 模块分解

编译器由 12 个 CMake OBJECT 库组成，每个库对应编译管线的一个阶段：

```
LL1-emu8086/
├── lib/
│   ├── Lex/         词法分析    (Lexer.cpp, Token.cpp)
│   ├── AST/         抽象语法树   (AST.cpp)
│   ├── Parse/       语法分析    (Parser.cpp)
│   ├── Sema/        语义分析    (Scope.cpp, Sema.cpp)
│   ├── IR/          中间表示框架 (Type, Value, User, Instruction,
│   │                              BasicBlock, Function, Module,
│   │                              IRBuilder, IRPrinter)
│   ├── IRGen/       IR 代码生成 (IRGen.cpp)
│   ├── Opt/         优化管线    (PassManager + 7 Passes +
│   │                              DominatorTree 分析)
│   ├── CodeGen/     目标代码生成 (CodeGen, MachineInstr/Operand,
│   │                 SelectionDAG/  MachineFunction, RegAlloc)
│   │   ├── SelectionDAG/   DAG 构建与工厂 (SDBuilder, SelectionDAG)
│   │   ├── Target/         Target 抽象基类 (TargetMachine,
│   │   │                    TargetRegisterInfo, TargetInstrInfo,
│   │   │                    TargetLowering)
│   │   └── Target8086/     8086 目标实现 (ISel, Lowering, Printer)
│   └── Driver/      编译器驱动   (Compiler.cpp)
├── tools/ll1c/      CLI 前端    (main.cpp)
├── test/            17 套单元测试 + 2 组编译流水线参考数据
└── examples/        9 个示例程序
```

### 3.2 模块依赖图

```
                    ┌──────┐
                    │Driver│
                    └──┬───┘
          ┌────────────┼──────────────┐
          ▼            ▼              ▼
       ┌─────┐    ┌───────┐    ┌─────────┐
       │ Lex │◄───│ Parse │◄───│  Sema   │
       └─────┘    └───┬───┘    └────┬────┘
                      │             │
                      ▼             ▼
                   ┌──────┐    ┌─────────┐
                   │ AST  │◄───│ SymbolTab│
                   └──┬───┘    └─────────┘
                      │
                      ▼
                  ┌───────┐
                  │ IRGen │
                  └───┬───┘
                      │
                      ▼
                  ┌───────┐          ┌──────────┐
                  │  IR   │◄─────────│ IRPrinter │ (.ll 输出)
                  └───┬───┘          └──────────┘
                 ┌────┴────────────┐
                 ▼                  ▼
            ┌────────┐      ┌──────────────┐
            │  Opt   │      │   CodeGen    │
            │7 Passes│      │ Old + New    │
            └────────┘      │ Machine IR   │
                            └──────┬───────┘
                                   │
                            ┌──────┴───────┐
                            ▼              ▼
                      ┌──────────┐  ┌────────────┐
                      │SelectionDAG│ │Target8086  │
                      │ + SDBuilder│ │ISel/Printer│
                      └──────────┘  └────────────┘
```

---

## 4. 前端：词法分析 + 语法分析 + 语义分析

### 4.1 词法分析器（Lexer）

**设计参考**：Clang 的 `Lexer.h`

以字符为单位读取源码，跳过空白符和注释，识别 token 并返回 Token 结构体（含类型、词素、源码位置）。

**Token 分类**（共 37 种）：

| 类别 | 数量 | 示例 |
|------|------|------|
| 关键字 | 16 | int, char, bool, if, else, while, for, fn, print, read, return, ... |
| 运算符 | 14 | +, -, *, /, %, <, >, <=, >=, ==, !=, &&, \|\|, !, = |
| 分隔符 | 8 | (, ), {, }, [, ], ;, , |
| 字面量 | 2 | TOK_NUMBER, TOK_IDENT |
| 特殊 | 2 | TOK_EOF, TOK_ERROR |

**关键实现**：多字符运算符（`<=` `>=` `==` `!=` `&&` `||`）的贪婪匹配，`//` 行注释的跳过，`std::unordered_map` 关键字查表。

### 4.2 语法分析器（Parser）

**设计参考**：Clang 的递归下降解析器

采用 LL(1) 递归下降算法。左递归已通过标准变换消除，表达式解析采用优先级递推法（Precedence Climbing）。

**核心解析方法**（15 个）：

```
parseProgram()         parseTopLevel()
parseFuncDef()         parseVarDecl()
parseStmt()            parseBlock()         parseIfStmt()
parseWhileStmt()       parseForStmt()       parseReturnStmt()
parsePrintStmt()
parseExpr()            parseAssignment()    parseLogicalOr()
parseLogicalAnd()      parseEquality()      parseRelational()
parseAdditive()        parseMultiplicative() parseUnary()
parsePrimary()
```

### 4.3 抽象语法树（AST）

**设计参考**：Clang 的三继承体系（Decl / Stmt / Expr）

```
ASTNode (基类，含 Kind 枚举 + 源码位置)
├── Decl (声明)
│   └── FuncDecl
├── Stmt (语句)
│   ├── Block (语句块)
│   ├── IfStmt / WhileStmt / ForStmt
│   ├── BreakStmt / ContinueStmt / ReturnStmt
│   ├── PrintStmt
│   ├── VarDecl (变量声明，兼作语句)
│   └── Expr (表达式，兼作语句)
│       ├── BinaryExpr / UnaryExpr / CallExpr
│       ├── IntegerLiteral / CharLiteral / BoolLiteral
│       ├── VarExpr / ArraySubscriptExpr
│       └── ReadExpr
```

### 4.4 语义分析器（Sema）

**设计参考**：Clang 的 `Sema` + `Scope` + 符号表

采用 **两遍遍历** 策略，覆盖 14 项检查：重复定义、未声明变量、类型兼容、条件 bool、return 匹配、参数数量/类型、break/continue 位置等。

---

## 5. 中端：LLVM 风格中间表示

### 5.1 IR 框架设计

**设计参考**：LLVM 的 `Value → User → Instruction` 继承体系

自实现了精简但完整的 SSA 形式 IR 框架，不依赖 LLVM 库：

```
Value (基类)
├── Argument     — 函数参数
├── BasicBlock   — 基本块（指令链表 + terminator）
├── Function     — 函数（参数 + 基本块列表）
├── Module       — 模块（函数列表）
├── ConstantInt  — 整数常量
└── User (使用其他 Value 的基类，含 def-use 链)
    └── Instruction
        ├── Terminator:     RetInst / BranchInst
        ├── Binary:         BinaryOpInst (add/sub/mul/sdiv/srem/and/or/xor)
        ├── Compare:        ICmpInst (eq/ne/slt/sle/sgt/sge)
        ├── Memory:         AllocaInst / LoadInst / StoreInst
        ├── Array:          ArrayLoadInst / ArrayStoreInst
        ├── Call:           CallInst
        ├── Phi:            PhiInst
        └── Cast:           SExt / ZExt / Trunc
```

### 5.2 核心机制

| 机制 | 说明 |
|------|------|
| **Use-Def 链** | 每个 `User` 持有操作数引用，每个 `Value` 持有使用列表 |
| **RAUW** | `replaceAllUsesWith()` 替换所有引用，SSA 变换核心 |
| **BasicBlock terminator** | 每个 BB 必须以 `ret` 或 `br` 结尾 |
| **IRBuilder** | 仿 LLVM `IRBuilder`，基于插入点的指令工厂，自动插入到当前 BB |

### 5.3 IR 代码生成（IRGen）

将语义校验后的 AST 翻译为 SSA 形式的 IR。支持变量声明、算术表达式、比较、if/while/for 控制流、函数调用、数组操作、break/continue 等全部语言特性。

### 5.4 IR 文本输出（IRPrinter）

`--emit-llvm` 标志输出标准 `.ll` 格式。每个 Value 自动编号，支持 `define`、`alloca`、`store`、`load`、`add`、`icmp`、`br`、`ret`、`call`、`phi`、`arrayload`/`arraystore` 等指令的格式化输出。

---

## 6. 优化管线（Optimizer）

### 6.1 架构设计

采用类 LLVM New PassManager 架构，支持惰性分析计算与结果失效化：

```
AnalysisKey → AnalysisManager::getResult<T>()
PreservedAnalyses → 控制分析缓存生命周期
ModuleToFunctionPassAdaptor → 将 FunctionPass 适配为 ModulePass
```

### 6.2 7 个 Pass 概览

| # | Pass | 类型 | 作用 |
|---|------|------|------|
| 1 | **Mem2Reg** | Function | alloca/load/store → SSA phi 节点（IDF Phi 放置 + 支配树重命名 + 退化 Phi 消除） |
| 2 | **PhiElimination** | Function | Phi 指令 → alloca+store+load（为 CodeGen 降级，旧管线使用） |
| 3 | **InstCombine** | Function | 常量折叠 + 代数化简（x+0→x, x*1→x, x*0→0 等 20+ 条规则） |
| 4 | **Reassociate** | Function | 表达式重结合优化运算顺序 |
| 5 | **GVN** | Function | 全局值编号，消除公共子表达式 |
| 6 | **SimplifyCFG** | Function | 控制流简化（消除不可达块、合并分支、消除空 BB） |
| 7 | **DCE** | Function | 死代码消除（标记式：可达指令标记 + 未标记删除） |

### 6.3 DominatorTree 分析

采用 **Cooper-Harvey-Kennedy 迭代数据流算法**，计算：
- **IDom**：每个基本块的直接支配者（迭代至不动点）
- **Dominance Frontier**：支配边界（IDF 计算的基础）
- **DomTree Children + LevelOrder**：支配树遍历结构

为 Mem2Reg 的 Phi 放置和重命名提供基础分析。

### 6.4 管线可视化

提供 `--dump-pass-ir=<dir>` CLI 标志，在每个 Pass 执行后自动将 IR 写入指定目录，文件命名格式为 `NN_after_PassName_FuncName.ll`，便于观察每个 Pass 的效果。

---

## 7. 后端：双管线架构

### 7.1 旧管线（直接文本生成）

直接将 LLVM IR 映射为 8086 汇编文本，使用 BP 帧指针 + BX 基址管理栈空间，简单 round-robin 寄存器分配。

### 7.2 新管线（Machine IR 流水线）⚠️ 本次新增

仿 LLVM 后端架构，完整实现五阶段 Machine IR 流水线：

```
LLVM IR (优化后)
  │
  ▼
SelectionDAGBuilder::visit(Function)    ← IR → DAG 转换
  │  alloca → FrameIndex
  │  Phi → CopyFromReg/CopyToReg (COPY-based 消除)
  ▼
SelectionDAG                             ← DAG 中间表示
  │  SDNode + SDValue + chain token
  ▼
Target8086ISel::runOnFunction()         ← 指令选择
  │  模式匹配: ISD::ADD → MCOpcode::ADD
  │  输出 MachineInstr 序列
  ▼
MachineFunction                          ← Machine IR
  │  MachineInstr + MachineOperand (5 种类型)
  │  Virtual Registers (bit 31 标志)
  ▼
LinearScanRegAlloc::run()               ← 寄存器分配
  │  LiveInterval 分析
  │  physreg 分配 + spill/reload
  │  COPY 合并
  ▼
Target8086MCInstPrinter::print()        ← 汇编打印
  │  MachineInstr → 8086 text
  ▼
8086 .asm
```

### 7.3 核心数据结构

#### Register（虚拟寄存器系统）

```cpp
using Register = uint32_t;
// bit 31 = 1 → 虚拟寄存器, bit 31 = 0 → 物理寄存器
// NoRegister = 0xFFFFFFFF  (哨兵值)

// 8086 物理寄存器: AX=1, CX=2, DX=3, BX=4, SP=5, BP=6, SI=7, DI=8
// 虚拟寄存器: index 0 → 0x80000000, index 1 → 0x80000001, ...
```

#### MachineOperand（5 种操作数类型）

| 类型 | 说明 | 示例 |
|------|------|------|
| `MO_Register` | 虚拟/物理寄存器 | `%vreg0`, `ax` |
| `MO_Immediate` | 立即数 | `42`, `100` |
| `MO_FrameIndex` | 栈帧槽索引 | `FI(0)`, `FI(1)` |
| `MO_MachineBasicBlock` | 目标基本块 | `MBB("main_entry_0")` |
| `MO_ExternalSymbol` | 外部符号 | `__print`, `__read` |

#### MachineInstr（30 种操作码）

```
Terminator: RET, JMP, JE, JNE, JL, JLE, JG, JGE
Arithmetic: ADD, SUB, MUL, DIV, AND, OR, XOR, SHL, SHR
Move:      MOV, PUSH, POP, COPY
Compare:   CMP
Call:      CALL
```

#### SelectionDAG

- **29 个 ISD 操作码**：ADD, SUB, MUL, LOAD, STORE, BR, BR_CC, CALL, RET, SETCC, CopyFromReg, CopyToReg, Constant, FrameIndex, TokenFactor, EntryToken 等
- **SDNode**：含 Payload 联合体（ConstVal, FrameIdx, RegNum, CallCallee, TargetMBB, CmpPred）
- **BumpPtrAllocator**：高效 SDNode 内存分配
- **Chain Token**：内存操作顺序约束

### 7.4 Target 抽象体系

```
TargetMachine (工厂)
├── TargetRegisterInfo (寄存器描述 + 寄存器类)
├── TargetInstrInfo (指令格式 + genLoad/genStore)
└── TargetLowering (操作合法化表)

Target8086 实现:
├── Target8086RegisterInfo: AX=1..DI=8, GR16_ABCD 可分配类
├── Target8086InstrInfo: 8086 指令格式
├── Target8086Lowering: 操作合法/Expand 表
├── Target8086ISel: 模式匹配 (手写，无 TableGen)
└── Target8086MCInstPrinter: MachineInstr → 8086 文本
```

### 7.5 Phi 消除策略对比

| 策略 | 旧管线 | 新管线 |
|------|--------|--------|
| 方法 | PhiElimination Pass: Phi → alloca+store+load | SDBuilder 中 Phi → CopyFromReg/CopyToReg |
| 中间步骤 | 引入内存操作 | 引入虚拟寄存器 COPY |
| 寄存器分配 | 后续处理 alloca/load/store | RegAlloc 合并 COPY（同 physreg → 消除） |
| 效率 | 有内存回退开销 | 更接近 LLVM 的实现方式 |

---

## 8. 编译器驱动程序

### 8.1 CLI 接口

```
ll1c <source.ll1> [-o output] [--emit-llvm] [--opt] [--new-codegen] [--dump-pass-ir=<dir>]

  source.ll1          输入源文件
  -o <file>           指定输出文件（默认 output.asm）
  --emit-llvm         输出 LLVM IR (.ll) 而非 8086 汇编
  --opt               启用优化管线
  --new-codegen       使用新 Machine IR 管线（SDAG→ISel→RA→Print）
  --dump-pass-ir=<dir> 在每个 Pass 后将 IR dump 到指定目录
```

### 8.2 驱动流程

```
Compiler::compileFile(source):
  1. Lexer:   source → token stream
  2. Parser:  tokens → AST
  3. Sema:    AST → validated AST
  4. IRGen:   AST → LLVM IR Module
  5. CodeGen: IR Module → 8086 .asm

Compiler::compileFileOpt(source):
  1-4. (同上)
  5. PassBuilder → 7 Pass 优化管线
  6. CodeGen → .asm

Compiler::compileFileNew(source):
  1-4. (同上)
  5. SDBuilder → SelectionDAG → ISel → RegAlloc → Printer → .asm

Compiler::compileFileNewOpt(source):
  1-4. (同上)
  5. PassBuilder → 7 Pass 优化管线
  6. SDBuilder → SelectionDAG → ISel → RegAlloc → Printer → .asm
```

---

## 9. 测试体系

### 9.1 测试覆盖

| 模块 | 测试数 | 文件 |
|------|--------|------|
| IR 核心 | 6 | test_type, test_value, test_instruction, test_bb, test_function, test_irbuilder |
| 前端 | 4 | test_lexer, test_ast, test_parser, test_sema |
| IRGen | 1 | test_irgen |
| 旧后端 | 1 | test_codegen |
| 新 Machine IR 后端 | 4 | test_register, test_machine_operand, test_selection_dag, test_new_pipeline |
| 集成 | 1 | test_e2e |
| **总计** | **17** | **100% 通过** |

### 9.2 编译流水线参考数据

- **test/passdemo/** — Pass 演示：源码 + 每个 Pass 后的 IR dump + 最终汇编 + 效果分析 summary
- **test/bubbletest/** — 冒泡排序全链路：源码 + 逐 Pass IR + 最终汇编 + summary

### 9.3 运行方式

```bash
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest                       # 全部 17 个测试
ctest -R Register -V        # 单个测试
```

---

## 10. 示例程序

### 10.1 冒泡排序（完整算法验证）

编译 `examples/bubble_sort.ll1`，在 emu8086 中运行，输出 20 个排序后的整数：
`13 16 17 45 48 53 58 72 115 126 141 217 280 303 328 347 378 380 380 457`

### 10.2 其他示例

| 文件 | 说明 |
|------|------|
| `basic.ll1` | if/else 分支 |
| `loop.ll1` | while 循环累加 |
| `io_test.ll1` | print + read 输入输出 |
| `array_test.ll1` | 数组声明、赋值、读取 |
| `bubble_sort.ll1` | 冒泡排序 |
| `func_call.ll1` | 函数调用 |
| `for_test.ll1` | for 循环 |
| `break_test.ll1` | break 语句 |
| `char_test.ll1` | char 类型 |

---

## 11. 设计启发与参考

| 参考源 | 对应模块 | 借鉴内容 |
|--------|---------|---------|
| LLVM IR | lib/IR | Value→User→Instruction 体系、RAUW、IRBuilder |
| LLVM SelectionDAG | lib/CodeGen/SelectionDAG | SDNode/SDValue、Chain Token、Legalize 框架 |
| LLVM MC | lib/CodeGen | MachineInstr/MachineOperand、Register、MCOpcode |
| LLVM RegAlloc | lib/CodeGen/RegAlloc | 线性扫描分配、LiveInterval、Spill/Reload |
| Clang | lib/Parse, lib/Sema | 递归下降解析、Decl/Stmt/Expr 体系、两遍语义分析 |
| LLVM PassBuilder | lib/Opt | Pass 管线注册与执行、AnalysisManager |
| X86 Target | lib/CodeGen/Target8086 | TargetMachine 抽象、ISel 模式 |
| emu8086 | 目标输出 | 寻址模式、DOS 中断、cdecl 调用约定 |

---

## 12. 总结与展望

### 12.1 已实现功能

- ✅ 完整的 LL(1) 语言前端（Lexer + Parser + AST + Sema）
- ✅ LLVM 风格 SSA 中间表示框架（Value/User/Instruction/BB/Function/Module/IRBuilder）
- ✅ AST → IR 代码生成（全部语言特性）
- ✅ 优化管线：类 New PassManager，7 个 Pass + DominatorTree 分析
- ✅ 全函数跨 BB Mem2Reg（IDF Phi 放置 + 支配树重命名 + 退化 Phi 消除）
- ✅ **虚拟寄存器系统**（Register = uint32_t, phys/vreg 区分）
- ✅ **MachineInstr/MachineOperand**（5 种操作数类型, 30 种操作码）
- ✅ **SelectionDAG 框架**（29 个 ISD 操作码, BumpPtrAllocator, Chain Token）
- ✅ **SelectionDAGBuilder**（IR → DAG Visitor, Phi → COPY 降低）
- ✅ **8086 指令选择器**（手写模式匹配, BR_CC predicate 映射）
- ✅ **线性扫描寄存器分配**（LiveInterval 分析, Spill/Reload, COPY 合并）
- ✅ **可扩展 Target 抽象**（TargetMachine/TRI/TII/TLI）
- ✅ IR → 8086 汇编代码生成（双管线：旧 + 新 Machine IR）
- ✅ 旧管线：BP 帧指针 + BX 基址 + cdecl 调用约定 + 数组 [BX+SI] 寻址
- ✅ print / read I/O 运行时（DOS `int 21h`）
- ✅ LLVM IR 文本输出（`.ll` 格式）
- ✅ 永久 `--dump-pass-ir=<dir>` CLI 功能
- ✅ 完整回归测试（17 套，100% 通过）
- ✅ 编译流水线参考数据（passdemo + bubbletest）

### 12.2 后续扩展方向

- [ ] 多维数组
- [ ] RISC-V 后端扩展（利用 Target 抽象体系）
- [ ] TableGen 风格的模式描述语言
- [ ] 更高级的寄存器分配（图着色）
- [ ] 指令调度器
- [ ] Link-Time Optimization (LTO)
