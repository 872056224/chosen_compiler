#ifndef LL1_AST_AST_H
#define LL1_AST_AST_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include <ll1/Lex/Token.h>

namespace ll1 {

// Forward declarations
class Type;

// ====== Builtin type for AST (language-level) ======
enum class BuiltinType { Void, Int, Char, Bool };
const char *builtinTypeName(BuiltinType t);

// ====== ASTNode (base) ======
class ASTNode {
public:
    enum class Kind {
        // Decl
        VarDecl, FuncDecl,
        // Stmt
        Block, IfStmt, WhileStmt, ForStmt,
        BreakStmt, ContinueStmt, ReturnStmt,
        PrintStmt,
        // Expr
        BinaryExpr, UnaryExpr, CallExpr,
        IntegerLiteral, CharLiteral, BoolLiteral, VarExpr,
        ArraySubscriptExpr, ReadExpr,
    };

    ASTNode(Kind k) : NodeKind(k) {}
    virtual ~ASTNode() = default;

    Kind getKind() const { return NodeKind; }
    SourceLocation getLoc() const { return Loc; }
    void setLoc(SourceLocation loc) { Loc = loc; }

private:
    Kind NodeKind;
    SourceLocation Loc;
};

// ====== Decl ======
class Decl : public ASTNode {
public:
    Decl(Kind k) : ASTNode(k) {}
};

class FuncDecl : public Decl {
public:
    struct Param {
        BuiltinType Ty;
        std::string Name;
    };

    FuncDecl(BuiltinType returnTy, std::string name,
             std::vector<Param> params, std::unique_ptr<class Stmt> body)
        : Decl(Kind::FuncDecl), ReturnTy(returnTy), Name(std::move(name)),
          Params(std::move(params)), Body(std::move(body)) {}

    BuiltinType getReturnType() const { return ReturnTy; }
    const std::string &getName() const { return Name; }
    const std::vector<Param> &getParams() const { return Params; }
    Stmt *getBody() const { return Body.get(); }

private:
    BuiltinType ReturnTy;
    std::string Name;
    std::vector<Param> Params;
    std::unique_ptr<Stmt> Body;
};

// ====== Stmt ======
class Stmt : public ASTNode {
public:
    Stmt(Kind k) : ASTNode(k) {}
};

class VarDecl : public Stmt {
public:
    VarDecl(BuiltinType ty, std::string name, std::unique_ptr<class Expr> init = nullptr)
        : Stmt(Kind::VarDecl), Ty(ty), Name(std::move(name)), Init(std::move(init)) {}

    BuiltinType getType() const { return Ty; }
    const std::string &getName() const { return Name; }
    Expr *getInit() const { return Init.get(); }
    bool hasInit() const { return Init != nullptr; }

    // Array support
    bool isArray() const { return IsArray; }
    int getArraySize() const { return ArraySize; }
    void setArray(int size) { IsArray = true; ArraySize = size; }

private:
    BuiltinType Ty;
    std::string Name;
    std::unique_ptr<Expr> Init;
    bool IsArray = false;
    int ArraySize = 0;
};

class Block : public Stmt {
public:
    Block(std::vector<std::unique_ptr<Stmt>> stmts = {})
        : Stmt(Kind::Block), Statements(std::move(stmts)) {}

    const std::vector<std::unique_ptr<Stmt>> &getStatements() const { return Statements; }
    void addStmt(std::unique_ptr<Stmt> s) { Statements.push_back(std::move(s)); }

private:
    std::vector<std::unique_ptr<Stmt>> Statements;
};

class IfStmt : public Stmt {
public:
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenBlock,
           std::unique_ptr<Stmt> elseBlock = nullptr)
        : Stmt(Kind::IfStmt), Cond(std::move(cond)),
          ThenBlock(std::move(thenBlock)), ElseBlock(std::move(elseBlock)) {}

    Expr *getCond() const { return Cond.get(); }
    Stmt *getThen() const { return ThenBlock.get(); }
    Stmt *getElse() const { return ElseBlock.get(); }
    bool hasElse() const { return ElseBlock != nullptr; }

private:
    std::unique_ptr<Expr> Cond;
    std::unique_ptr<Stmt> ThenBlock;
    std::unique_ptr<Stmt> ElseBlock;
};

class WhileStmt : public Stmt {
public:
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : Stmt(Kind::WhileStmt), Cond(std::move(cond)), Body(std::move(body)) {}

    Expr *getCond() const { return Cond.get(); }
    Stmt *getBody() const { return Body.get(); }

private:
    std::unique_ptr<Expr> Cond;
    std::unique_ptr<Stmt> Body;
};

