# LL1 Compiler — 系统分析文档

> **面向 8086 的 LL(1) 教学型编译器设计与实现**
>
> 版本：MVP v1.0 | 日期：2026-06-21

---

## 摘要

本项目设计并实现了一个完整的编译器，将自设计的 LL(1) 高级语言（LL1 Language）编译为 Intel 8086 汇编代码，可在 emu8086 模拟器中运行。编译器采用 **LLVM 架构风格**，自实现了精简的中间表示（IR）框架和 8086 代码生成器，覆盖了从词法分析、语法分析、语义分析、IR 生成、IR 输出到目标代码生成的完整编译流程。

**关键词**：编译器、LL(1)文法、递归下降、中间表示、8086汇编、代码生成

---

## 1. 项目概况

### 1.1 项目规模

| 指标 | 数值 |
|------|------|
| C++ 源文件（.h + .cpp） | 50 个 |
| 总代码行数 | ~5,046 行 |
| 模块数 | 9 个独立 CMake 库 |
| 单元测试 | 13 套 |
| Git 提交 | 30 次 |
| 示例程序 | 5 个 |

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
│  中端    │  IRGen → LLVM IR → IRPrinter (.ll)
└────┬────┘
     │ LLVM IR
     ▼
┌─────────┐
│  后端    │  CodeGen → 8086 汇编 (.asm)
└─────────┘
```

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
| 函数 | `fn int add(int a, int b) {...}` | ✅ |
| 输入 | `read()` | ✅ |
| 输出 | `print(expr);` | ✅ |
| 注释 | `// 单行注释` | ✅ |
| for/break/continue | — | 解析通过，IRGen 待实现 |

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

编译器由 9 个 CMake OBJECT 库组成，每个库对应编译管线的一个阶段：

```
LL1-emu8086/
├── lib/
│   ├── Lex/      词法分析    (Lexer.cpp)
│   ├── AST/      抽象语法树   (AST.cpp)
│   ├── Parse/    语法分析    (Parser.cpp)
│   ├── Sema/     语义分析    (Scope.cpp, Sema.cpp)
│   ├── IR/       中间表示框架 (Type, Value, User, Instruction, BasicBlock, Function, Module, IRBuilder, IRPrinter)
│   ├── IRGen/    IR 代码生成 (IRGen.cpp)
│   ├── CodeGen/  目标代码生成 (CodeGen.cpp)
│   └── Driver/   编译器驱动   (Compiler.cpp)
├── tools/ll1c/   CLI 前端    (main.cpp)
├── test/         13 套单元测试
└── examples/     5 个示例程序
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
                      │
                      ▼
                  ┌────────┐
                  │ CodeGen │ → 8086 .asm
                  └────────┘
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

**关键实现**：数组声明 `int a[10]` 与数组下标 `a[i]` 的区分解析，`read()` 作为无参表达式，`print(expr)` 作为语句。

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

所有节点间通过 `std::unique_ptr` 表达树形所有权，Sema 后 `VarExpr` 通过裸指针指回对应声明。

### 4.4 语义分析器（Sema）

**设计参考**：Clang 的 `Sema` + `Scope` + 符号表

采用 **两遍遍历** 策略：
- Pass 1：收集所有顶层声明（函数 + 全局变量），插入全局作用域
- Pass 2：对每个函数体，进入函数作用域，插入参数，递归遍历语句/表达式进行类型检查

**符号表结构**：

```
SymbolTable
└── Scope (链表栈)
    ├── Global Scope
    │   ├── func: "main" → FuncDecl
    │   └── var:  "x" → VarDecl
    └── Function Scope ← enterScope / exitScope
        ├── param: "a" → ParmDecl
        ├── local: "i" → VarDecl
        └── Block Scope (嵌套块)
