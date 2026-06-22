#include <ll1/Sema/Sema.h>
#include <ll1/Parse/Parser.h>
#include <cstddef>
#include <iostream>

namespace ll1 {

// ====== DiagnosticEngine ======
void DiagnosticEngine::report(SourceLocation loc, Diagnostic::Level level, const std::string &msg) {
    Diags.push_back({level, loc, msg});
    if (level == Diagnostic::Error) ErrorCount++;
}

void DiagnosticEngine::printAll() {
    for (auto &d : Diags) {
        const char *lvl = "note";
        if (d.Lvl == Diagnostic::Warning) lvl = "warning";
        if (d.Lvl == Diagnostic::Error) lvl = "error";
        std::cerr << "line " << d.Loc.Line << ": " << lvl << ": " << d.Message << "\n";
    }
}

// ====== Sema ======
Sema::Sema(DiagnosticEngine &diag) : Diags(diag) {}

bool Sema::analyze(Program &prog) {
    collectDecls(prog);

    // Pass 2: check function bodies
    for (auto &node : prog.Declarations) {
        if (auto *func = dynamic_cast<FuncDecl*>(node.get())) {
            checkFuncBody(*func);
        }
        // Global VarDecls don't need body checking (init expr already in Pass 1 scope)
    }

    return !Diags.hasErrors();
}

void Sema::collectDecls(Program &prog) {
    for (auto &node : prog.Declarations) {
        if (auto *func = dynamic_cast<FuncDecl*>(node.get())) {
            Symbol sym;
            sym.Name = func->getName();
            sym.Ty = func->getReturnType();
            sym.Node = func;
            sym.IsFunction = true;
            if (!SymTable.declare(sym)) {
                Diags.report(func->getLoc(), Diagnostic::Error,
                    "redefinition of function '" + func->getName() + "'");
            }
        } else if (auto *var = dynamic_cast<VarDecl*>(node.get())) {
            Symbol sym;
            sym.Name = var->getName();
            sym.Ty = var->getType();
            sym.Node = var;
            if (!SymTable.declare(sym)) {
                Diags.report(var->getLoc(), Diagnostic::Error,
                    "redefinition of global variable '" + var->getName() + "'");
            }
            // Check initializer
            if (var->hasInit()) {
                checkExpr(*var->getInit());
            }
        }
    }
}

void Sema::checkFuncBody(FuncDecl &func) {
    inFunction = true;
    currentFuncReturnType = func.getReturnType();

    SymTable.enterScope(Scope::ScopeKind::Function);

    // Declare parameters
    for (auto &p : func.getParams()) {
        Symbol sym;
        sym.Name = p.Name;
        sym.Ty = p.Ty;
        sym.IsParameter = true;
        if (!SymTable.declare(sym)) {
            Diags.report(func.getLoc(), Diagnostic::Error,
                "duplicate parameter '" + p.Name + "' in function '" + func.getName() + "'");
        }
    }

    // Check body
    if (auto *body = func.getBody()) {
        checkStmt(*body);
    }

    SymTable.exitScope();
    inFunction = false;
}

void Sema::checkStmt(Stmt &stmt) {
    switch (stmt.getKind()) {
    case ASTNode::Kind::Block:
        checkBlock(static_cast<Block&>(stmt)); break;
    case ASTNode::Kind::IfStmt:
        checkIfStmt(static_cast<IfStmt&>(stmt)); break;
    case ASTNode::Kind::WhileStmt:
        checkWhileStmt(static_cast<WhileStmt&>(stmt)); break;
    case ASTNode::Kind::ForStmt:
        checkForStmt(static_cast<ForStmt&>(stmt)); break;
    case ASTNode::Kind::ReturnStmt:
        checkReturnStmt(static_cast<ReturnStmt&>(stmt)); break;
    case ASTNode::Kind::VarDecl:
        checkVarDecl(static_cast<VarDecl&>(stmt)); break;
    case ASTNode::Kind::PrintStmt:
        checkPrintStmt(static_cast<PrintStmt&>(stmt)); break;
    case ASTNode::Kind::BreakStmt:
        if (!inLoop) Diags.report(stmt.getLoc(), Diagnostic::Error, "'break' outside of loop");
        break;
    case ASTNode::Kind::ContinueStmt:
        if (!inLoop) Diags.report(stmt.getLoc(), Diagnostic::Error, "'continue' outside of loop");
        break;
    default:
        // Expression statements
        if (auto *expr = dynamic_cast<Expr*>(&stmt)) {
            checkExpr(*expr);
        }
        break;
    }
}

void Sema::checkBlock(Block &block) {
    SymTable.enterScope(Scope::ScopeKind::Block);
    for (auto &s : block.getStatements()) {
        checkStmt(*s);
    }
    SymTable.exitScope();
}

void Sema::checkIfStmt(IfStmt &stmt) {
    BuiltinType condTy = checkExpr(*stmt.getCond());
    if (condTy != BuiltinType::Bool) {
        Diags.report(stmt.getCond()->getLoc(), Diagnostic::Error,
            "if condition must be boolean, got " + std::string(typeName(condTy)));
    }
    checkStmt(*stmt.getThen());
    if (stmt.hasElse()) {
        checkStmt(*stmt.getElse());
    }
}

void Sema::checkWhileStmt(WhileStmt &stmt) {
    BuiltinType condTy = checkExpr(*stmt.getCond());
    if (condTy != BuiltinType::Bool) {
        Diags.report(stmt.getCond()->getLoc(), Diagnostic::Error,
            "while condition must be boolean, got " + std::string(typeName(condTy)));
    }
    bool prevInLoop = inLoop;
    inLoop = true;
    checkStmt(*stmt.getBody());
    inLoop = prevInLoop;
}

void Sema::checkForStmt(ForStmt &stmt) {
    SymTable.enterScope(Scope::ScopeKind::Block);
    if (stmt.getInit()) checkStmt(*stmt.getInit());
    if (stmt.getCond()) {
        BuiltinType condTy = checkExpr(*stmt.getCond());
        if (condTy != BuiltinType::Bool) {
            Diags.report(stmt.getCond()->getLoc(), Diagnostic::Error,
                "for condition must be boolean");
        }
    }
    if (stmt.getIncr()) checkExpr(*stmt.getIncr());
    bool prevInLoop = inLoop;
    inLoop = true;
    checkStmt(*stmt.getBody());
    inLoop = prevInLoop;
    SymTable.exitScope();
}

void Sema::checkReturnStmt(ReturnStmt &stmt) {
    if (!inFunction) {
        Diags.report(stmt.getLoc(), Diagnostic::Error, "'return' outside of function");
        return;
    }
    if (stmt.hasValue()) {
        BuiltinType retTy = checkExpr(*stmt.getValue());
        if (!isCompatible(retTy, currentFuncReturnType)) {
            Diags.report(stmt.getLoc(), Diagnostic::Error,
                "return type mismatch: expected " + std::string(typeName(currentFuncReturnType)) +
                " but got " + std::string(typeName(retTy)));
        }
    } else {
        if (currentFuncReturnType != BuiltinType::Void) {
            Diags.report(stmt.getLoc(), Diagnostic::Error,
                "non-void function must return a value");
        }
    }
}

void Sema::checkVarDecl(VarDecl &decl) {
    Symbol sym;
    sym.Name = decl.getName();
    sym.Ty = decl.getType();
    sym.Node = &decl;
    sym.IsArray = decl.isArray();
    sym.ArraySize = decl.getArraySize();
    if (decl.isArray() && decl.getArraySize() <= 0) {
        Diags.report(decl.getLoc(), Diagnostic::Error,
            "array size must be positive");
    }
    if (!SymTable.declare(sym)) {
        Diags.report(decl.getLoc(), Diagnostic::Error,
            "redefinition of '" + decl.getName() + "'");
        return;
    }
    if (decl.hasInit() && decl.isArray()) {
        Diags.report(decl.getLoc(), Diagnostic::Error,
            "array '" + decl.getName() + "' cannot have initializer");
    } else if (decl.hasInit()) {
        BuiltinType initTy = checkExpr(*decl.getInit());
        if (!isCompatible(initTy, decl.getType())) {
            Diags.report(decl.getLoc(), Diagnostic::Error,
                "type mismatch in initialization of '" + decl.getName() +
                "': expected " + typeName(decl.getType()) +
                " but got " + typeName(initTy));
        }
    }
}

void Sema::checkPrintStmt(PrintStmt &stmt) {
    BuiltinType valTy = checkExpr(*stmt.getValue());
    if (valTy != BuiltinType::Int && valTy != BuiltinType::Char) {
        Diags.report(stmt.getLoc(), Diagnostic::Error,
            "print requires int or char argument");
    }
}

BuiltinType Sema::checkArraySubscript(ArraySubscriptExpr &expr) {
    const Symbol *sym = SymTable.lookup(expr.getName());
    if (!sym) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "use of undeclared array '" + expr.getName() + "'");
        return BuiltinType::Int;
    }
    if (!sym->IsArray) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "'" + expr.getName() + "' is not an array");
        return BuiltinType::Int;
    }
    BuiltinType indexTy = checkExpr(*expr.getIndex());
    if (indexTy != BuiltinType::Int) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "array index must be int");
    }
    return sym->Ty;
}

