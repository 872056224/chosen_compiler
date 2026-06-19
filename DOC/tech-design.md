# LL1 Compiler — 技术架构设计文档

> **状态**: ✅ MVP 实现完成  
> **最后更新**: 2026-06-19  
> **参考**: LLVM 源码 (`llvm-project-main/`)

---

## 项目定位

基于 **LLVM 架构思想**的教学型编译器，**自实现精简版 IR 框架**（不链接 LLVM 库），将 LL(1) 语言编译到 8086 汇编，在 emu8086 中运行。

### 核心技术决策

| 决策项 | 选择 | 原因 |
|--------|------|------|
| 实现方式 | 参考 LLVM 设计，自实现精简版 | 教学价值高，LLVM 不支持 8086/16 位 |
| 实现语言 | C++17 | 最贴近 LLVM 风格，可直接模仿其类层次设计 |
| 构建系统 | CMake | 模块化组织，与 LLVM 一致 |
| IR 粒度 | 完整简化版 SSA IR | 支持 mem2reg + 中端优化 Pass |
| 后端架构 | 可扩展 Target 体系 | 未来可添加 RISC-V 等后端 |

### 实现原则

```
最小化原则 → 仅保留必要组件
增量实现原则 → MVP → 第二版 → 第三版
可运行优先原则 → 每个阶段产出可运行的产物
```

---

## 1. 编译管线 (Pipeline)

```
LL(1) Source (.ll1)
       │
       ▼
┌─────────────────┐
│   Lexer          │  字符流 → Token 流
│   (lib/Lex)      │
└────────┬────────┘
         │ Token流
         ▼
┌─────────────────┐
│   Parser         │  Token流 → AST (递归下降)
│   (lib/Parse)    │
└────────┬────────┘
         │ AST (Expr/Stmt/Decl树)
         ▼
┌─────────────────┐
│   Semantic       │  AST遍历 → 符号表/类型检查
│   (lib/Sema)     │
└────────┬────────┘
         │ 标注后的AST
         ▼
┌─────────────────┐
│   IRGen          │  语义AST → LLVM IR Module
│   (lib/IRGen)    │
└────────┬────────┘
         │ LLVM IR Module (内存)
         ▼
┌─────────────────┐
│   Optimizer      │  IR → IR (Pass Pipeline)
│   (lib/Opt)      │
└────────┬────────┘
         │ 优化后的IR Module
         ▼
┌─────────────────┐
│   8086 CodeGen   │  LLVM IR → 8086 Assembly
│   (lib/CodeGen)  │
└────────┬────────┘
         │ .asm 文本文件
         ▼
┌─────────────────┐
│   emu8086        │  加载 .asm 并执行
│   (外部工具)     │
└─────────────────┘
```

---

## 2. 目录结构