```

**检查项**（共 14 项）：

| 类别 | 检查内容 |
|------|---------|
| 声明 | 重复定义、数组大小非正 |
| 引用 | 未声明变量、函数名用作变量 |
| 类型 | 运算操作数兼容、赋值兼容、条件必须 bool、return 匹配 |
| 函数 | 参数数量匹配、参数类型匹配、非 void 必须有返回值 |
| 控制流 | break/continue 仅循环内、return 仅函数内 |
| 数组 | 下标类型为 int、数组名不可直接使用 |
| I/O | print 参数仅 int/char |

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

将语义校验后的 AST 翻译为 SSA 形式的 IR。

**映射示例**：

```c
int a = 10;
```
```llvm
%a = alloca i16
store i16 10, i16 %a
```

```c
if (a < b) { return b; }
```
```llvm
%lt_tmp = icmp slt i16 %a_val, %b_val
br i1 %lt_tmp, label %then, label %if_end
then:
  ret i16 %b_val
if_end:
  ret i16 %a_val
```

```c
a[i] = 42;
```
```llvm
arraystore i16 42, %a, i
```

### 5.4 IR 文本输出（IRPrinter）

`--emit-llvm` 标志输出标准 `.ll` 格式。每个 Value 自动编号，支持 `define`、`alloca`、`store`、`load`、`add`、`icmp`、`br`、`ret`、`call`、`phi`、`arrayload`/`arraystore` 等指令的格式化输出。

---

## 6. 后端：8086 代码生成

### 6.1 设计考量

LLVM 官方不支持 16 位 8086 目标。本项目的 CodeGen 自实现了从 LLVM IR 到 8086 汇编的完整映射。

**特点**：
- 采用 BX 相对寻址管理变量空间（`mov bx, 8000h` 作为基址）
- 数组元素通过 `[BX+SI]` 基址变址寻址访问
- SI 专门用于数组索引计算，AX/CX/DX 为临时寄存器
- 运行时库（`__print` / `__read`）通过 DOS 中断（`int 21h`）实现

### 6.2 IR → 8086 映射表

| LLVM IR | 8086 汇编 |
|---------|----------|
| `alloca i16` | 在 BX 空间中分配 2 字节 |
| `store i16 X, ptr %p` | `mov [bx+offset], reg` |
| `load i16, ptr %p` | `mov reg, [bx+offset]` |
| `add i16 %a, %b` | `add reg_a, reg_b` |
| `icmp slt %a, %b` | `cmp reg_a, reg_b` |
| `br label %dest` | `jmp label` |
| `br i1 %cond, %t, %f` | `cmp ..., reg_b` + `jcc label_t` + `jmp label_f` |
| `ret i16 %val` | `mov ax, val` + `ret` |
| `call @__print(%x)` | `mov ax, x` + `call __print` |
| `call @__read()` | `call __read`，返回 AX |
| `arrayload %arr, %idx` | `mov si, idx, shl si,1, mov reg,[bx+si]` |
| `arraystore %val, %arr, %idx` | `mov si, idx, shl si,1, mov [bx+si], val` |

### 6.3 运行时库

编译器在每个汇编输出末尾嵌入两个运行时函数：

**`__print`**：将 AX 中的整数转为十进制字符串并通过 `int 21h / ah=02h` 逐字符输出，末尾附加一个空格。

**`__read`**：通过 `int 21h / ah=01h` 逐字符读取，转换 ASCII 数字为整数存入 AX，以回车（0Dh）作为结束标志。

---

## 7. 编译器驱动程序

### 7.1 CLI 接口

```
ll1c <source.ll1> [-o output] [--emit-llvm]

  source.ll1    输入源文件
  -o <file>     指定输出文件（默认 output.asm）
  --emit-llvm   输出 LLVM IR (.ll) 而非 8086 汇编
```

### 7.2 驱动流程

```cpp
Compiler::compile(source):
  1. Lexer:   source → token stream
  2. Parser:  tokens → AST
  3. Sema:    AST → validated AST + symbol table
  4. IRGen:   AST → LLVM IR Module
  5. CodeGen: IR Module → MachineModule
  6. emitAssembly: MachineModule → .asm text

Compiler::compileIR(source):
  1-4. (同上)
  5. IRPrinter: IR Module → .ll text
