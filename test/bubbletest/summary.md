# Bubble Sort — 编译器全链路中间结果

> 源语言：LL1 | 编译器：ll1c | 目标：8086 汇编 (emu8086)

---

## 源文件

**[bubble_sort.ll1](bubble_sort.ll1)** — LL1 语言写的选择排序，对 20 个整数升序排列并打印。

```c
fn int main() {
    int arr[20]; int i = 0; int j = 0; int temp = 0;
    arr[0] = 328; ... arr[19] = 48;   // 初始化

    i = 0;
    while (i < 20) {                   // 外层循环
        j = i;
        while (j < 20) {               // 内层循环
            if (arr[i] > arr[j]) {     // 比较
                temp = arr[i];         // 交换
                arr[i] = arr[j];
                arr[j] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }

    i = 0;
    while (i < 20) { print(arr[i]); i = i + 1; }  // 输出
    return arr[0];
}
```

---

## 编译流水线

```
bubble_sort.ll1 (源文件)
    │
    ▼  Lexer → Parser → Sema → IRGen
    │
00_unoptimized.ll ...................... 未优化 LLVM IR
    │
    ▼  Mem2Reg
01_after_Mem2Reg.ll .................... 内存提升为 SSA 寄存器
    │
    ▼  PhiElimination
02_after_PhiElimination.ll ............. Phi 节点消除 (alloca+store+load)
    │
    ▼  InstCombine
03_after_InstCombine.ll ................ 指令合并 (常量折叠、代数化简)
    │
    ▼  Reassociate
04_after_Reassociate.ll ................ 表达式重关联 (常量集中)
    │
    ▼  GVN (全局值编号)
05_after_GVN.ll ........................ 公共子表达式消除
    │
    ▼  SimplifyCFG
06_after_SimplifyCFG.ll ................ 控制流简化 (分支优化、空块消除)
    │
    ▼  DCE (死代码消除)
07_after_DCE.ll ........................ 无用指令删除
    │
    ▼  CodeGen (IR → 8086 汇编)
08_final.asm ........................... 最终 8086 汇编输出
```

---

## 各阶段文件说明

| 文件 | 说明 | BB数 | 关键变化 |
|------|------|------|---------|
| **bubble_sort.ll1** | LL1 源代码 | — | 数组初始化 + 嵌套 while 循环 |
| **00_unoptimized.ll** | IRGen 直接输出 | 12 | alloca/load/store 模式，while→br+icmp |
| **01_after_Mem2Reg.ll** | 内存提升 | 12 | 局部变量提升为 SSA，插入 Phi 节点 |
| **02_after_PhiElimination.ll** | Phi 消除 | 12 | Phi→alloca+store+load（CodeGen 兼容） |
| **03_after_InstCombine.ll** | 指令合并 | 12 | 常量折叠、add x,0→x 等化简 |
| **04_after_Reassociate.ll** | 表达式重排 | 12 | 表达式树重排，常量前置 |
| **05_after_GVN.ll** | 值编号 | 12 | 公共子表达式消除 |
| **06_after_SimplifyCFG.ll** | CFG 简化 | 12 | 条件分支简化、空基本块消除 |
| **07_after_DCE.ll** | 死代码 | 12 | 删除不可达/无副作用指令 |
| **08_final.asm** | 8086 汇编 | — | 栈帧 (push bp/mov bp,sp)、数组 [bx+si]、__print |

---

## 关键技术指标

| 指标 | 数值 |
|------|------|
| 源文件行数 | 47 |
| IR 基本块数 | 12 |
| 优化 Pass 数 | 7 |
| 最终汇编行数 | 237 |
| 数组元素 | 20 个 16-bit 整数 |
| 栈帧大小 | 46 字节 (arr 40 + i 2 + j 2 + temp 2) |
| 寄存器分配 | 3 个 (AX/CX/DX) 轮转 |
| 输出格式 | emu8086 兼容 |

---

## 如何重现

```bash
cd build
cmake --build .
./tools/ll1c/ll1c.exe ../test/bubbletest/bubble_sort.ll1 --opt -o bubble_sort.asm
```

在 emu8086 中打开 `bubble_sort.asm`，运行后应在屏幕输出：
```
13 16 17 45 48 53 58 72 115 126 141 217 280 303 328 347 378 380 380 457
```

---

## 查看中间 IR

```bash
# 未优化 IR
./tools/ll1c/ll1c.exe ../test/bubbletest/bubble_sort.ll1 --emit-llvm -o unoptimized.ll

# 优化后 IR (需临时修改 Compiler::optimize 加 setOptDumpDir)
# 各阶段 IR 已提取到 test/bubbletest/01~07_after_*.ll
```
