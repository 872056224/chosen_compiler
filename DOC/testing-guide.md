# LL1 Compiler — 测试指南

> 最后更新: 2026-06-22 | 测试数: 17 | 全部通过

---

## 快速运行

```bash
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest
```

预期输出：
```
100% tests passed, 0 tests failed out of 17
```

---

## 17 个测试逐项说明

### IR 层（6 个）

| # | 测试 | 覆盖内容 |
|---|------|---------|
| 1 | **Type** | Void/Int1/Int8/Int16 单例，getKind() |
| 2 | **Value** | 命名、类型、Use-Def 链，replaceAllUsesWith() (RAUW) |
| 3 | **Instruction** | BinaryOpInst/RetInst/AllocaInst/StoreInst/LoadInst/ICmpInst/PhiInst 创建与操作数 |
| 4 | **BasicBlock** | 指令列表管理、terminator 检测、getPredecessors/getSuccessors |
| 5 | **Function** | 参数/BB 管理、Module 函数查找、entry block |
| 6 | **IRBuilder** | 全部工厂方法 (alloca/store/load/add/icmp/br/ret/phi/call)，自动插入当前 BB |

### 前端（4 个）

| # | 测试 | 覆盖内容 |
|---|------|---------|
| 7 | **Lexer** | 14 关键字、标识符、数字、14 运算符(含多字符)、分隔符、行注释、行列号 |
| 8 | **AST** | VarDecl/FuncDecl/Block/IfStmt/WhileStmt/ForStmt/ReturnStmt/BinaryExpr/VarExpr/IntegerLiteral/BoolLiteral/CallExpr |
| 9 | **Parser** | 变量声明、20+ 表达式模式、运算符优先级、一元运算、错误检测(缺失标识符/括号/顶层裸表达式) |
| 10 | **Sema** | 合法程序(全局/局部变量、函数、if/while 控制流)、错误检测(重复定义/未声明/类型不匹配/非bool条件) |

### 上端（IRGen）（1 个）

| # | 测试 | 覆盖内容 |
|---|------|---------|
| 11 | **IRGen** | AST→LLVM IR: 变量声明/赋值/返回、算术、比较+if/else、while循环、void函数、多函数、bool变量、复合表达式 |

### 旧后端（1 个）

| # | 测试 | 覆盖内容 |
|---|------|---------|
| 12 | **CodeGen** | IR→8086汇编: return 42、变量add、if/else cmp+jl+jmp、框架输出 |

### 新 Machine IR 后端（4 个）⚠️ 本次新增

| # | 测试 | 覆盖内容 |
|---|------|---------|
| 13 | **Register** | Register 类型 (uint32_t)：NoRegister 哨兵、物理/虚拟寄存器判別、index↔vreg 互转、8086 物理编号 (AX=1..DI=8) |
| 14 | **MachineOperand** | 5 种操作数类型 (Reg/Imm/FI/MBB/ES) 的创建与访问器、def/dead/kill 标志操作 |
| 15 | **SelectionDAG** | SDNode/SDValue 基础：EntryToken、Constant、FrameIndex、Binary(ADD)、LOAD/STORE、SETCC(谓词)、RET、SExt、Payload 存取、SDValue 比较 |
| 16 | **NewPipeline** | 新管线端到端集成：5 个子测试(func_call/if-else/for循环/简单return/print)，验证输出含函数标签、push/pop、call、cmp+jcc、jmp、ret |

### 集成（1 个）

| # | 测试 | 覆盖内容 |
|---|------|---------|
| 17 | **Integration** | 全管线 `源码→.asm文件`: 简单表达式、if/else、while循环、void函数、多return路径、文件读入、.asm写出 |

---

## 测试覆盖一览

```
17 tests: IR(6) + Frontend(4) + IRGen(1) + OldBackend(1) + NewBackend(4) + Integration(1)
```

| 模块 | 测试文件 | 状态 |
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
| CodeGen (old) | `test/CodeGen/test_codegen.cpp` | ✅ |
| Register | `test/CodeGen/test_register.cpp` | ✅ ⚠️ new |
| MachineOperand | `test/CodeGen/test_machine_operand.cpp` | ✅ ⚠️ new |
| SelectionDAG | `test/CodeGen/test_selection_dag.cpp` | ✅ ⚠️ new |
| NewPipeline | `test/CodeGen/test_new_pipeline.cpp` | ✅ ⚠️ new |
| Integration | `test/Integration/test_e2e.cpp` | ✅ |

---

## 新旧管线对照

| 特性 | 旧管线 (`--opt`) | 新管线 (`--new-codegen --opt`) |
|------|-----------------|------------------------------|
| 测试覆盖 | Test 12 (CodeGen) | Test 13-16 (Register, MO, SDAG, NewPipeline) |
| 中间表示 | 直接 emit 文本 | MachineInstr + MachineOperand |
| 寄存器 | 字符串 "ax"/"cx"/"dx" | Register (uint32_t, phys/vreg) |
| 指令选择 | switch-case 一对一映射 | SelectionDAG → ISel 模式匹配 |
| Phi 处理 | PhiElimination (alloca+store+load) | SDBuilder COPY 降低 |
| 寄存器分配 | 线性扫描 (string) | 线性扫描 (Register) + spill/reload 插入 |
| 输出方式 | 生成过程中 `emit()` | 后置 Printer (MachineInstr→text) |
| 目标抽象 | 无 | TargetMachine / TRI / TII / TLI |

