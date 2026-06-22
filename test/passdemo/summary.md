# Optimization Pass Demo — 全链路中间结果

> 源语言：LL1 | 编译器：ll1c | 目标：展示 7 个优化 Pass 的实际效果

---

## 源文件: [pass_demo.ll1](pass_demo.ll1)

专门设计的测试程序，每个部分触发一个特定的优化 Pass：

```c
fn int main() {
    int sum = 0;  int i = 0;
    while (i < 5) { sum = sum + i; i = i + 1; }   // → Mem2Reg
    int a = 100;
    int b = a + 0;    // → InstCombine: x+0→x
    int c = a * 1;    // → InstCombine: x*1→x
    int d = 20 + 30;  // → InstCombine: 20+30→50
    int e = d - 0;    // → InstCombine: x-0→x
    int f = a - a;    // → InstCombine: x-x→0
    int x = 5;
    int y = (1 + x) + 3;  // → Reassociate: (1+5)+3 → 4+5=9
    int z = x + (2 + x);  // → Reassociate
    int p = i + sum;  // → GVN: should dedup with q
    int q = i + sum;  // → GVN: same expression as p
    int r = p + q;
    if (0 != 0) { print(999); }    // → SimplifyCFG: dead branch
    if (1 != 0) { r = r + 1; }     // → SimplifyCFG: always true
    int dead1 = 999;   // → DCE: never used
    int dead2 = dead1 + 1;  // → DCE: dead chain
    print(sum); print(r); print(d);
    return 0;
}
```

---

## 编译流水线

```
pass_demo.ll1 (源文件)
  → Lexer → Parser → Sema → IRGen
  → 00_unoptimized.ll
  → Mem2Reg       → 01_after_Mem2Reg.ll
  → PhiElimination → 02_after_PhiElimination.ll
  → InstCombine    → 03_after_InstCombine.ll
  → Reassociate    → 04_after_Reassociate.ll
  → GVN            → 05_after_GVN.ll
  → SimplifyCFG    → 06_after_SimplifyCFG.ll
  → DCE            → 07_after_DCE.ll
  → CodeGen        → 08_final.asm
```

---

## 每个 Pass 的实际效果（diff 验证）

### 01 — Mem2Reg (内存提升)

| 效果 | 具体变化 |
|------|---------|
| 初始 store 消除 | `store i16 0, i16 %sum` 和 `store i16 0, i16 %i` 被移除 |
| alloca 保留 | 循环变量 alloca 未完全提升（use-list 清理机制限制） |

### 02 — PhiElimination (Phi 消除)

无可见变化 — Mem2Reg 未生成 Phi 节点（见上），此 Pass 为 no-op。

### 03 — InstCombine (指令合并) ⭐ 效果最显著

| 模式 | Before | After |
|------|--------|-------|
| `x + 0 → x` | `%add_tmp = add i16 %a, 0` + `store i16 %add_tmp, i16 %b` | `store i16 %a, i16 %b` |
| `x * 1 → x` | `%mul_tmp = mul i16 %a, 1` + `store i16 %mul_tmp, i16 %c` | `store i16 %a, i16 %c` |
| 常量折叠 | `%add_tmp = add i16 20, 30` + `store i16 %add_tmp, i16 %d` | `store i16 50, i16 %d` |
| `x - 0 → x` | `%sub_tmp = sub i16 %d, 0` + `store i16 %sub_tmp, i16 %e` | `store i16 %d, i16 %e` |
| `x - x → 0` | `%sub_tmp = sub i16 %a, %a` + `store i16 %sub_tmp, i16 %f` | `store i16 0, i16 %f` |

### 04 — Reassociate (表达式重关联)

| 效果 | Before | After |
|------|--------|-------|
| 常量前置+折叠 | `(1 + x) + 3` → `%add_tmp = add i16 %add_tmp, 3` | `%0 = add i16 4, %x` (1+3→4) |

### 05 — GVN (全局值编号)

无可见变化 — 重复的 `i + sum` 未被消除（GVN 需 DominatorTree 遍历优化）。

### 06 — SimplifyCFG (控制流简化)

无可见变化 — 常量条件分支 `if (0 != 0)` / `if (1 != 0)` 未折叠。

### 07 — DCE (死代码消除)

| 效果 | 具体变化 |
|------|---------|
| 死指令移除 | 删除 Reassociate 后残留的 `%add_tmp = add i16 1, %x` |

---

## 关键指标

| 指标 | 数值 |
|------|------|
| 源文件行数 | 39 |
| 未优化 IR 指令数 | ~100 |
| 有可见效果的 Pass | 4 / 7 (Mem2Reg, InstCombine, Reassociate, DCE) |
| InstCombine 消除的指令 | 8 条 (4 个模式) |
| Reassociate 重排的表达式 | 2 个 |
| DCE 消除的指令 | 1 条 |
| 最终汇编行数 | ~210 |

---

## 文件列表

| 文件 | 说明 |
|------|------|
| `pass_demo.ll1` | LL1 源文件 |
| `00_unoptimized.ll` | 未优化 IR |
| `01_after_Mem2Reg.ll` | 初始 store 消除 |
| `02_after_PhiElimination.ll` | (无变化) |
| `03_after_InstCombine.ll` | `x+0→x`, `x*1→x`, `20+30→50`, `x-0→x`, `x-x→0` |
| `04_after_Reassociate.ll` | `(1+x)+3 → 4+x` |
| `05_after_GVN.ll` | (无可见变化) |
| `06_after_SimplifyCFG.ll` | (无可见变化) |
| `07_after_DCE.ll` | 删除死 `add i16 1, %x` |
| `08_final.asm` | 8086 汇编 |