class ForStmt : public Stmt {
public:
    ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond,
            std::unique_ptr<Expr> incr, std::unique_ptr<Stmt> body)
        : Stmt(Kind::ForStmt), Init(std::move(init)), Cond(std::move(cond)),
          Incr(std::move(incr)), Body(std::move(body)) {}

    Stmt *getInit() const { return Init.get(); }
    Expr *getCond() const { return Cond.get(); }
    Expr *getIncr() const { return Incr.get(); }
    Stmt *getBody() const { return Body.get(); }

private:
    std::unique_ptr<Stmt> Init;
    std::unique_ptr<Expr> Cond;
    std::unique_ptr<Expr> Incr;
    std::unique_ptr<Stmt> Body;
};

class BreakStmt : public Stmt {
public:
    BreakStmt() : Stmt(Kind::BreakStmt) {}
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt() : Stmt(Kind::ContinueStmt) {}
};

class ReturnStmt : public Stmt {
public:
    ReturnStmt(std::unique_ptr<Expr> value = nullptr)
        : Stmt(Kind::ReturnStmt), Value(std::move(value)) {}

    Expr *getValue() const { return Value.get(); }
    bool hasValue() const { return Value != nullptr; }

private:
    std::unique_ptr<Expr> Value;
};

// ====== Expr (inherits Stmt, allows Expr-as-Stmt) ======
class Expr : public Stmt {
public:
    Expr(Kind k) : Stmt(k) {}
};

class BinaryExpr : public Expr {
public:
    enum class Op { Add, Sub, Mul, Div, Rem, Lt, Gt, Le, Ge, Eq, Ne, And, Or, Assign };
    static const char *opName(Op op);

    BinaryExpr(Op op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : Expr(Kind::BinaryExpr), O(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {}

    Op getOp() const { return O; }
    Expr *getLHS() const { return LHS.get(); }
    Expr *getRHS() const { return RHS.get(); }

private:
    Op O;
    std::unique_ptr<Expr> LHS;
    std::unique_ptr<Expr> RHS;
};

class UnaryExpr : public Expr {
public:
    enum class Op { Neg, Not };
    static const char *opName(Op op);

    UnaryExpr(Op op, std::unique_ptr<Expr> operand)
        : Expr(Kind::UnaryExpr), O(op), Operand(std::move(operand)) {}

    Op getOp() const { return O; }
    Expr *getOperand() const { return Operand.get(); }

private:
    Op O;
    std::unique_ptr<Expr> Operand;
};

class CallExpr : public Expr {
public:
    CallExpr(std::string callee, std::vector<std::unique_ptr<Expr>> args)
        : Expr(Kind::CallExpr), Callee(std::move(callee)), Args(std::move(args)) {}

    const std::string &getCallee() const { return Callee; }
    const std::vector<std::unique_ptr<Expr>> &getArgs() const { return Args; }

private:
    std::string Callee;
    std::vector<std::unique_ptr<Expr>> Args;
};

class IntegerLiteral : public Expr {
public:
    IntegerLiteral(int16_t val) : Expr(Kind::IntegerLiteral), Value(val) {}
    int16_t getValue() const { return Value; }
private:
    int16_t Value;
};

class CharLiteral : public Expr {
public:
    CharLiteral(char val) : Expr(Kind::CharLiteral), Value(val) {}
    char getValue() const { return Value; }
private:
    char Value;
};

class BoolLiteral : public Expr {
public:
    BoolLiteral(bool val) : Expr(Kind::BoolLiteral), Value(val) {}
    bool getValue() const { return Value; }
private:
    bool Value;
};

class VarExpr : public Expr {
public:
    VarExpr(std::string name) : Expr(Kind::VarExpr), Name(std::move(name)) {}
    const std::string &getName() const { return Name; }

    // Set after semantic analysis
    void setDecl(class Decl *d) { DeclRef = d; }
    Decl *getDecl() const { return DeclRef; }

private:
    std::string Name;
    Decl *DeclRef = nullptr;
};

// ====== Array subscript: a[i] ======
class ArraySubscriptExpr : public Expr {
public:
    ArraySubscriptExpr(std::string name, std::unique_ptr<Expr> index)
        : Expr(Kind::ArraySubscriptExpr), Name(std::move(name)), Index(std::move(index)) {}

    const std::string &getName() const { return Name; }
    Expr *getIndex() const { return Index.get(); }

private:
    std::string Name;
    std::unique_ptr<Expr> Index;
};

// ====== I/O nodes ======
class PrintStmt : public Stmt {
public:
    PrintStmt(std::unique_ptr<Expr> value)
        : Stmt(Kind::PrintStmt), Value(std::move(value)) {}

    Expr *getValue() const { return Value.get(); }

private:
    std::unique_ptr<Expr> Value;
};

class ReadExpr : public Expr {
public:
    ReadExpr() : Expr(Kind::ReadExpr) {}
};

} // namespace ll1

#endif
