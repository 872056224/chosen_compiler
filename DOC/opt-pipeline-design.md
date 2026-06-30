# LL1 Compiler — 优化管线设计

> 类 LLVM New PassManager 架构 | 版本 v2.0 | 2026-06-21

---

## 1. 概述

本文档描述 LL1 编译器的优化管线（Optimization Pipeline）设计，采用简化版 LLVM New PassManager (NPM) 架构。

### 设计原则

- **参考 LLVM NPM**：AnalysisManager 缓存/失效、PreservedAnalyses 返回、PassBuilder 构建管线
- **简化类型擦除**：使用虚基类 (`FunctionPass`/`ModulePass`) 替代模板化的 PassConcept/PassModel
- **懒加载分析**：AnalysisManager 仅在查询时计算，结果自动缓存
- **增量失效**：Pass 通过 PreservedAnalyses 声明保留哪些分析，AnalysisManager 据此失效缓存

---

## 2. 与 LLVM NPM 的差异

| 特性 | LLVM NPM | LL1 简化 NPM |
|------|----------|-------------|
| 多态机制 | PassConcept/PassModel 类型擦除 | 虚基类 FunctionPass/ModulePass |
| 容器类型 | DenseMap, SmallPtrSet | std::unordered_map, std::set |
| Pass 注册 | PassInfoMixin CRTP | 虚函数 passName() |
| 分析注册 | AnalysisInfoMixin + 静态 Key | 分析类内嵌静态 AnalysisKey |
| PassInstrumentation | 完整插桩框架 | 内联 DEBUG 日志 |
| 管线构建 | PassBuilder::buildPerModuleDefaultPipeline | buildDefaultPipeline() |
| 内联适配器 | ModuleToFunctionPassAdaptor (独立类) | 直接持有 FunctionPassManager |

---

## 3. 核心基础设施

### 3.1 文件布局

```
include/ll1/Opt/
  PassManager.h              # PassManager, AnalysisManager, PreservedAnalyses
  Passes/
    Dominators.h             # DominatorTree + DominatorTreeAnalysis
    Mem2Reg.h                # Mem2Reg pass
    InstCombine.h            # InstCombine pass
    Reassociate.h            # Reassociate pass
    GVN.h                    # GVN pass
    SimplifyCFG.h            # SimplifyCFG pass
    DCE.h                    # DCE pass

lib/Opt/
  PassManager.cpp            # PassManager 实现
  Passes/
    Dominators.cpp
    Mem2Reg.cpp
    InstCombine.cpp
    Reassociate.cpp
    GVN.cpp
    SimplifyCFG.cpp
    DCE.cpp
```

### 3.2 AnalysisKey & PreservedAnalyses

```cpp
// 不透明分析 ID（8 字节对齐，避免 typeid 的 ABI 问题）
struct alignas(8) AnalysisKey {};
struct alignas(8) AnalysisSetKey {};

// 分析集声明
class CFGAnalyses {
public:
    static AnalysisSetKey* ID() { return &SetKey; }
private:
    static AnalysisSetKey SetKey;
};

template<typename IRUnitT>
class AllAnalysesOn {
public:
    static AnalysisSetKey* ID() { return &SetKey; }
private:
    static AnalysisSetKey SetKey;
};
```

**PreservedAnalyses** 是每个 Pass 的返回值，声明该 Pass 保留了哪些分析：

- `PreservedAnalyses::all()` — 什么都没改，所有分析仍有效
- `PreservedAnalyses::none()` — 改了 IR，所有分析都需重新计算
- `.preserve<AnalysisT>()` — 显式保留某个分析
- `.preserveSet<CFGAnalyses>()` — 保留整个分析集
- `.abandon<AnalysisT>()` — 显式废弃某个分析
- `.intersect(PA)` — 取交集（PassManager 聚合多个 Pass 结果时使用）

### 3.3 Pass 基类

```cpp
class FunctionPass {
public:
    virtual ~FunctionPass() = default;
    virtual PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) = 0;
    virtual const char* passName() const = 0;
};

class ModulePass {
public:
    virtual ~ModulePass() = default;
    virtual PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) = 0;
    virtual const char* passName() const = 0;
};
```

