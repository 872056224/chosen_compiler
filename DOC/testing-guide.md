# LL1 Compiler — 测试指南

## 环境要求

- Windows + MinGW (gcc 14.2.0)
- CMake 3.16+
- Git Bash 或 PowerShell

## 构建

```bash
cd D:/my_project/compiler/LL1-emu8086/build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

构建产物：
```
build/
├── tools/ll1c/ll1c.exe          ← 编译器
└── test/
    ├── test_type.exe             ← 11 个单元测试
    ├── test_value.exe
    ├── test_instruction.exe
    ├── test_bb.exe
    ├── test_function.exe
    ├── test_irbuilder.exe
    ├── test_lexer.exe
    ├── test_ast.exe
    ├── test_parser.exe
    ├── test_sema.exe
    ├── test_irgen.exe
    ├── test_codegen.exe
    └── test_e2e.exe              ← 端到端集成测试
```

---

## 运行所有测试（推荐）

```bash
cd D:/my_project/compiler/LL1-emu8086/build
ctest -V
```

输出示例：
```
test 1
    Start 1: Type
1/13 Test #1: Type .............................   Passed    0.01 sec
test 2
    Start 2: Value
2/13 Test #2: Value ............................   Passed    0.01 sec
...
test 13
    Start 13: Integration
13/13 Test #13: Integration ......................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 13
Total Test time (real) =   0.16 sec
```

## 运行单个测试

```bash
# 只运行 Lexer 测试
ctest -R Lexer -V

# 只运行 CodeGen 测试
ctest -R CodeGen -V

# 只运行 IR 相关测试
ctest -R "(Type|Value|Instruction|BasicBlock|Function|IRBuilder)" -V
```

---

## 13 个测试逐项说明

### 1. Type — IR 类型系统
```bash
ctest -R Type -V
```
**测什么**：`Type` 类的 `Void`/`Int1`/`Int8`/`Int16` 单例模式，`getKind()` 正确性

### 2. Value — IR 值基类
```bash
ctest -R "^Value$" -V
```
**测什么**：`Value` 的命名、类型、Use-Def 链，`replaceAllUsesWith()` (RAUW)

### 3. Instruction — IR 指令
```bash
ctest -R "^Instruction$" -V
```
**测什么**：`BinaryOpInst`/`RetInst`/`AllocaInst`/`StoreInst`/`LoadInst`/`ICmpInst` 的创建和操作数

### 4. BasicBlock — IR 基本块
```bash
ctest -R "^BasicBlock$" -V
```
**测什么**：`BasicBlock` 的指令列表管理、terminator 检测

### 5. Function — IR 函数 & Module
```bash
ctest -R "^Function$" -V
```
**测什么**：`Function` 的参数/BB 管理，`Module` 的函数查找，`entry block`

### 6. IRBuilder — IR 构建器
```bash
ctest -R IRBuilder -V
```
**测什么**：`IRBuilder` 的全部工厂方法（alloca/store/load/add/icmp/br/ret），自动插入到当前 BB

### 7. Lexer — 词法分析器
```bash
ctest -R Lexer -V
```
**测什么**：
- 14 个关键字识别
- 标识符 (字母/数字/下划线)
- 数字字面量
- 14 个运算符 (含多字符 `<=` `>=` `==` `!=` `&&` `||`)
- 分隔符 (`(` `)` `{` `}` `;` `,`)
- `//` 行注释跳过
- 源码行号/列号跟踪

### 8. AST — 抽象语法树
```bash
ctest -R "^AST$" -V
```
**测什么**：所有 AST 节点类型的创建：`VarDecl`/`FuncDecl`/`Block`/`IfStmt`/`WhileStmt`/`ReturnStmt`/`BinaryExpr`/`VarExpr`/`IntegerLiteral`

### 9. Parser — 递归下降解析器
```bash
ctest -R "^Parser$" -V
```
**测什么**：
- 变量声明（无/有初始值）
- 表达式（20+ 种模式：加减乘除、比较、逻辑、赋值链）
- 运算符优先级（`1+2*3` vs `(1+2)*3`）
- 一元运算（`-x`, `!flag`）
- 空程序处理
- **错误检测**：缺失标识符、缺失括号、顶层裸表达式