// ====== Expression type checking ======
BuiltinType Sema::checkExpr(Expr &expr) {
    switch (expr.getKind()) {
    case ASTNode::Kind::BinaryExpr: return checkBinaryExpr(static_cast<BinaryExpr&>(expr));
    case ASTNode::Kind::UnaryExpr:  return checkUnaryExpr(static_cast<UnaryExpr&>(expr));
    case ASTNode::Kind::CallExpr:   return checkCallExpr(static_cast<CallExpr&>(expr));
    case ASTNode::Kind::VarExpr:    return checkVarExpr(static_cast<VarExpr&>(expr));
    case ASTNode::Kind::IntegerLiteral: return checkIntegerLiteral(static_cast<IntegerLiteral&>(expr));
    case ASTNode::Kind::BoolLiteral: return checkBoolLiteral(static_cast<BoolLiteral&>(expr));
    case ASTNode::Kind::CharLiteral: return BuiltinType::Char;
    case ASTNode::Kind::ArraySubscriptExpr: return checkArraySubscript(static_cast<ArraySubscriptExpr&>(expr));
    case ASTNode::Kind::ReadExpr: return BuiltinType::Int;
    default: return BuiltinType::Void;
    }
}

BuiltinType Sema::checkBinaryExpr(BinaryExpr &expr) {
    BuiltinType lhsTy = checkExpr(*expr.getLHS());
    BuiltinType rhsTy = checkExpr(*expr.getRHS());

    auto op = expr.getOp();

    // Comparison operators -> Bool result
    if (op == BinaryExpr::Op::Lt || op == BinaryExpr::Op::Gt ||
        op == BinaryExpr::Op::Le || op == BinaryExpr::Op::Ge ||
        op == BinaryExpr::Op::Eq || op == BinaryExpr::Op::Ne) {
        if (lhsTy != rhsTy) {
            Diags.report(expr.getLoc(), Diagnostic::Warning,
                "comparison between " + std::string(typeName(lhsTy)) +
                " and " + std::string(typeName(rhsTy)));
        }
        return BuiltinType::Bool;
    }

    // Logical operators -> Bool result, Bool operands
    if (op == BinaryExpr::Op::And || op == BinaryExpr::Op::Or) {
        if (lhsTy != BuiltinType::Bool || rhsTy != BuiltinType::Bool) {
            Diags.report(expr.getLoc(), Diagnostic::Error,
                "logical operator requires bool operands");
        }
        return BuiltinType::Bool;
    }

    // Arithmetic operators
    if (lhsTy != rhsTy) {
        Diags.report(expr.getLoc(), Diagnostic::Warning,
            "operand type mismatch in binary expression");
    }
    return lhsTy; // Result type same as operands
}