### 3.4 PassManager

```cpp
class FunctionPassManager {
    std::vector<std::unique_ptr<FunctionPass>> Passes;
public:
    template<typename PassT> void addPass(PassT p);
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    bool isEmpty() const;
};

class ModulePassManager {
    std::vector<std::unique_ptr<ModulePass>> Passes;
public:
    template<typename PassT> void addPass(PassT p);
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    bool isEmpty() const;
};
```

每个 `run()` 方法：
1. 初始化 PA = `PreservedAnalyses::all()`
2. 依次运行每个 Pass
3. 将每个 Pass 返回的 PA 与初始 PA 取交集
4. 调用 `AM.invalidate(IR, PA)` 失效不再保留的分析
5. 返回最终 PA

### 3.5 AnalysisManager

```cpp
template<typename IRUnitT>
class AnalysisManager {
public:
    // 注册分析 Pass（通常由 PassBuilder 在管线构建时调用）
    template<typename PassT, typename PassBuilderT>
    bool registerPass(PassBuilderT &&builder);

    // 获取分析结果（若未缓存则运行分析）
    template<typename PassT>
    typename PassT::Result& getResult(IRUnitT &IR);

    // 仅获取已缓存结果（不运行分析，可能返回 nullptr）
    template<typename PassT>
    typename PassT::Result* getCachedResult(IRUnitT &IR);

    // 根据 PreservedAnalyses 失效分析缓存
    void invalidate(IRUnitT &IR, const PreservedAnalyses &PA);

    // 清空特定 IR 单元的所有缓存
    void clear(IRUnitT &IR);
};
```

内部结构：
- **AnalysisPassMap**：`AnalysisKey* → unique_ptr<AnalysisPassConcept>` — 已注册的分析 Pass
- **AnalysisResultMap**：`{AnalysisKey*, IRUnitT*} → unique_ptr<ResultConcept>` — 分析结果缓存

查询流程 (`getResult<DominatorTreeAnalysis>`):
1. 检查 `AnalysisResultMap` 是否有缓存，有则直接返回
2. 从 `AnalysisPassMap` 找到注册的分析 Pass
3. 调用其 `run()` 方法，结果存入缓存
4. 返回结果引用

失效流程 (`invalidate`):
1. 遍历该 IR 单元的所有缓存结果
2. 对每个结果，检查 `PreservedAnalyses::isPreserved<AnalysisT>()`
3. 若未保留，则从缓存中删除

### 3.6 ModuleToFunctionPassAdaptor

```cpp
class ModuleToFunctionPassAdaptor : public ModulePass {
    FunctionPassManager FPM;
public:
    explicit ModuleToFunctionPassAdaptor(FunctionPassManager fpm)
        : FPM(std::move(fpm)) {}

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) override {
        // 为每个函数创建独立的 FunctionAnalysisManager
        // 初始管线无模块级分析，所以 FAM 完全独立创建
        for (auto &F : M.getFunctions()) {
            FunctionAnalysisManager FAM;
            // 将运行时需要的分析注册到 FAM
            // (PassBuilder 在构造时通过 registerAnalyses 注入)
            registerAnalyses(FAM);
            FPM.run(*F, FAM);
        }
        return PreservedAnalyses::none();
    }

    // 由 PassBuilder 设置，用于在 FAM 上注册分析
    std::function<void(FunctionAnalysisManager&)> registerAnalyses;
};
```

> **注**：LLVM NPM 使用 `InnerAnalysisManagerProxy` 将 MAM 分析代理到 FAM。我们的初始管线所有 6 个 Pass 都是函数级 Pass，不需要模块级分析，因此简化为独立的 FAM 创建。后续如有模块级 Pass（如全局变量优化），再添加代理机制。

### 3.7 PassBuilder