### 10. Sema — 语义分析
```bash
ctest -R "^Sema$" -V
```
**测什么**：
- **合法程序**：全局变量、函数、局部变量、if/while 控制流
- **错误检测**：重复定义、未声明变量、return 类型不匹配、非 bool 条件

### 11. IRGen — IR 代码生成
```bash
ctest -R "^IRGen$" -V
```
**测什么**：AST → LLVM IR 完整翻译
- 变量声明/赋值/返回
- 算术表达式（add/sub/mul/div）
- 比较 + if/else
- while 循环
- void 函数、多函数、bool 变量
- 复合表达式

### 12. CodeGen — 8086 代码生成
```bash
ctest -R "^CodeGen$" -V
```
**测什么**：LLVM IR → 8086 汇编
- `return 42` → `mov ax, 42` + `ret`
- 变量 `add` → `mov` + `add`
- if/else → `cmp` + `jl` + `jmp`
- 汇编输出包含 `main proc`/`endp`/`end`

### 13. Integration — 端到端集成
```bash
ctest -R Integration -V
```
**测什么**：全管线 `源码 → .asm文件`
1. 简单表达式 `a + b`
2. if/else 分支
3. while 循环
4. void 函数
5. 多 return 路径
6. 从文件读入源码
7. 写入 .asm 输出文件

---

## 编译器 CLI 使用

```bash
cd D:/my_project/compiler/LL1-emu8086/build

# 编译单个文件
./tools/ll1c/ll1c.exe ../examples/basic.ll1 -o basic.asm

# 查看汇编输出
cat basic.asm
```

### 示例 1：if/else 分支
输入 (`examples/basic.ll1`)：
```c
fn int main() {
    int a = 10;
    int b = 20;
    if (a < b) {
        return b;
    }
    return a;
}
```

输出 (`basic.asm`)：
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

### 示例 2：while 循环
输入 (`examples/loop.ll1`)：
```c
fn int main() {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

输出 (`loop.asm`)：
```asm
.code
main proc
    push bp
    mov  bp, sp
    sub  sp, 4
    mov  ax, 0
    mov  [bp-2], ax
    mov  bx, 0
    mov  [bp-4], bx
    jmp  .main_while_cond
.main_while_cond:
    mov  cx, [bp-2]
    cmp  cx, 10
    jl   .main_while_body
    jmp  .main_while_exit
.main_while_body:
    mov  ax, [bp-4]
    add  ax, [bp-2]
    mov  [bp-4], ax
    mov  bx, [bp-2]
    add  bx, 1
    mov  [bp-2], bx
    jmp  .main_while_cond
.main_while_exit:
    mov  ax, [bp-4]
    ret
    mov  sp, bp
    pop  bp
main endp
end
```

---

## 测试覆盖一览

| 模块 | 测试文件 | 通过 |
|------|---------|------|
| Type | `test/IR/test_type.cpp` | ✅ |
| Value/User | `test/IR/test_value.cpp` | ✅ |
| Instruction | `test/IR/test_instruction.cpp` | ✅ |
| BasicBlock | `test/IR/test_bb.cpp` | ✅ |
| Function/Module | `test/IR/test_function.cpp` | ✅ |
| IRBuilder | `test/IR/test_irbuilder.cpp` | ✅ |
| Lexer | `test/Lex/test_lexer.cpp` | ✅ |
| AST | `test/AST/test_ast.cpp` | ✅ |
| Parser | `test/Parse/test_parser.cpp` | ✅ |
| Sema | `test/Sema/test_sema.cpp` | ✅ |
| IRGen | `test/IRGen/test_irgen.cpp` | ✅ |
| CodeGen | `test/CodeGen/test_codegen.cpp` | ✅ |
| Integration | `test/Integration/test_e2e.cpp` | ✅ |

**总计：13/13 通过**

---

## 手动验证 8086 输出（在 emu8086 中）

1. 编译 `.ll1` 源文件：
```bash
./tools/ll1c/ll1c.exe ../examples/basic.ll1 -o basic.asm
```

2. 在 emu8086 中打开 `basic.asm`

3. 运行 → 查看 AX 寄存器的最终值
   - `basic.asm` 返回 `20` (b > a，走 then 分支)
   - `loop.asm` 返回 `45` (0+1+...+9=45)