BuiltinType Sema::checkUnaryExpr(UnaryExpr &expr) {
    BuiltinType operandTy = checkExpr(*expr.getOperand());
    if (expr.getOp() == UnaryExpr::Op::Not) {
        if (operandTy != BuiltinType::Bool) {
            Diags.report(expr.getLoc(), Diagnostic::Error,
                "logical not requires bool operand");
        }
        return BuiltinType::Bool;
    }
    // Negation - numeric
    return operandTy;
}

BuiltinType Sema::checkCallExpr(CallExpr &expr) {
    const Symbol *sym = SymTable.lookup(expr.getCallee());
    if (!sym) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "undefined function '" + expr.getCallee() + "'");
        return BuiltinType::Void;
    }
    if (!sym->IsFunction) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "'" + expr.getCallee() + "' is not a function");
        return BuiltinType::Void;
    }
    auto *funcDecl = static_cast<const FuncDecl*>(sym->Node);
    if (expr.getArgs().size() != funcDecl->getParams().size()) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "function '" + expr.getCallee() + "' expects " +
            std::to_string(funcDecl->getParams().size()) + " arguments, got " +
            std::to_string(expr.getArgs().size()));
    }
    // Check arg types
    for (size_t i = 0; i < expr.getArgs().size() && i < funcDecl->getParams().size(); ++i) {
        BuiltinType argTy = checkExpr(*expr.getArgs()[i]);
        BuiltinType paramTy = funcDecl->getParams()[i].Ty;
        if (!isCompatible(argTy, paramTy)) {
            Diags.report(expr.getLoc(), Diagnostic::Error,
                "argument " + std::to_string(i+1) + " type mismatch in call to '" +
                expr.getCallee() + "': expected " + typeName(paramTy) +
                " but got " + typeName(argTy));
        }
    }
    return sym->Ty; // Return type
}