```cpp
class PassBuilder {
public:
    // 构建默认优化管线
    ModulePassManager buildDefaultPipeline() {
        // 构建函数级 Pass 管线
        FunctionPassManager FPM;
        FPM.addPass(Mem2Reg());
        FPM.addPass(InstCombine());
        FPM.addPass(Reassociate());
        FPM.addPass(GVN());
        FPM.addPass(SimplifyCFG());
        FPM.addPass(DCE());

        // 包装为模块级 Pass，同时注入分析注册回调
        ModuleToFunctionPassAdaptor adaptor(std::move(FPM));
        adaptor.registerAnalyses = [](FunctionAnalysisManager &FAM) {
            FAM.registerPass<DominatorTreeAnalysis>([]() {
                return DominatorTreeAnalysis();
            });
        };

        ModulePassManager MPM;
        MPM.addPass(std::move(adaptor));
        return MPM;
    }
};
```

默认管线：**Mem2Reg → PhiElimination → InstCombine → Reassociate → GVN → SimplifyCFG → DCE**

---

## 4. 分析基础设施

### 4.1 DominatorTree

**算法**：Cooper-Harvey-Kennedy 迭代数据流算法（与 LLVM 相同）

**复杂度**：最坏 O(N²)，结构化代码 O(N)

**计算结果**：
- `IDom` — 每个 BB 的立即支配者
- `DomFrontier` — 支配边界（phi 放置用）
- `Children` — 支配树子节点
- `LevelOrder` — 从根节点的层序遍历

```cpp
class DominatorTree {
public:
    struct Result {
        std::unordered_map<BasicBlock*, BasicBlock*> IDom;
        std::unordered_map<BasicBlock*, std::set<BasicBlock*>> DomFrontier;
        std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> Children;
        std::vector<BasicBlock*> LevelOrder;

        bool dominates(BasicBlock *A, BasicBlock *B) const;
        bool strictlyDominates(BasicBlock *A, BasicBlock *B) const;
        BasicBlock* findNearestCommonDominator(BasicBlock *A, BasicBlock *B) const;
    };

    using Result = Result;
    Result run(Function &F, FunctionAnalysisManager &FAM);
};
```

**注册为分析 Pass**：

```cpp
class DominatorTreeAnalysis {
public:
    static AnalysisKey Key;
    using Result = DominatorTree::Result;

    Result run(Function &F, FunctionAnalysisManager &FAM) {
        return DominatorTree().run(F, FAM);
    }
};
```

---

## 5. Pass 详细设计

### 5.1 Mem2Reg（内存到寄存器提升）

**目标**：将 alloca/load/store 模式转换为 SSA phi 节点

**算法**（标准 Sreedhar-Gao phi 放置 + 重命名）：

```
Phase 1 — 收集需要提升的 alloca：
  遍历函数，收集所有 "可提升的" alloca（非数组、非跨 BB 地址逃逸）

Phase 2 — 计算支配边界（需要 DominatorTree）：
  对每个 alloca，找到所有存在 store 的 BB
  计算这些 BB 的 Iterated Dominance Frontier (IDF)
  在 IDF 的每个 BB 入口插入 phi 节点

Phase 3 — 重命名（沿支配树遍历）：
  维护 reaching-def 栈（每个 alloca 一个）
  遇到 store → push 新值，更新栈
  遇到 load → 替换为栈顶值
  遇到 phi → 设为栈顶值，然后 push phi
  退出 BB → pop 所有本 BB 的 push

Phase 4 — 清理：
  删除所有已提升的 alloca + load + store
  删除所有无操作的 phi（所有入口相同值）
```

**示例**：

```
优化前：                      优化后：
entry:                        entry:
  %x = alloca i16               br label %while_cond
  store i16 0, ptr %x
  br label %while_cond         while_cond:
while_cond:                      %x.0 = phi i16 [0, %entry], [%x.1, %while_body]
  %v1 = load i16, ptr %x        %cmp = icmp slt i16 %x.0, 10
  %cmp = icmp slt i16 %v1, 10   br i1 %cmp, label %while_body, label %while_exit
  br i1 %cmp, label %while_body, label %while_exit
                               while_body:
while_body:                      %add = add i16 %x.0, 1
  %v2 = load i16, ptr %x        br label %while_cond
  %add = add i16 %v2, 1
  store i16 %add, ptr %x       while_exit:
  br label %while_cond           ret i16 0
while_exit:
  ret i16 0
```