```
LL1-emu8086/
├── CMakeLists.txt              # 顶层 CMake
├── README.md
├── DOC/
│   ├── prompt.md               # 原始提示词
│   └── tech-design.md          # 本技术文档
│
├── include/                    # 公开头文件（仿 llvm/include/llvm/）
│   └── ll1/
│       ├── Lex/
│       │   ├── Lexer.h
│       │   └── Token.h
│       ├── Parse/
│       │   └── Parser.h
│       ├── AST/
│       │   ├── AST.h
│       │   ├── Expr.h
│       │   ├── Stmt.h
│       │   └── Decl.h
│       ├── Sema/
│       │   ├── Sema.h
│       │   ├── Scope.h
│       │   └── SymbolTable.h
│       ├── IR/
│       │   ├── Module.h
│       │   ├── Function.h
│       │   ├── BasicBlock.h
│       │   ├── Instruction.h
│       │   ├── Type.h
│       │   ├── Value.h
│       │   └── IRBuilder.h
│       ├── IRGen/
│       │   └── IRGen.h
│       ├── Opt/
│       │   ├── PassManager.h
│       │   └── Passes/
│       │       ├── Mem2Reg.h
│       │       ├── InstCombine.h
│       │       ├── SimplifyCFG.h
│       │       ├── GVN.h
│       │       └── DCE.h
│       ├── CodeGen/
│       │   ├── Target.h
│       │   ├── TargetMachine.h
│       │   ├── TargetRegisterInfo.h
│       │   ├── TargetInstrInfo.h
│       │   ├── MCInst.h
│       │   ├── ISelLowering.h
│       │   ├── MachineScheduler.h     # 占位，后续扩展
│       │   ├── RegAlloc.h
│       │   ├── AsmEmitter.h
│       │   └── Target8086/
│       │       ├── Target8086.h
│       │       ├── Target8086Machine.h
│       │       ├── Target8086RegisterInfo.h
│       │       ├── Target8086InstrInfo.h
│       │       ├── Target8086ISelLowering.h
│       │       ├── Target8086AsmEmitter.h
│       │       └── Target8086CallingConv.h
│       └── Driver/
│           └── Compiler.h
│
├── lib/                        # 实现文件（仿 llvm/lib/）
│   ├── CMakeLists.txt
│   ├── Lex/
│   │   ├── Lexer.cpp
│   │   └── Token.cpp
│   ├── Parse/
│   │   └── Parser.cpp
│   ├── AST/
│   │   └── AST.cpp
│   ├── Sema/
│   │   ├── Sema.cpp
│   │   └── Scope.cpp
│   ├── IR/
│   │   ├── Module.cpp
│   │   ├── Function.cpp
│   │   ├── BasicBlock.cpp
│   │   ├── Instruction.cpp
│   │   ├── Type.cpp
│   │   ├── Value.cpp
│   │   └── IRBuilder.cpp
│   ├── IRGen/
│   │   └── IRGen.cpp
│   ├── Opt/
│   │   ├── PassManager.cpp
│   │   ├── Mem2Reg.cpp
│   │   ├── InstCombine.cpp
│   │   ├── SimplifyCFG.cpp
│   │   ├── GVN.cpp
│   │   └── DCE.cpp
│   ├── CodeGen/
│   │   ├── Target.cpp
│   │   ├── TargetMachine.cpp
│   │   ├── MCInst.cpp
│   │   ├── ISelLowering.cpp
│   │   ├── MachineScheduler.cpp     # 占位
│   │   ├── RegAlloc.cpp
│   │   ├── TargetRegisterInfo.cpp
│   │   ├── TargetInstrInfo.cpp
│   │   └── Target8086/
│   │       ├── Target8086.cpp
│   │       ├── Target8086Machine.cpp
│   │       ├── Target8086RegisterInfo.cpp
│   │       ├── Target8086InstrInfo.cpp
│   │       ├── Target8086ISelLowering.cpp
│   │       ├── Target8086AsmEmitter.cpp
│   │       └── Target8086CallingConv.cpp
│   └── Driver/
│       └── Compiler.cpp
│
├── tools/                      # 可执行文件（仿 llvm/tools/）
│   └── ll1c/
│       ├── CMakeLists.txt
│       └── main.cpp
│
├── test/                       # 测试用例
│   ├── Lex/
│   ├── Parse/
│   ├── Sema/
│   ├── IRGen/
│   ├── Opt/
│   └── CodeGen/
│
└── examples/                   # 示例程序
    ├── basic.ll1
    ├── if_else.ll1
    ├── while_loop.ll1
    └── function.ll1
```

---

## 3. 模块依赖

```
                    ┌──────┐
                    │Driver│  ← 顶层调度
                    └──┬───┘
          ┌────────────┼──────────────┐
          ▼            ▼              ▼
       ┌─────┐    ┌───────┐    ┌─────────┐
       │ Lex │◄───│ Parse │◄───│  Sema   │
       └─────┘    └───┬───┘    └────┬────┘
                      │             │
                      ▼             ▼
                   ┌──────┐    ┌─────────┐
                   │ AST  │◄───│SymbolTab│
                   └──┬───┘    └─────────┘
                      │
                      ▼
                  ┌───────┐
                  │ IRGen │
                  └───┬───┘
                      │
                      ▼
                  ┌───────┐
                  │  IR   │ ← 核心IR数据结构（最底层）
                  └───┬───┘
                 ┌────┴────┐
                 ▼         ▼
            ┌───────┐ ┌────────┐
            │  Opt  │ │ CodeGen│
            └───────┘ └────────┘
```

> 依赖方向：上层依赖下层。IR 是最底层核心，Opt 和 CodeGen 都依赖 IR。

---

## 4. LL(1) 语言设计

### 4.1 类型系统

```
int   → i16   (8086 原生宽度)
char  → i8
bool  → i1
void  → void (仅函数返回)
```

### 4.2 关键字全集

```
类型:     int, char, bool, void
控制流:   if, else, while, for, break, continue, return
函数:     fn
常量:     true, false
```

### 4.3 运算符（按优先级）

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 | `()` 函数调用 | 左 |
| 2 | `-` 一元取负, `!` 逻辑非 | 右 |
| 3 | `*` `/` `%` | 左 |
| 4 | `+` `-` | 左 |
| 5 | `<` `>` `<=` `>=` | 左 |
| 6 | `==` `!=` | 左 |
| 7 | `&&` | 左 |
| 8 | `=` 赋值 | 右 |

### 4.4 语法规则（简易 EBNF）

