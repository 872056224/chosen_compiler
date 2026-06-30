# LL1 Compiler — 函数调用约定

> cdecl 风格调用约定 | 版本 MVP v1.0 | 2026-06-21

---

## 1. 概述

LL1 编译器采用 **C 风格调用约定（cdecl）**，核心特征：

- 参数从右到左压栈
- 调用方负责清理栈
- 返回值通过 AX 寄存器传递
- 被调用方使用标准栈帧（BP 帧指针）

---

## 2. 栈帧布局

每个函数调用在栈上建立如下结构：

```
高地址
┌──────────────┐
│   arg N      │  [bp + 2 + N*2]
├──────────────┤
│   ...        │
├──────────────┤
│   arg 2      │  [bp + 6]
├──────────────┤
│   arg 1      │  [bp + 4]
├──────────────┤
│ 返回地址      │  [bp + 2]    ← call 指令自动压栈
├──────────────┤
│ 旧 BP 值     │  [bp]        ← push bp; mov bp, sp 后 BP 指向这里
├──────────────┤
│ 局部变量 1   │  [bx]        ← bx = bp - local_size
├──────────────┤
│ 局部变量 2   │  [bx + 2]
├──────────────┤
│   ...        │
└──────────────┘ ← SP
低地址
```

**双指针方案**：BP 指向栈帧顶部（参数区上方），BX 指向局部变量区底部。
- 参数访问：`[bp + 4]` `[bp + 6]` …（正偏移）
- 局部变量：`[bx + 0]` `[bx + 2]` …（正偏移）
- 数组元素：`[bx + si]`（BX 为基址，SI 为元素索引 × 2）

---

## 3. 调用方（Caller）

### 3.1 调用流程

```
调用 add(10, 20) 翻译为：

    mov  reg, 20       ; 参数从右到左
    push reg            ; 最后参数先压
    mov  reg, 10
    push reg            ; 第一参数后压
    call add            ; CPU 自动压入返回地址
    add  sp, 4          ; 调用方清理栈（2 参数 × 2 字节）
    ; 返回值在 AX 中
```

### 3.2 寄存器状态

| 寄存器 | 调用前 | 调用后 |
|--------|--------|--------|
| AX | — | 返回值（若非 void） |
| BX | 局部变量基址 | 保留（BP 恢复后重建） |
| CX DX | — | 易失，可能被改写 |
| BP SP | 栈帧 | 恢复 |

### 3.3 void 函数

```c
fn void greet() { return; }
```

调用方：

```
    call greet
    ; add sp, 0（无参数，不清理）
    ; AX 未定义，不应使用
```

---

## 4. 被调用方（Callee）

### 4.1 函数入口

```
add:
    push bp              ; 1. 保存调用方帧指针
    mov  bp, sp          ; 2. 建立新栈帧
    sub  sp, N           ; 3. 分配 N 字节局部变量空间
    mov  bx, bp          ; 4. BX = 局部变量区起始
    sub  bx, N           ;    （BP - 局部空间大小）
```

### 4.2 参数接收

参数通过 `[bp + offset]` 读取，复制到局部 alloca：

```
    mov  ax, [bp + 4]   ; 读取第一参数
    mov  [bx], ax       ; 存入局部变量
    mov  cx, [bp + 6]   ; 读取第二参数
    mov  [bx + 2], cx   ; 存入局部变量
```

IR 层面：为每个参数创建 `alloca`，然后用 `Store(Argument, alloca)` 完成复制。

### 4.3 函数返回

```
    mov  ax, result      ; 返回值放 AX
    mov  sp, bp          ; 恢复 SP，丢弃局部变量
    pop  bp              ; 恢复调用方 BP
    ret                  ; CPU 弹出返回地址并跳转
```

void 函数不设置 AX。

---

## 5. 指令开销分析

| 操作 | 指令数 | 说明 |
|------|--------|------|
| 函数入口（有局部变量） | 5 | push bp + mov bp,sp + sub sp,N + mov bx,bp + sub bx,N |
| 函数入口（无局部变量） | 3 | push bp + mov bp,sp + mov bx,bp |
| 参数传递（每参数） | 2 | mov + push |
| 调用 | 1 | call |
| 栈清理（每参数） | 1 | add sp, N*2 |
| 函数返回 | 3 | mov sp,bp + pop bp + ret |

---

## 6. 完整示例

### LL1 源码

```c
fn int add(int a, int b) {
    return a + b;
}

fn int main() {
    int x = add(10, 20);
    print(x);
    return 0;
}
```

### 生成的 8086 汇编

```asm
add:
    push bp
    mov  bp, sp
    sub  sp, 4
    mov  bx, bp
    sub  bx, 4
    mov  ax, [bp+4]      ; 参数 a
    mov  [bx], ax
    mov  cx, [bp+6]      ; 参数 b
    mov  [bx+2], cx
    mov  dx, [bx]        ; load a
    mov  ax, [bx+2]      ; load b
    add  dx, ax          ; a + b
    mov  ax, dx          ; 结果在 AX
    mov  sp, bp
    pop  bp
    ret

main:
    push bp
    mov  bp, sp
    sub  sp, 2
    mov  bx, bp
    sub  bx, 2
    mov  cx, 20          ; 参数 #2
    push cx
    mov  dx, 10           ; 参数 #1
    push dx
    call add
    add  sp, 4            ; 清理栈
    mov  [bx], ax         ; x = 返回值
    mov  ax, [bx]
    call __print          ; print(x) → 30
    mov  cx, 0
    mov  ax, cx
    mov  sp, bp
    pop  bp
    ret
```

运行结果：输出 `30`。

---

## 7. 支持情况

| 特性 | 状态 | 说明 |
|------|------|------|
| 多参数函数 | ✅ | 支持任意数量 int 参数 |
| 返回值 (int) | ✅ | AX 传递 |
| void 返回 | ✅ | 不检查 AX |
| 嵌套调用 | ✅ | 独立栈帧 |
| 递归 | ✅ | 每层独立栈帧 |
| print/read | ✅ | 内置函数，通过 AX 传参 |
| 参数类型 | int only | char/bool 通过隐式转换 |
| 调用方清理 | ✅ | `add sp, N` |
| 寄存器保存 | ⚠️ | caller-save 未实现（后续线性扫描） |
| 可变参数 | ❌ | 不支持 |

---

## 8. 代码生成对照表

| LLVM IR 指令 | 8086 汇编模式 |
|-------------|-------------|
| `call i16 @fn(i16 %a, i16 %b)` | `push b; push a; call fn; add sp,4` |
| `ret i16 %val` | `mov ax,val; mov sp,bp; pop bp; ret` |
| `alloca i16` | `sub sp, 2` + BX 偏移分配 |
| `store i16 %v, ptr %a` | `mov [bx+N], reg` |
| `load i16, ptr %a` | `mov reg, [bx+N]` |
| Argument → alloca | `mov ax, [bp+4]; mov [bx+N], ax` |

---

## 9. 与标准 cdecl 的差异

| 项目 | 标准 cdecl | LL1 实现 |
|------|-----------|---------|
| caller-save | AX, CX, DX 由 caller 保存 | 未实现（简单轮转复用） |
| 返回值宽度 | 任意 | 固定 16-bit (AX) |
| 栈对齐 | 通常 4 字节 | 2 字节（8086 原生） |
| 浮点参数 | 通过栈或 XMM | 不支持 |
| 结构体返回 | 隐藏指针 | 不支持 |