```

---

## 8. 测试体系

### 8.1 测试覆盖

| 测试套件 | 测试文件 | 覆盖内容 |
|---------|---------|---------|
| Type | `test/IR/test_type.cpp` | Void/Int1/Int8/Int16 类型单例 |
| Value | `test/IR/test_value.cpp` | Use-Def 链、RAUW 操作 |
| Instruction | `test/IR/test_instruction.cpp` | 10 种指令的创建和操作数 |
| BasicBlock | `test/IR/test_bb.cpp` | BB 指令列表、terminator |
| Function | `test/IR/test_function.cpp` | 函数/参数/BB/Module 管理 |
| IRBuilder | `test/IR/test_irbuilder.cpp` | 工厂方法自动插入 |
| Lexer | `test/Lex/test_lexer.cpp` | 37 种 token、注释、位置跟踪 |
| AST | `test/AST/test_ast.cpp` | 全部节点类型创建 |
| Parser | `test/Parse/test_parser.cpp` | 表达式/语句/错误恢复 |
| Sema | `test/Sema/test_sema.cpp` | 合法程序 + 5 类错误检测 |
| IRGen | `test/IRGen/test_irgen.cpp` | 完整 IR 翻译 |
| CodeGen | `test/CodeGen/test_codegen.cpp` | 3 种模式的汇编生成 |
| Integration | `test/Integration/test_e2e.cpp` | 7 种场景端到端 |

**总计：13 套测试，100% 通过。**

### 8.2 运行方式

```bash
cd build
cmake --build .          # 构建
ctest -V                 # 运行全部测试
ctest -R Lexer -V        # 运行单个模块测试
```

---

## 9. 示例程序

### 9.1 冒泡排序（完整算法验证）

```c
fn int main() {
    int arr[20];
    arr[0] = 328; arr[1] = 58; ... arr[19] = 48;

    int i = 0;
    int temp = 0;
    int swapped = 1;

    while (swapped != 0) {
        swapped = 0;
        i = 0;
        while (i < 19) {
            if (arr[i] > arr[i + 1]) {
                temp = arr[i];
                arr[i] = arr[i + 1];
                arr[i + 1] = temp;
                swapped = 1;
            }
            i = i + 1;
        }
    }

    i = 0;
    while (i < 20) {
        print(arr[i]);
        i = i + 1;
    }
    return arr[0];
}
```

输出（emu8086 运行结果）：`13 16 17 45 48 53 58 72 115 126 141 217 280 303 328 347 378 380 380 457`

### 9.2 其他示例

| 文件 | 说明 |
|------|------|
| `basic.ll1` | if/else 分支 |
| `loop.ll1` | while 循环累加 |
| `io_test.ll1` | print + read 输入输出 |
| `array_test.ll1` | 数组声明、赋值、读取 |

---

## 10. 设计启发与参考

| 参考源 | 对应模块 | 借鉴内容 |
|--------|---------|---------|
| LLVM IR | lib/IR | Value→User→Instruction 体系、RAUW、IRBuilder |
| Clang | lib/Parse, lib/Sema | 递归下降解析、Decl/Stmt/Expr 体系、两遍语义分析 |
| Kaleidoscope | 整体架构 | Lexer→Parser→AST→IRGen→JIT 管线 |
| LLVM PassBuilder | lib/Opt (预留) | Pass 管线注册与执行 |
| X86 Target | lib/CodeGen | TargetMachine 抽象、ISel 模式 |
| emu8086 | 目标输出 | proc/endp/寻址模式 |

---

## 11. 总结与展望

### 11.1 已实现功能

- ✅ 完整的 LL(1) 语言前端（Lexer + Parser + AST + Sema）
- ✅ LLVM 风格 SSA 中间表示框架（Value/User/Instruction/BB/Function/Module/IRBuilder）
- ✅ AST → IR 代码生成
- ✅ IR → 8086 汇编代码生成（`[BX+SI]` 数组寻址）
- ✅ LLVM IR 文本输出（`.ll` 格式）
- ✅ print / read I/O 运行时
- ✅ 整数数组支持
- ✅ 嵌套 while / if 控制流
- ✅ 完整回归测试（13 套）

### 11.2 后续扩展方向

- [ ] 优化 Pass 管线（Mem2Reg / InstCombine / SimplifyCFG / GVN / DCE）
- [ ] 寄存器分配（线性扫描）
- [ ] for / break / continue 的 IRGen 和 CodeGen
- [ ] 函数调用约定的完整实现
- [ ] char / bool 类型的完整 8086 CodeGen
- [ ] RISC-V 后端扩展