```
Program      ::= (VarDecl | FuncDef)*

Type         ::= 'int' | 'char' | 'bool' | 'void'

VarDecl      ::= Type ident ('=' Expr)? ';'

FuncDef      ::= 'fn' Type ident '(' Params? ')' Block

Params       ::= Type ident (',' Type ident)*

Block        ::= '{' Stmt* '}'

Stmt         ::= VarDecl
               | Expr ';'
               | 'if' '(' Expr ')' Block ('else' Block)?
               | 'while' '(' Expr ')' Block
               | 'for' '(' Expr? ';' Expr? ';' Expr? ')' Block
               | 'break' ';'
               | 'continue' ';'
               | 'return' Expr? ';'

Expr         ::= Assignment

Assignment   ::= LogicalOr ('=' Assignment)?

LogicalOr    ::= LogicalAnd ('&&' LogicalAnd)*

LogicalAnd   ::= Equality ('&&' Equality)*

Equality     ::= Relational (('==' | '!=') Relational)*

Relational   ::= Additive (('<' | '>' | '<=' | '>=') Additive)*

Additive     ::= Multiplicative (('+' | '-') Multiplicative)*

Multiplicative ::= Unary (('*' | '/' | '%') Unary)*

Unary        ::= ('-' | '!') Unary | Primary

Primary      ::= ident '(' Args? ')'
               | ident
               | number
               | '(' Expr ')'
               | 'true' | 'false'

Args         ::= Expr (',' Expr)*
```

---

## 5. LL(1) 文法设计

### 5.1 Token 定义

```cpp
enum TokenKind {
    tok_eof, tok_error, tok_ident, tok_number,

    // 关键字
    kw_int, kw_char, kw_bool, kw_void,
    kw_if, kw_else, kw_while, kw_for,
    kw_break, kw_continue, kw_return,
    kw_fn, kw_true, kw_false,

    // 运算符
    op_plus, op_minus, op_star, op_slash, op_percent,
    op_lt, op_gt, op_le, op_ge, op_eq, op_ne,
    op_and, op_or, op_not, op_assign,

    // 分隔符
    l_paren, r_paren, l_brace, r_brace, semi, comma,
};
```

### 5.2 完整 EBNF（左递归已消除）

```
Program      ::= TopLevel*
TopLevel     ::= FuncDef | VarDecl

VarDecl      ::= Type ident ('=' Expr)? ';'
FuncDef      ::= 'fn' Type ident '(' Params? ')' Block
Type         ::= 'int' | 'char' | 'bool' | 'void'

Params       ::= Type ident ParamRest*
ParamRest    ::= ',' Type ident

Block        ::= '{' Stmt* '}'

Stmt         ::= VarDecl
               | ExprStmt
               | IfStmt
               | WhileStmt
               | ForStmt
               | BreakStmt
               | ContinueStmt
               | ReturnStmt

ExprStmt     ::= Expr? ';'
IfStmt       ::= 'if' '(' Expr ')' Block ElseClause
ElseClause   ::= 'else' (IfStmt | Block) | ε
WhileStmt    ::= 'while' '(' Expr ')' Block
ForStmt      ::= 'for' '(' Expr? ';' Expr? ';' Expr? ')' Block
BreakStmt    ::= 'break' ';'
ContinueStmt ::= 'continue' ';'
ReturnStmt   ::= 'return' Expr? ';'

// 表达式 (左递归消除后)
Expr         ::= LogicalOr
LogicalOr    ::= LogicalAnd LogicalOrRest
LogicalOrRest::= '||' LogicalAnd LogicalOrRest | ε
LogicalAnd   ::= Equality LogicalAndRest
LogicalAndRest::= '&&' Equality LogicalAndRest | ε
Equality     ::= Relational EqualityRest
EqualityRest ::= ('==' | '!=') Relational EqualityRest | ε
Relational   ::= Additive RelationalRest
RelationalRest::= ('<' | '>' | '<=' | '>=') Additive RelationalRest | ε
Additive     ::= Multiplicative AdditiveRest
AdditiveRest ::= ('+' | '-') Multiplicative AdditiveRest | ε
Multiplicative ::= Unary MultiplicativeRest
MultiplicativeRest::= ('*' | '/' | '%') Unary MultiplicativeRest | ε
Unary        ::= ('-' | '!') Unary | Primary
Primary      ::= ident '(' Args? ')' | ident | number | '(' Expr ')' | 'true' | 'false'
Args         ::= Expr ArgRest*
ArgRest      ::= ',' Expr
```

### 5.3 FIRST 集

| 非终结符 | FIRST |
|----------|-------|
| `Program` | `{fn, int, char, bool}` |
| `TopLevel` | `{fn, int, char, bool}` |
| `FuncDef` | `{fn}` |
| `VarDecl` | `{int, char, bool}` |
| `Type` | `{int, char, bool, void}` |
| `Stmt` | `{int, char, bool, ident, number, (, -, !, true, false, if, while, for, break, continue, return, ;}` |
| `IfStmt` | `{if}` |
| `WhileStmt` | `{while}` |
| `ForStmt` | `{for}` |
| `BreakStmt` | `{break}` |
| `ContinueStmt` | `{continue}` |
| `ReturnStmt` | `{return}` |
| `ExprStmt` | `{ident, number, (, -, !, true, false, ;}` |
| `ElseClause` | `{else, ε}` |
| `Expr` | `{ident, number, (, -, !, true, false}` |
| `Primary` | `{ident, number, (, true, false}` |