---

## 运行单个测试

```bash
# 新 Machine IR 测试
ctest -R Register -V
ctest -R MachineOperand -V
ctest -R SelectionDAG -V
ctest -R NewPipeline -V

# 只运行旧 CodeGen 测试
ctest -R "^CodeGen$" -V

# 只运行 IR 相关
ctest -R "(Type|Value|Instruction|BasicBlock|Function|IRBuilder)" -V

# 端到端
ctest -R "(Integration|NewPipeline)" -V
```

---

## CLI 使用

```bash
cd build

# 旧管线（默认）
./tools/ll1c/ll1c.exe ../examples/func_call.ll1 -o old.asm

# 旧管线 + 优化
./tools/ll1c/ll1c.exe ../examples/func_call.ll1 --opt -o old_opt.asm

# 新 Machine IR 管线
./tools/ll1c/ll1c.exe ../examples/func_call.ll1 --new-codegen -o new.asm

# 新管线 + 优化
./tools/ll1c/ll1c.exe ../examples/func_call.ll1 --new-codegen --opt -o new_opt.asm

# 输出 LLVM IR
./tools/ll1c/ll1c.exe ../examples/func_call.ll1 --emit-llvm -o output.ll
```

---

## 示例输出（新管线）

输入 (`examples/func_call.ll1`)：
```c
fn int add(int a, int b) { return a + b; }
fn int main() {
    int x = add(10, 20);
    print(x);
    return 0;
}
```

输出 (`--new-codegen`)：
```asm
; Function: add
add:
    push bp
    mov  bp, sp
    sub  sp, 4
    mov  bx, bp
    sub  bx, 4
entry:
    mov [bx], dx
    mov [bx+2], dx
    mov dx, [bx]
    mov cx, [bx+2]
    mov ax, dx
    add ax, cx
    mov ax, ax
    mov  sp, bp
    pop  bp
    ret

; Function: main
main:
    push bp
    mov  bp, sp
    sub  sp, 2
    mov  bx, bp
    sub  bx, 2
entry:
    push dx
    push dx
    call add
    add sp, 4
    mov [bx], dx
    mov dx, [bx]
    push dx
    call __print
    add sp, 2
    mov ax, dx
    mov  sp, bp
    pop  bp
    ret
hlt
```

---

## 编译流水线参考数据

### test/passdemo/ — Pass 效果演示

展示 7 个 Pass 对源码的逐步优化效果，每个 Pass 后 dump IR：

| 文件 | 说明 |
|------|------|
| `pass_demo.ll1` | 测试源文件（含 while 循环、算术、条件） |
| `00_unoptimized.ll` | 未优化的原始 IR（alloca + store + load） |
| `01_after_Mem2Reg_main.ll` | Mem2Reg 后：alloca 消除，phi 节点出现 |
| `02_after_PhiElimination_main.ll` | PhiElim 后：phi → alloca+store+load |
| `03_after_InstCombine_main.ll` | InstCombine 后：常量折叠、代数化简 |
| `04_after_Reassociate_main.ll` | Reassociate 后：表达式重结合 |
| `05_after_GVN_main.ll` | GVN 后：公共子表达式消除 |
| `06_after_SimplifyCFG_main.ll` | SimplifyCFG 后：CFG 简化 |
| `07_after_DCE_main.ll` | DCE 后：死代码消除 |
| `08_final.asm` | 最终 8086 汇编输出 |
| `summary.md` | 每个 Pass 效果的 diff 分析 |

### test/bubbletest/ — 冒泡排序全链路

真实算法（20 个元素冒泡排序）的完整编译流水线：

| 文件 | 说明 |
|------|------|
| `bubble_sort.ll1` | 冒泡排序源文件 |
| `00_unoptimized.ll` 至 `07_after_DCE.ll` | 逐 Pass IR dump |
| `08_final.asm` | 最终 8086 汇编 |
| `summary.md` | 编译过程分析（BB 数变化、关键优化） |

### 生成 Pass Dump

```bash
# 在 passdemo 目录生成最新的逐 Pass IR
cd build
./tools/ll1c/ll1c.exe ../test/passdemo/pass_demo.ll1 --dump-pass-ir=../test/passdemo/ -o ../test/passdemo/pass_demo.asm
```

---

## 手动验证（emu8086）

1. 编译源文件：
```bash
./tools/ll1c/ll1c.exe ../examples/for_test.ll1 --new-codegen -o for_test_new.asm
```

2. 在 emu8086 中打开 `for_test_new.asm`

3. 运行 → 查看输出：
   - `for_test.ll1` 求和 0-9 → 应输出 `45`
   - `func_call.ll1` → 应输出 `30`
   - `bubble_sort.ll1` → 应输出排序后的 20 个随机数
