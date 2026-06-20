#ifndef LL1_SEMA_SEMA_H
#define LL1_SEMA_SEMA_H

#include <ll1/Sema/Scope.h>
#include <ll1/AST/AST.h>
#include <string>
#include <vector>

namespace ll1 {

class Program;

struct Diagnostic {
    enum Level { Note, Warning, Error };
    Level Lvl;
    SourceLocation Loc;
    std::string Message;
};

class DiagnosticEngine {
public:
    void report(SourceLocation loc, Diagnostic::Level level, const std::string &msg);
    bool hasErrors() const { return ErrorCount > 0; }
    unsigned getErrorCount() const { return ErrorCount; }
    const std::vector<Diagnostic> &getDiagnostics() const { return Diags; }
    void printAll();
private:
    std::vector<Diagnostic> Diags;
    unsigned ErrorCount = 0;
};

class Sema {
public:
    explicit Sema(DiagnosticEngine &diag);

    bool analyze(Program &prog);

    SymbolTable &getSymbolTable() { return SymTable; }

private:
    SymbolTable SymTable;
    DiagnosticEngine &Diags;

    BuiltinType currentFuncReturnType = BuiltinType::Void;
    bool inLoop = false;
    bool inFunction = false;

    // Pass 1: collect declarations
    void collectDecls(Program &prog);

    // Pass 2: check bodies
    void checkFuncBody(FuncDecl &func);
    void checkStmt(Stmt &stmt);
    void checkBlock(Block &block);
    void checkIfStmt(IfStmt &stmt);
    void checkWhileStmt(WhileStmt &stmt);
    void checkForStmt(ForStmt &stmt);
    void checkReturnStmt(ReturnStmt &stmt);
    void checkVarDecl(VarDecl &decl);
    void checkPrintStmt(PrintStmt &stmt);

    // Expression type checking
    BuiltinType checkExpr(Expr &expr);
    BuiltinType checkBinaryExpr(BinaryExpr &expr);
    BuiltinType checkUnaryExpr(UnaryExpr &expr);
    BuiltinType checkCallExpr(CallExpr &expr);
    BuiltinType checkVarExpr(VarExpr &expr);
    BuiltinType checkIntegerLiteral(IntegerLiteral &lit);
    BuiltinType checkBoolLiteral(BoolLiteral &lit);
    BuiltinType checkArraySubscript(ArraySubscriptExpr &expr);

    // Type compatibility
    bool isCompatible(BuiltinType from, BuiltinType to) const;
    const char *typeName(BuiltinType t) const;
};

} // namespace ll1
#endif