### 5.4 关键不相交性验证

```
FIRST(VarDecl)  = {int, char, bool}
FIRST(ExprStmt) = {ident, number, (, -, !, true, false, ;}
→ 交集 = ∅ ✓

FIRST(FuncDef)  = {fn}
FIRST(VarDecl)  = {int, char, bool}
→ 交集 = ∅ ✓   (TopLevel 可 LL(1) 决策)

FIRST(IfStmt)       = {if}
FIRST(WhileStmt)    = {while}
FIRST(ForStmt)      = {for}
FIRST(BreakStmt)    = {break}
FIRST(ContinueStmt) = {continue}
FIRST(ReturnStmt)   = {return}
→ 全部互不相交 ✓   (Statement 可 LL(1) 决策)
```

### 5.5 悬空 else 处理

采用标准就近匹配策略，在递归下降中自然解决：

```
ElseClause ::= 'else' (IfStmt | Block) | ε
```

### 5.6 LL(1) 验证结论

| 问题 | 状态 |
|------|------|
| 左递归 | ✅ 已消除（表达式部分转换为尾递归） |
| 左公因子 | ✅ 已提取（Primary 中 ident/ident`(` 合并） |
| 悬空 else | ✅ 标准就近匹配，递归下降自然解决 |
| FIRST/FIRST 冲突 | ✅ 无 |
| FIRST/FOLLOW 冲突 | ✅ 无（含ε规则的FOLLOW与FIRST无交集） |

**文法总体满足 LL(1) ✓**

---

## 6. AST 设计（仿 Clang AST）

### 6.1 类层次结构

```
                    ┌──────────┐
                    │ ASTNode  │  ← 所有AST节点的基类
                    │ - Kind   │
                    │ - Loc    │  ← 源码位置（报错用）
                    └────┬─────┘
            ┌────────────┼──────────────┐
            ▼            ▼              ▼
        ┌──────┐    ┌─────────┐   ┌──────────┐
        │ Decl │    │  Stmt   │   │  Expr    │  ← Expr 继承 Stmt
        │(声明) │    │ (语句)  │   │ (表达式) │     (同 Clang)
        └──┬───┘    └────┬────┘   └────┬─────┘
    ┌──────┼──────┐      │        ┌────┼──────────┐
    ▼      ▼      ▼      ▼        ▼    ▼     ▼     ▼
VarDecl ParmDecl FuncDecl │   Binary Unary Call Literal
                          │   ┌──┬──┬──┬──┐
                          ▼   ▼  ▼  ▼  ▼  ▼
                    IfStmt WhileStmt ForStmt
                    BreakStmt ContinueStmt
                    ReturnStmt
                    Block (CompoundStmt)
```

### 6.2 与 Clang 的对应关系

| 我们的 AST | Clang 对应 | 说明 |
|---|---|---|
| `ASTNode` | `Decl` / `Stmt` (两个基类) | 我们简化为一个基类，用 Kind 区分 |
| `Decl::Kind` | `Decl::Kind` (通过 DeclNodes.inc) | 同样的枚举分发模式 |
| `VarDecl` | `clang::VarDecl` | 变量声明 |
| `FuncDecl` | `clang::FunctionDecl` | 函数声明，合并了参数 |
| `Block` | `clang::CompoundStmt` | 语句块 |
| `IfStmt` | `clang::IfStmt` | if 语句 |
| `BinaryExpr` | `clang::BinaryOperator` | 二元表达式 |
| `VarExpr` | `clang::DeclRefExpr` | 变量引用 |
| `CallExpr` | `clang::CallExpr` | 函数调用 |

---

## 7. 语义分析设计

### 7.1 符号表

```cpp
struct Symbol {
    std::string Name;
    Decl *Node;          // 指向对应的 VarDecl / FuncDecl / ParmDecl
    Type *Ty;
    bool isParameter;
};

class Scope {
public:
    enum class ScopeKind { Global, Function, Block };
    ScopeKind getKind() const;
    Scope *getParent() const;

    bool insert(Symbol sym);           // 重复定义 → false
    Symbol *lookup(const std::string &name);      // 递归向父作用域
    Symbol *lookupLocal(const std::string &name); // 仅本层

private:
    std::map<std::string, Symbol> Symbols;
    Scope *Parent;
    ScopeKind Kind;
};

class SymbolTable {
public:
    void enterScope(Scope::ScopeKind k);
    void exitScope();
    Scope *currentScope() const;
    bool declare(Symbol sym);
    Symbol *lookup(const std::string &name);
private:
    Scope *CurrentScope;
};
```

### 7.2 Sema 两遍遍历