**分析依赖**：需要 DominatorTree
**PreservedAnalyses 返回**：`PreservedAnalyses::none()`（大幅修改 IR）

> **注**：当前 Mem2Reg 支持全函数跨基本块提升，产生真实 Phi 节点。Phi 节点由后续的 PhiElimination pass 转换为 alloca+store+load 模式供 CodeGen 处理。

---

### 5.2 PhiElimination（Phi 节点消除）

**目标**：将 SSA Phi 节点转换为 CodeGen 可处理的内存操作。

**算法**：

```
对每个 PhiInst %phi = phi [%v1, %BB1], [%v2, %BB2], ...：
  1. 在 entry 块创建临时 alloca：%phi_slot = alloca <ty>
  2. 对每个前驱 BB_i：在终结指令之前插入 store %v_i, %phi_slot
  3. 将 phi 替换为：%reload = load %phi_slot
  4. 删除 phi
```

**分析依赖**：无
**PreservedAnalyses 返回**：`PreservedAnalyses().preserveSet<CFGAnalyses>()`

---

### 5.3 InstCombine（指令合并）

**目标**：代数恒等式化简、冗余指令消除

**算法**：单遍 worklist 驱动 peephole 优化

**第一批模式**：

| 模式 | 替换 |
|------|------|
| `add x, 0` | `x` |
| `sub x, 0` | `x` |
| `mul x, 1` | `x` |
| `div x, 1` | `x` |
| `sub x, x` | `0` |
| `and x, -1` (全1) | `x` |
| `or x, 0` | `x` |
| `xor x, 0` | `x` |
| `icmp eq C, C` | `true` |
| `trunc(zext x)` | `x` (当类型匹配) |
| `zext(trunc x)` | `x` (当类型匹配) |
| `not(not x)` | `x` |
| `neg(neg x)` | `x` |
| `br(icmp eq C1, C2)` 常量折叠 | 无条件 `br` |
| `add C1, C2` 常量折叠 | `C1 + C2` |
| `sub C1, C2` 常量折叠 | `C1 - C2` |

**分析依赖**：无
**PreservedAnalyses 返回**：`PreservedAnalyses().preserveSet<CFGAnalyses>().preserve<DominatorTreeAnalysis>()`（不改 CFG）

---

### 5.4 Reassociate（表达式重关联）

**目标**：规范化表达式树，将常量集中便于折叠

**算法**：

```
Phase 1 — 线性化：
  将嵌套的 add/sub 链展平为线性列表
  例：(a + b) + c → [a, b, c]

Phase 2 — 排序：
  常量排最前，同变量操作数相邻
  例：[1, x, 3] → [1, 3, x]

Phase 3 — 重建：
  按 (a+b)+(c+d) 平衡树重建
  常量折叠：1 + 3 → 4
  例：[1, 3, x] → (1 + 3) + x → 4 + x
```

**示例**：`(1 + x) + 3` → `x + 4`

**分析依赖**：无
**PreservedAnalyses 返回**：`PreservedAnalyses().preserveSet<CFGAnalyses>()`

---

### 5.5 GVN（全局值编号）

**目标**：消除冗余计算（公共子表达式消除 + 部分常量传播）

**算法**：支配树层序遍历 + 哈希值编号

```
Phase 1 — 初始化：
  给每个常量分配唯一值编号
  给每个 Argument 分配唯一值编号

Phase 2 — 按支配树层序遍历每条指令：
  计算指令的哈希签名：
    hash = hash(opcode, operand_VN_1, operand_VN_2, ...)
  如果哈希签名已经存在于当前可见的表达式集合中：
    → 找到冗余指令，用支配者的结果替换当前指令
  否则：
    → 这是新的值编号，加入可见集合

Phase 3 — 清除替换：
  对每条被替换的指令，检查是否已死（无 use），若死则删除
```