BuiltinType Sema::checkVarExpr(VarExpr &expr) {
    const Symbol *sym = SymTable.lookup(expr.getName());
    if (!sym) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "use of undeclared identifier '" + expr.getName() + "'");
        return BuiltinType::Int; // Error recovery
    }
    if (sym->IsFunction) {
        Diags.report(expr.getLoc(), Diagnostic::Error,
            "function '" + expr.getName() + "' used as variable");
        return BuiltinType::Int;
    }
    // Set DeclRef for IRGen (only if the node derives from Decl)
    if (dynamic_cast<const Decl*>(sym->Node)) {
        expr.setDecl(const_cast<Decl*>(static_cast<const Decl*>(sym->Node)));
    }
    return sym->Ty;
}

BuiltinType Sema::checkIntegerLiteral(IntegerLiteral &lit) {
    (void)lit;
    return BuiltinType::Int;
}

BuiltinType Sema::checkBoolLiteral(BoolLiteral &lit) {
    (void)lit;
    return BuiltinType::Bool;
}

bool Sema::isCompatible(BuiltinType from, BuiltinType to) const {
    if (from == to) return true;
    if (from == BuiltinType::Int && to == BuiltinType::Char) return true; // int → char truncation
    if (to == BuiltinType::Int && from == BuiltinType::Char) return true; // char → int promotion
    if (to == BuiltinType::Int && from == BuiltinType::Bool) return true; // bool → int
    return false;
}

const char *Sema::typeName(BuiltinType t) const {
    return builtinTypeName(t);
}

} // namespace ll1