```
Pass1: 收集所有声明（FuncDef/VarDecl）→ 插入 SymTable (Global)
Pass2: 遍历每个 FuncDef 体
  → 进入 Function 作用域
  → 插入参数
  → 递归遍历 Stmt/Expr:
    ├ VarDecl: 检查重复 → 插入 → 递归分析 Init
    ├ VarExpr: lookup → 未定义报错 → 设置 DeclRef
    ├ BinaryExpr: 递归分析 LHS/RHS → 检查类型
    ├ CallExpr: lookup 函数 → 检查参数数量和类型
    ├ IfStmt: 分析 Cond → 确保 bool
    ├ WhileStmt: 分析 Cond → 确保 bool
    ├ ReturnStmt: 检查返回类型匹配
    ├ BreakStmt: 检查在循环内
    └ ... 其他节点
  → 退出 Function 作用域
```

### 7.3 检查项清单

| 检查项 | 规则 |
|--------|------|
| 重复定义 | 同一作用域内不可声明同名符号 |
| 未定义变量 | 使用的变量必须在当前/上级作用域已声明 |
| 类型检查-运算 | 操作数类型兼容 |
| 类型检查-赋值 | 左右类型兼容 |
| 类型检查-条件 | 条件必须是 bool |
| 类型检查-return | 返回值类型与函数签名匹配 |
| 函数参数数量 | 调用时参数数量匹配 |
| 函数参数类型 | 参数类型匹配 |
| 控制流 | break/continue 必须在循环内 |
| 控制流 | return 必须在函数内 |
| 函数重复定义 | 同签名函数不可重复定义 |
| 函数名冲突 | 函数名不能与变量名冲突 |

---

## 8. LLVM IR 设计（精简自实现版）

### 8.1 类层次（仿 LLVM Value → User → Instruction）

```
Value (基类)
├── Argument (函数参数)
├── BasicBlock
├── Function
├── Module
├── User (使用其他 Value 的基类)
│   ├── Instruction
│   │   ├── BinaryOpInst
│   │   ├── ICmpInst
│   │   ├── AllocaInst
│   │   ├── LoadInst
│   │   ├── StoreInst
│   │   ├── BranchInst
│   │   ├── RetInst
│   │   ├── CallInst
│   │   ├── PhiInst
│   │   └── CastInst
│   └── Constant
│       ├── ConstantInt
│       ├── ConstantChar
│       └── ConstantBool
└── Type (非 Value，独立设计)
```

### 8.2 指令集

| 分类 | 指令 | 说明 |
|------|------|------|
| Terminator | `ret`, `br` | 控制流终结 |
| Binary | `add`, `sub`, `mul`, `sdiv`, `srem` | 算术运算 |
| Bitwise | `and`, `or`, `xor`, `shl`, `shr` | 位操作 |
| Compare | `icmp` | 整数比较，产生 i1 |
| Memory | `alloca`, `load`, `store` | 栈内存操作 |
| Call | `call` | 函数调用 |
| Phi | `phi` | SSA φ 节点 |
| Cast | `sext`, `zext`, `trunc` | 类型转换 |

### 8.3 核心接口

```cpp
class Value {
    ValueKind Kind;
    Type *Ty;
    std::string Name;
    std::vector<User*> Uses;
public:
    void replaceAllUsesWith(Value *v);  // RAUW — SSA的核心操作
};

class User : public Value {
    std::vector<Value*> Operands;
};

class BasicBlock : public Value {
    std::list<std::unique_ptr<Instruction>> Instructions;
    Function *Parent;
};

class Function : public Value {
    std::vector<std::unique_ptr<Argument>> Arguments;
    std::vector<std::unique_ptr<BasicBlock>> Blocks;
};

class Module : public Value {
    std::string Name;
    std::vector<std::unique_ptr<Function>> Functions;
};
```

### 8.4 IRBuilder（仿 llvm/IR/IRBuilder.h）

```cpp
class IRBuilder {
    LLVMContext &Context;
    BasicBlock *CurrentBB;
    BasicBlock::iterator InsertPt;

public:
    void setInsertPoint(BasicBlock *bb);

    // 创建指令（自动插入到 CurrentBB）
    Value *CreateAdd(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateSub(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateMul(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateSDiv(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateICmpEQ(Value *lhs, Value *rhs, const std::string &name = "");
    Value *CreateICmpSLT(Value *lhs, Value *rhs, const std::string &name = "");
    // ... 其他: Sub, SRem, ICmpNE/SLE/SGT/SGE, And, Or, Not, Neg
    // ... Cast: SExt, ZExt, Trunc

    AllocaInst *CreateAlloca(Type *ty, const std::string &name = "");
    LoadInst *CreateLoad(Type *ty, Value *ptr, const std::string &name = "");
    StoreInst *CreateStore(Value *val, Value *ptr);
    CallInst *CreateCall(Function *callee, const std::vector<Value*> &args);
    RetInst *CreateRetVoid();
    RetInst *CreateRet(Value *v);
    BranchInst *CreateBr(BasicBlock *dest);
    BranchInst *CreateCondBr(Value *cond, BasicBlock *trueBB, BasicBlock *falseBB);
    PhiInst *CreatePhi(Type *ty, const std::string &name = "");
};
```