**示例**：

```
优化前：                      优化后：
  %a = add %x, %y               %a = add %x, %y
  ...                           ...
  %b = add %x, %y              ; 删除 %b，用 %a 替换所有 %b 的 use
```

**限制**：不做 PRE（部分冗余消除），不做内存别名分析（仅处理寄存器值）

**分析依赖**：需要 DominatorTree
**PreservedAnalyses 返回**：`PreservedAnalyses().preserveSet<CFGAnalyses>()`

---

### 5.6 SimplifyCFG（控制流简化）

**目标**：减少基本块数量，简化控制流

**变换**：

| 变换 | 条件 | 操作 |
|------|------|------|
| 空块消除 | BB 只有无条件的 br | 重定向前驱到后继，删除 BB |
| 单前驱合并 | 前驱只有一个后继 | 合并 BB 到前驱 |
| 跳转跳转折叠 | `br label %B` 且 B 是 `br label %C` | `br label %C` |
| 不可达块删除 | BB 无前驱（非入口） | 删除 BB |
| 条件分支简化 | `br i1 %cond, label %A, label %A` | `br label %A` |
| 常量条件分支 | `br i1 true/false, ...` | `br label %then` / `br label %else` |

**分析依赖**：需要 DominatorTree
**PreservedAnalyses 返回**：`PreservedAnalyses().preserve<DominatorTreeAnalysis>()` (更新后有效) 或无块变化时 `PreservedAnalyses::all()`

---

### 5.7 DCE（死代码消除）

**目标**：移除无副作用的无用指令

**算法**：Aggressive DCE（标记-清扫）

```
Phase 1 — 初始化：
  将所有指令标记为 "死"

Phase 2 — 标记存活（根标记）：
  以下指令为 "活"：
    - 终止指令（ret, br）
    - Store 指令（有副作用）
    - Call 指令（有副作用）
    - ArrayStore 指令

Phase 3 — 传播存活（worklist）：
  while worklist 非空：
    取出一条活指令
    对其每个操作数，若操作数是 Instruction：
      标记该操作数为活，加入 worklist

Phase 4 — 清扫：
  遍历所有 BB，删除所有仍标记为 "死" 的指令
```

**说明**：与普通 DCE（仅删除无用指令的单遍）不同，Aggressive DCE 会追踪使用链，能删除整条死代码链。

**分析依赖**：无
**PreservedAnalyses 返回**：`PreservedAnalyses().preserveSet<CFGAnalyses>()`（不改 CFG）

---

## 6. 管线执行流程

```
IRGen 产生 Module
        │
        ▼
ModulePassManager::run(Module &M, MAM)
        │
        ▼
ModuleToFunctionPassAdaptor::run()  ─── 对 Module 中的每个 Function
        │
        ▼
FunctionPassManager::run(Function &F, FAM)
        │
        ├── Mem2Reg::run(F, FAM)
        │      ├── FAM.getResult<DominatorTreeAnalysis>(F)   ← 首次调用，计算并缓存
        │      ├── 计算 IDF，放置 phi
        │      ├── 沿支配树重命名
        │      └── return PreservedAnalyses::none()
        │      FAM.invalidate(F, none())  → 清除 DomTree 缓存（CFG 改变了）
        │
        ├── PhiElimination::run(F, FAM)
        │      ├── 遍历所有 Phi 节点
        │      ├── 创建 entry alloca + 前驱 store + 替换为 load
        │      └── return PA.preserveSet<CFGAnalyses>()
        │
        ├── InstCombine::run(F, FAM)
        │      ├── worklist 驱动 peephole
        │      └── return PA.preserveSet<CFGAnalyses>().preserve<DominatorTreeAnalysis>()
        │      FAM.invalidate(F, PA)  → DomTree 仍有效（但未缓存，下次重新计算）
        │
        ├── Reassociate::run(F, FAM)
        │      ├── 线性化 + 排序 + 重建
        │      └── return PA.preserveSet<CFGAnalyses>()
        │
        ├── GVN::run(F, FAM)
        │      ├── FAM.getResult<DominatorTreeAnalysis>(F)   ← 重新计算（新 IR）
        │      ├── 值编号，消除冗余
        │      └── return PA.preserveSet<CFGAnalyses>()
        │
        ├── SimplifyCFG::run(F, FAM)
        │      ├── FAM.getCachedResult<DominatorTreeAnalysis>(F)  ← 命中缓存！
        │      ├── 合并/删除基本块
        │      └── return PA.preserve<DominatorTreeAnalysis>()
        │
        └── DCE::run(F, FAM)
               ├── 标记-清扫
               └── return PA.preserveSet<CFGAnalyses>()
```

