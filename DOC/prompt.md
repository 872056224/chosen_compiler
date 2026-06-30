# Claude Code 提示词

你是一名 LLVM Compiler Architect。

现在请帮助我设计并实现一个基于 LLVM 架构的教学型编译器项目。

## 项目目标

设计一个完整编译器：

```
LL(1) Language
        ↓
Lexical Analysis
        ↓
LL(1) Recursive Descent Parser
        ↓
AST
        ↓
Semantic Analysis
        ↓
LLVM IR Generation
        ↓
LLVM Optimization Passes
        ↓
8086 Assembly
        ↓
emu8086
```

最终输出：

```
mov ax,1
add ax,2
mov result,ax
```

能够在 emu8086 中运行。

------

## 已有资源是llvm源码文件夹

项目目录旁边放置：

```
llvm-project-main/
```

这是 LLVM 最新源码。

你必须：

1. 深度阅读 LLVM 源码结构
2. 学习 Clang 前端设计
3. 学习 Kaleidoscope 示例
4. 学习 LLVM Target Backend 设计
5. 学习 LLVM PassManager
6. 参考 LLVM 官方实现方式

不要凭空设计。

所有架构必须参考 LLVM 源码中的真实实现。

------

# 总体要求

本项目并不是重新实现 LLVM。

而是：

```
使用 LLVM 框架
只保留必要组件
实现一个最小可运行编译器
```

必须遵循：

```
最小化原则
增量实现原则
可运行优先原则
```

不要一开始实现完整 LLVM Backend。

------

# 第一阶段任务

请先分析并设计：

## 1 编译器总体架构

输出：

```
目录结构

模块划分

类图

数据流图

调用关系图
```

说明：

```
Lexer
Parser
AST
Semantic
IRGen
Optimizer
Backend
Driver
```

之间如何协作。

------

## 2 LL(1)语言设计

请设计一种：

```
教学型语言
```

要求：

### 数据类型

支持：

```
int
char
bool
```

------

### 表达式

支持：

```
+
-
*
/
%
```

------

### 关系运算

支持：

```
<
>
<=
>=
==
!=
```

------

### 控制流

支持：

```
if
else
while
for
break
continue
return
```

------

### 函数

支持：

```
function
parameter
local variable
```

------

### 示例

例如：

```
int main() {
    int a = 10;
    int b = 20;

    if(a < b){
        return b;
    }

    return a;
}
```

------

## 3 LL(1)文法设计

要求：

输出完整：

```
Token定义

EBNF

FIRST集合

FOLLOW集合

预测分析表
```

验证：

```
文法是否满足LL(1)
```

若冲突：

必须进行：

```
消除左递归
提取左公因子
```

直到满足 LL(1)。

------

## 4 AST设计

参考：

Clang AST设计思想。

输出：

```
ASTNode

Expr

Stmt

Decl
```

继承体系。

给出 UML 图。

------

## 5 语义分析设计

设计：

```
Symbol Table

Scope

Type Checker
```

支持：

```
变量声明检查
重复定义检查
未定义变量检查
类型检查
函数参数检查
```

------

## 6 LLVM IR生成设计

参考：

LLVM Kaleidoscope。

设计：

```
AST
    ↓
LLVM IR
```

映射关系。

例如：

### 变量

```
int a = 1;
```

生成：

```
%a = alloca i32
store i32 1, ptr %a
```

------

### 表达式

```
a+b
```

生成：

```
%tmp = add i32 ...
```

------

### if

```
if(a<b)
```

生成：

```
icmp
br
```

------

### while

生成：

```
cond
body
exit
```

对应CFG。

------

## 7 LLVM优化Pass选择

不要使用全部Pass。

请分析：

哪些Pass适合教学编译器。

推荐：

```
mem2reg
instcombine
simplifycfg
gvn
dce
```

输出：

```
每个Pass作用

是否保留

原因
```

并给出：

```
PassBuilder
```

配置代码。

------

## 8 后端设计

重点分析：

LLVM IR
 如何映射到
 8086汇编。

------

不要直接实现完整LLVM Target。

先设计：

```
Instruction Selection

Register Allocation

Assembly Emission
```

最小方案。

支持：

```
MOV
ADD
SUB
MUL
DIV
CMP
JMP
JE
JNE
CALL
RET
```

即可。

------

## 9 emu8086约束分析

请研究：

8086架构：

```
AX
BX
CX
DX
SI
DI
BP
SP
```

以及：

```
段寄存器

内存模型

调用约定
```

输出：

```
LLVM IR -> 8086映射表
```

------

## 10 实现路线

给出：

### MVP版本

```
变量
表达式
if
while
```

------

### 第二版

```
函数
作用域
优化Pass
```

------

### 第三版

```
8086后端完善
```

------

## 输出要求

不要立即写代码。

先完成：

```
架构设计文档
技术方案
模块拆分
开发路线图
风险分析
```

对于每个模块：

```
参考LLVM源码位置
参考类
参考设计模式
```

并注明：

```
为什么这样设计
与LLVM官方实现的对应关系
```

最后给出：

```
第一步应该实现什么
第二步应该实现什么
第三步应该实现什么
```

保证整个项目最终可以：

```
LL(1) Source
    ↓
LLVM IR
    ↓
8086 Assembly
    ↓
emu8086运行
```

------

这个提示词的好处是，它会强制 Claude Code **先做架构设计，再开始编码**，而不是一上来就在 LLVM 源码里乱改。对于你这个课程设计规模，我建议最终实现成：

```
LL(1)
↓
AST
↓
LLVM IR
↓
自定义8086代码生成器
↓
emu8086
```

而不是完整 LLVM Target Backend。这样工作量控制在 2~4 周内，答辩时也能讲清楚 LLVM 前端、中端 Pass、后端代码生成的完整流程。