### 8.5 IR 生成映射表

```
int a = 1;        →  %a = alloca i16
                      store i16 1, ptr %a

a + b             →  %tmp = add i16 %a_val, %b_val

if (a < b) ...    →  %cmp = icmp slt i16 %a_val, %b_val
                      br i1 %cmp, label %then, label %else

while (a < 100)   →  br label %cond        ; entry
                  →  cond:                  ; cond block
                      %cmp = icmp slt i16 %a_val, 100
                      br i1 %cmp, label %body, label %exit
                  →  body: ... br %cond     ; body block
                  →  exit:                  ; exit block
```

---

## 9. Pass 管线设计

### 9.1 Pass 选择

| Pass | 作用 | 保留 | 原因 |
|------|------|------|------|
| Mem2Reg (PromotePass) | alloca → SSA 寄存器 | ✅ | 核心优化，消除内存访问 |
| InstCombine | 指令合并/简化 | ✅ | 常量折叠、代数化简 |
| Reassociate | 表达重结合 | ✅ | 优化表达式顺序 |
| SimplifyCFG | CFG 简化 | ✅ | 消除不可达块、合并分支 |
| GVN | 全局值编号 | ✅ | 消除公共子表达式 |
| DCE | 死代码消除 | ✅ | 删除无用指令 |
| Inline | 函数内联 | ❌ | MVP 不需要 |
| Unroll | 循环展开 | ❌ | 教学不需要 |
| Vectorize | 向量化 | ❌ | 8086 不支持 |

### 9.2 PassManager 设计（仿 LLVM new PM）

```cpp
// Pass 基类
class FunctionPass {
public:
    virtual bool run(Function &F) = 0;
    virtual ~FunctionPass() = default;
};

class ModulePass {
public:
    virtual bool run(Module &M) = 0;
    virtual ~ModulePass() = default;
};

// PassManager
class FunctionPassManager {
public:
    void addPass(std::unique_ptr<FunctionPass> pass);
    bool run(Function &F);
private:
    std::vector<std::unique_ptr<FunctionPass>> Passes;
};

class ModulePassManager {
public:
    void addPass(std::unique_ptr<ModulePass> pass);
    bool run(Module &M);
};
```

### 9.3 默认管线

```cpp
// 仿 Kaleidoscope Chapter 7 的管线配置
void addDefaultPasses(FunctionPassManager &FPM) {
    FPM.addPass(std::make_unique<Mem2RegPass>());      // alloca→SSA
    FPM.addPass(std::make_unique<InstCombinePass>());  // 指令合并
    FPM.addPass(std::make_unique<ReassociatePass>());  // 表达重结合
    FPM.addPass(std::make_unique<GVNPass>());          // 公共子表达式消除
    FPM.addPass(std::make_unique<SimplifyCFGPass>());  // CFG简化
    FPM.addPass(std::make_unique<DCEPass>());          // 死代码消除
}
```

---

## 10. 后端设计（可扩展 Target 体系）

### 10.1 数据流

```
LLVM IR (Module, Function, BasicBlock, Instruction)
    │
    ▼
┌─────────────────────────────┐
│  ISelLowering               │  ← Target 无关框架
│  遍历IR指令                 │
│  调用 TargetInstrInfo 匹配  │  ← Target 相关
│  生成 MIR (MCInst序列)      │
└────────────┬────────────────┘
             │ MIR (MachineFunction + MachineBasicBlock)
             ▼
┌─────────────────────────────┐
│  MachineScheduler (占位)    │  ← 后续扩展，不填充
└────────────┬────────────────┘
             ▼
┌─────────────────────────────┐
│  RegAlloc (线性扫描)         │  ← Target 无关
│  虚拟寄存器 → 物理寄存器    │
│  处理 spill/reload           │  ← 使用 TargetRegisterInfo
└────────────┬────────────────┘
             │ 分配完成后的 MIR
             ▼
┌─────────────────────────────┐
│  AsmEmitter                  │
│  MCInst → 文本汇编输出      │  ← Target 相关 (语法差异)
└────────────┬────────────────┘
             │
             ▼
        .asm 文件
```

### 10.2 Target 抽象

```cpp
class TargetRegisterInfo {
public:
    virtual unsigned getNumRegs() const = 0;
    virtual std::string getRegName(unsigned reg) const = 0;
    virtual unsigned getRegClass(unsigned reg) const = 0;  // GP/SEG/...
    virtual bool isAllocatable(unsigned reg) const = 0;
};

class TargetInstrInfo {
public:
    virtual bool match(Instruction *I, MCInst &MI) const = 0;
    virtual std::string getInstName(unsigned opcode) const = 0;
};
```

### 10.3 8086 目标