---

## 7. 驱动器集成

修改 `lib/Driver/Compiler.cpp`，在 IRGen 和 CodeGen 之间插入优化：

```cpp
std::unique_ptr<Module> Compiler::optimize(std::unique_ptr<Module> module) {
    PassBuilder pb;
    auto mpm = pb.buildDefaultPipeline();

    ModuleAnalysisManager MAM;
    auto PA = mpm.run(*module, MAM);
    return module;
}
```

管道变为：**Lexer → Parser → Sema → IRGen → Opt → CodeGen**

CLI 增加 flags：
- `--opt` — 启用优化（开启默认管线）
- `--no-opt` — 禁用优化（默认，保持当前行为）
- `--emit-llvm` — 输出优化后的 .ll 文件（在 optimize 后调用 IRPrinter）

---

## 8. CMake 集成

在 `lib/CMakeLists.txt` 中新增：

```cmake
# Opt 模块
add_library(ll1Opt OBJECT
    Opt/PassManager.cpp
    Opt/Passes/Dominators.cpp
    Opt/Passes/Mem2Reg.cpp
    Opt/Passes/InstCombine.cpp
    Opt/Passes/Reassociate.cpp
    Opt/Passes/GVN.cpp
    Opt/Passes/SimplifyCFG.cpp
    Opt/Passes/DCE.cpp
)
target_link_libraries(ll1Opt ll1IR)
```

---

## 9. 测试计划

新增以下测试（`test/Opt/`）：

| 测试 | 覆盖 |
|------|------|
| test_mem2reg | alloca/load/store → phi，多重赋值，循环变量 |
| test_instcombine | 代数恒等式、常量折叠 |
| test_reassociate | 表达式重排、常量合并 |
| test_gvn | 公共子表达式消除 |
| test_simplifycfg | 空块删除、块合并、不可达块消除 |
| test_dce | 死指令删除、死代码链消除 |
| test_pipeline | 完整管线回归测试 |

所有测试链接 `ll1Opt` + `ll1IR` 库。

---

## 10. 实现阶段

| 阶段 | 内容 | 文件 | 状态 |
|------|------|------|------|
| 1 | PassManager + PreservedAnalyses + AnalysisManager 核心 | PassManager.h/.cpp | ✅ |
| 2 | DominatorTree + DominatorTreeAnalysis | Dominators.h/.cpp | ✅ |
| 3 | Mem2Reg（全函数跨 BB + Phi 放置） | Mem2Reg.h/.cpp | ✅ |
| 4 | PhiElimination（Phi → alloca+store+load） | PhiElim.h/.cpp | ✅ |
| 5 | InstCombine | InstCombine.h/.cpp | ✅ |
| 6 | Reassociate | Reassociate.h/.cpp | ✅ |
| 7 | GVN | GVN.h/.cpp | ✅ |
| 8 | SimplifyCFG | SimplifyCFG.h/.cpp | ✅ |
| 9 | DCE | DCE.h/.cpp | ✅ |
| 10 | PassBuilder + Driver 集成 | PassBuilder, Compiler.cpp | ✅ |
| 11 | 线性扫描寄存器分配 | CodeGen.h/.cpp | ✅ |
| — | CMake + 测试 | CMakeLists.txt, test/Opt/ | ✅ |
