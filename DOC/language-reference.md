# LL1 语言参考手册

## 1. 类型系统

| 类型 | 关键字 | 宽度 | 说明 |
|------|--------|------|------|
| 整数 | `int` | 16-bit | 8086 原生宽度，默认类型 |
| 字符 | `char` | 8-bit | 存储 ASCII 字符 |
| 布尔 | `bool` | 1-bit | `true` / `false` |
| 空类型 | `void` | — | 仅用于函数返回 |

### 隐式类型转换

```
char → int    ✅  8→16 位扩展
bool → int    ✅  0/1 扩展
int  → char   ❌  不允许
int  → bool   ❌  不允许
其他组合      ❌  两边类型必须兼容
```

---

## 2. 关键字

```
int       char      bool      void
if        else      while     for
break     continue  return    fn
true      false
```

**保留但 MVP 未完全实现**：`for` `break` `continue`（解析通过，代码生成未实现）

---

## 3. 运算符（按优先级从高到低）

| 优先级 | 运算符 | 含义 | 结合性 | 操作数类型 | 结果类型 |
|--------|--------|------|--------|-----------|---------|
| 1 | `()` | 函数调用 | 左 | — | 函数返回类型 |
| 2 | `-` | 一元取负 | 右 | `int` | `int` |
| | `!` | 逻辑非 | 右 | `bool` | `bool` |
| 3 | `*` | 乘法 | 左 | `int` | `int` |
| | `/` | 除法 | 左 | `int` | `int` |
| | `%` | 取模 | 左 | `int` | `int` |
| 4 | `+` | 加法 | 左 | `int` | `int` |
| | `-` | 减法 | 左 | `int` | `int` |
| 5 | `<` | 小于 | 左 | `int` | `bool` |
| | `>` | 大于 | 左 | `int` | `bool` |
| | `<=` | 小于等于 | 左 | `int` | `bool` |
| | `>=` | 大于等于 | 左 | `int` | `bool` |
| 6 | `==` | 等于 | 左 | 相同类型 | `bool` |
| | `!=` | 不等于 | 左 | 相同类型 | `bool` |
| 7 | `&&` | 逻辑与 | 左 | `bool` | `bool` |
| 8 | `=` | 赋值 | 右 | 左值 = 表达式 | 无（语句） |

---

## 4. 变量声明

```c
// 无初始值
int a;

// 带初始值
int a = 10;
char c = 65;      // char 用整数初始化
bool flag = true;

// 用表达式初始化
int sum = a + b;
```

### 规则
- 变量名：字母/下划线开头，字母/数字/下划线组成
- 同一作用域内不可重复声明
- 必须先声明再使用

---

## 5. 控制流

### if / else

```c
if (condition) {
    // condition 必须是 bool 类型
} else {
    // else 可选
}
```

### while

```c
while (condition) {
    // condition 必须是 bool 类型
}
```

### for（语法支持，代码生成未实现）

```c
for (init; cond; incr) {
    body;
}
```

### break / continue（语法支持，代码生成未实现）

---

## 6. 函数

```c
// 定义函数
fn int add(int a, int b) {
    return a + b;
}

// void 函数
fn void greet() {
    int x = 42;
    return;        // void 函数必须有 return
}

// 调用函数（语法支持，代码生成部分实现）
fn int main() {
    int result = add(10, 20);
    return result;
}
```

### 规则
- 函数必须用 `fn` 关键字声明
- 参数类型必须显式标注
- 函数名不可与变量名冲突
- 每个函数必须有 `return` 语句
- 非 void 函数必须返回匹配类型的值

---

## 7. 注释

```c
// 这是单行注释
int a = 10;  // 行尾注释也支持
```

只支持 `//` 单行注释。

---

## 8. 词法规则

### 标识符
```
[a-zA-Z_][a-zA-Z0-9_]*
```
示例：`x` `myVar` `_private` `foo123`

### 数字
```
[0-9]+
```
整数，16 位有符号范围（-32768 ~ 32767）

### 布尔常量
```
true
false
```

### 分隔符
```
(  )  {  }  ;  ,
```

---

## 9. 完整语法（EBNF）

```
Program      ::= TopLevel*
TopLevel     ::= FuncDef | VarDecl
VarDecl      ::= Type ident ('=' Expr)? ';'
FuncDef      ::= 'fn' Type ident '(' Params? ')' Stmt

Type         ::= 'int' | 'char' | 'bool' | 'void'
Params       ::= Type ident (',' Type ident)*

Stmt         ::= VarDecl
               | Expr ';'
               | 'if' '(' Expr ')' Stmt ElseClause
               | 'while' '(' Expr ')' Stmt
               | 'for' '(' Expr? ';' Expr? ';' Expr? ')' Stmt
               | 'break' ';'
               | 'continue' ';'
               | 'return' Expr? ';'
               | '{' Stmt* '}'

ElseClause   ::= 'else' Stmt | ε

Expr         ::= LogicalOr ('=' Expr)?
LogicalOr    ::= LogicalAnd ('||' LogicalAnd)*
LogicalAnd   ::= Equality ('&&' Equality)*
Equality     ::= Relational (('==' | '!=') Relational)*
Relational   ::= Additive (('<' | '>' | '<=' | '>=') Additive)*
Additive     ::= Multiplicative (('+' | '-') Multiplicative)*
Multiplicative ::= Unary (('*' | '/' | '%') Unary)*
Unary        ::= ('-' | '!') Unary | Primary
Primary      ::= ident '(' Args? ')' | ident | number | '(' Expr ')' | 'true' | 'false'
Args         ::= Expr (',' Expr)*
```

---

## 10. 示例程序

### 变量和表达式
```c
fn int main() {
    int a = 10;
    int b = 20;
    return a + b;
}
// 返回 30
```

### if/else 分支
```c
fn int main() {
    int a = 10;
    int b = 20;
    if (a < b) {
        return b;
    }
    return a;
}
// 返回 20
```

### while 循环
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
// 返回 45 (0+1+2+...+9)
```

### 复合表达式
```c
fn int main() {
    int a = 1;
    int b = 2;
    int c = a * b + 3;
    return c;
}
// 返回 5
```