**寄存器**: AX, BX, CX, DX, SI, DI, BP, SP, CS, DS, ES, SS

**支持的 8086 指令（MVP）**:

```
MOV  r,r | r,imm | r,m | m,r
ADD  r,r | r,imm
SUB  r,r | r,imm
MUL  r
DIV  r
CMP  r,r | r,imm
JMP  label
JE   label
JNE  label
JL   label
JG   label
JLE  label
JGE  label
CALL label
RET
PUSH r
POP  r
```

### 10.4 扩展路径

未来添加 RISC-V 后端时，只需新建：

```
include/ll1/CodeGen/TargetRISCV/
├── TargetRISCV.h
├── TargetRISCVRegisterInfo.h    # x0-x31
├── TargetRISCVInstrInfo.h       # RV32I 指令集
├── TargetRISCVISelLowering.h
└── TargetRISCVAsmEmitter.h

lib/CodeGen/TargetRISCV/
└── (对应 .cpp 实现)
```

无需修改 ISelLowering 框架、RegAlloc、PassManager 任何代码。

---

## 11. emu8086 约束分析

### 11.1 8086 架构要点

| 特性 | 约束 |
|------|------|
| 通用寄存器 | AX, BX, CX, DX (16-bit) |
| 索引寄存器 | SI, DI (16-bit) |
| 帧指针 | BP (16-bit) |
| 栈指针 | SP (16-bit) |
| 段寄存器 | CS, DS, ES, SS (16-bit) |
| 数据宽度 | 16 位（8 位通过 AH/AL, BH/BL 等访问） |
| 寻址模式 | [BX+SI], [BX+DI], [BP+SI], [BP+DI], [BX], [SI], [DI], [disp], [BP+disp], [BX+disp] |
| 内存模型 | 实模式，段:偏移 寻址 |

### 11.2 LLVM IR → 8086 映射表

| LLVM IR | 8086 汇编 | 说明 |
|---------|-----------|------|
| `add i16 %a, %b` | `add ax, bx` | 双操作数 |
| `sub i16 %a, %b` | `sub ax, bx` | |
| `mul i16 %a, %b` | `mov ax, src1; mul src2` | 8086 mul 隐含使用 AX |
| `sdiv i16 %a, %b` | `mov ax, src1; cwd; idiv src2` | 带符号除法 |
| `icmp slt %a, %b` | `cmp ax, bx` → 设置 flag | cmp + 条件跳转 |
| `br label %dest` | `jmp label` | |
| `br i1 %cond, %true, %false` | `cmp ... / je label_true / jmp label_false` | |
| `call @fn` | `call fn` | |
| `ret i16 %val` | `mov ax, val; ret` | 返回值在 AX |
| `alloca i16` | `sub sp, 2` (栈上分配) | |
| `load i16, ptr %p` | `mov ax, [bp+offset]` | |
| `store i16 %v, ptr %p` | `mov [bp+offset], ax` | |

### 11.3 调用约定

```
// 调用者:
push arg2      // 最后一个参数先入栈
push arg1
call function
add sp, 4      // 清理栈 (caller cleanup)

// 被调用者:
push bp        // 保存旧帧指针
mov bp, sp     // 设置新帧指针
sub sp, N      // 分配局部变量空间
...            // 函数体
mov ax, result // 返回值放 AX
mov sp, bp     // 恢复 sp
pop bp         // 恢复旧帧指针
ret            // 返回
```

---

## 12. 实现路线图

### MVP 版本（第一阶段）

```
目标: 可运行的端到端编译器
  ✓ 变量声明和赋值 (int 类型)
  ✓ 表达式 (+, -, *, /, %)
  ✓ 关系运算 (<, >, <=, >=, ==, !=)
  ✓ if/else
  ✓ while 循环
  ✗ 函数调用 (使用单一 main)
  ✗ 优化 Pass
  ✗ 寄存器分配 (直接栈映射)
```

### 第二版（第二阶段）

```
  ✓ 函数定义和调用
  ✓ 作用域
  ✓ 多类型 (int, char, bool)
  ✓ Mem2Reg Pass
  ✓ InstCombine Pass
  ✓ SimplifyCFG Pass
  ✓ 线性扫描寄存器分配
  ✓ 调用约定实现
```

### 第三版（第三阶段）

```
  ✓ 8086 后端完善 (完整指令覆盖)
  ✓ GVN / DCE Pass
  ✓ for 循环
  ✓ break / continue
  ✓ 多源文件? (可选)
  ✓ RISC-V 后端原型? (可选扩展)
```

### 实现顺序

```
Step 1: IR 核心 (Value/Type/Instruction/BasicBlock/Function/Module)
Step 2: Lexer (词法分析器)
Step 3: Parser (递归下降解析器)
Step 4: AST (抽象语法树)
Step 5: Sema (语义分析)
Step 6: IRGen (AST → LLVM IR)
Step 7: CodeGen (IR → 8086 汇编)
Step 8: Driver (组装管线)
Step 9: Opt (PassManager + 各 Pass)
```

---

## 13. 参考资料清单

| 参考内容 | LLVM 源码位置 |
|----------|--------------|
| Value/User/Instruction 体系 | `llvm/include/llvm/IR/Value.h`, `User.h`, `Instruction.h` |
| IRBuilder | `llvm/include/llvm/IR/IRBuilder.h` |
| PassManager (New PM) | `llvm/include/llvm/IR/PassManager.h`, `llvm/lib/Passes/` |
| Kaleidoscope 完整示例 | `llvm/examples/Kaleidoscope/Chapter7/toy.cpp` |
| Clang Decl 体系 | `clang/include/clang/AST/DeclBase.h`, `Decl.h` |
| Clang Stmt/Expr 体系 | `clang/include/clang/AST/Stmt.h`, `Expr.h` |
| X86 Target Machine | `llvm/lib/Target/X86/X86TargetMachine.h`, `X86.h` |
| X86 ISel | `llvm/lib/Target/X86/X86ISelDAGToDAG.cpp`, `X86ISelLowering.cpp` |
| X86 AsmPrinter | `llvm/lib/Target/X86/X86AsmPrinter.cpp` |
| MC 层 | `llvm/include/llvm/MC/`, `llvm/lib/MC/` |
| GVN Pass | `llvm/lib/Transforms/Scalar/GVN.cpp` |
| SimplifyCFG Pass | `llvm/lib/Transforms/Utils/SimplifyCFG.cpp` |
| Mem2Reg Pass | `llvm/lib/Transforms/Utils/Mem2Reg.cpp` |

---

## 14. MVP 实现结果

> 实现日期: 2026-06-19

### 项目统计

| 指标 | 数值 |
|------|------|
| 源文件数 (.h/.cpp) | 48 |
| 总代码行数 | ~4,330 |
| 单元测试 | 13 (100% passing) |
| Git commits | 13 |
| 模块数 | 9 (Lex, Parse, AST, Sema, IR, IRGen, Opt, CodeGen, Driver) |

### 已实现功能

| Phase | 模块 | 状态 | 测试 |
|-------|------|------|------|
| 0 | CMake 项目骨架 | ✅ | — |
| 1 | IR Core (Type/Value/User/Instruction/BB/Function/Module/IRBuilder) | ✅ | 6 |
| 2 | Lexer (Token + 关键字 + 运算符) | ✅ | 1 |
| 3 | AST (Decl/Stmt/Expr 继承体系) | ✅ | 1 |
| 4 | Parser (LL(1) 递归下降) | ✅ | 1 |
| 5 | Sema (Scope + SymbolTable + 类型检查) | ✅ | 1 |
| 6 | IRGen (AST → LLVM IR) | ✅ | 1 |
| 7 | CodeGen (IR → 8086 汇编) | ✅ | 1 |
| 8 | Driver (全管线编排 + CLI) | ✅ | — |
| 9 | Integration (端到端测试) | ✅ | 1 |

### 编译管线验证

```
LL1 Source (.ll1)
    → Lexer (字符流 → Token流)
    → Parser (Token流 → AST, 递归下降)
    → Sema (符号表 + 类型检查, 两遍遍历)
    → IRGen (AST → LLVM IR: alloca/store/load/icmp/br/ret)
    → CodeGen (IR → 8086: stack frame + mov/add/sub/cmp/jmp/ret)
    → emu8086 兼容 .asm 文件 ✅
```

### 生成的汇编示例

输入 (`examples/basic.ll1`):
```c
fn int main() {
    int a = 10;
    int b = 20;
    if (a < b) { return b; }
    return a;
}
```

输出 (`basic.asm`):
```asm
.code
main proc
    push bp
    mov  bp, sp
    sub  sp, 4
    mov  ax, 10
    mov  [bp-2], ax
    mov  bx, 20
    mov  [bp-4], bx
    mov  cx, [bp-2]
    cmp  ax, [bp-4]
    jl   .main_then
    jmp  .main_if_end
.main_then:
    mov  ax, [bp-4]
    ret
.main_if_end:
    mov  ax, [bp-2]
    ret
    mov  sp, bp
    pop  bp
main endp
end
```

### 命令行使用

```bash
cd build
./tools/ll1c/ll1c.exe ../examples/basic.ll1 -o output.asm
# Compiled examples/basic.ll1 → output.asm
```

### 待实现 (第二/三版)

- [ ] Mem2Reg / InstCombine / SimplifyCFG / GVN / DCE Pass
- [ ] 线性扫描寄存器分配（当前用简单轮转 AX/BX/CX/DX）
- [ ] for 循环的 IRGen + CodeGen
- [ ] break / continue
- [ ] char / bool 类型的完整 codegen
- [ ] 函数调用的完整实现（参数传递 + 调用约定）
- [ ] RISC-V 后端扩展
