#include <ll1/IRGen/IRGen.h>
#include <ll1/Parse/Parser.h>

namespace ll1 {

IRGen::IRGen() {
    M = std::make_unique<Module>("ll1_module");
    Builder = std::make_unique<IRBuilder>(Ctx);
}

Type *IRGen::mapType(BuiltinType t) {
    switch (t) {
    case BuiltinType::Void: return Ctx.getVoidTy();
    case BuiltinType::Int:  return Ctx.getInt16Ty();
    case BuiltinType::Char: return Ctx.getInt8Ty();
    case BuiltinType::Bool: return Ctx.getInt1Ty();
    }
    return Ctx.getVoidTy();
}

AllocaInst *IRGen::createEntryBlockAlloca(Type *ty, const std::string &name) {
    // Insert at beginning of entry block
    auto *entry = CurrentFunc->getEntryBlock();
    auto oldBB = Builder->getInsertBlock();
    Builder->setInsertPoint(entry);
    // If entry already has instructions, insert before first non-alloca
    auto *alloca = Builder->CreateAlloca(ty, name);
    Builder->setInsertPoint(oldBB);
    return alloca;
}

std::unique_ptr<Module> IRGen::generate(Program &prog) {
    // Collect all function declarations first (for forward references)
    for (auto &node : prog.Declarations) {
        if (auto *func = dynamic_cast<FuncDecl*>(node.get())) {
            genFuncDecl(*func);
        }
    }

    // Generate function bodies
    for (auto &node : prog.Declarations) {
        if (auto *func = dynamic_cast<FuncDecl*>(node.get())) {
            CurrentFunc = M->getFunction(func->getName());
            NamedValues.clear();

            // Create entry block
            auto *entry = CurrentFunc->createBasicBlock("entry");
            Builder->setInsertPoint(entry);

            // Alloca for parameters
            for (unsigned i = 0; i < func->getParams().size(); ++i) {
                auto &p = func->getParams()[i];
                Type *pty = mapType(p.Ty);
                auto *alloca = Builder->CreateAlloca(pty, p.Name);
                NamedValues[p.Name] = alloca;
                // Store argument value
                auto *arg = CurrentFunc->getArg(i);
                Builder->CreateStore(arg, alloca);
            }

            // Generate body
            if (auto *body = func->getBody()) {
                genStmt(*body);
            }

            // Ensure terminator on the current insert block
            auto *curBB = Builder->getInsertBlock();
            if (!curBB->getTerminator() || curBB->getTerminator()->getOpcode() != Instruction::Opcode::Ret) {
                if (func->getReturnType() == BuiltinType::Void) {
                    Builder->CreateRetVoid();
                } else {
                    Builder->CreateRet(Builder->getInt16(0));
                }
            }
        }
    }

    // Handle global VarDecls (generate in a synthetic __init function later, or skip for MVP)
    // For MVP, only function bodies matter

    return std::move(M);
}

void IRGen::genFuncDecl(FuncDecl &decl) {
    Type *retTy = mapType(decl.getReturnType());
    auto *fn = M->createFunction(retTy, decl.getName());

    // Add arguments
    for (auto &p : decl.getParams()) {
        Type *pty = mapType(p.Ty);
        fn->addArg(std::make_unique<Argument>(pty, p.Name));
    }
}

// ====== Statements ======
void IRGen::genStmt(Stmt &stmt) {
    switch (stmt.getKind()) {
    case ASTNode::Kind::Block:      genBlock(static_cast<Block&>(stmt)); break;
    case ASTNode::Kind::IfStmt:     genIfStmt(static_cast<IfStmt&>(stmt)); break;
    case ASTNode::Kind::WhileStmt:  genWhileStmt(static_cast<WhileStmt&>(stmt)); break;
    case ASTNode::Kind::ReturnStmt: genReturnStmt(static_cast<ReturnStmt&>(stmt)); break;
    case ASTNode::Kind::VarDecl:    genVarDecl(static_cast<VarDecl&>(stmt)); break;
    case ASTNode::Kind::PrintStmt:  genPrintStmt(static_cast<PrintStmt&>(stmt)); break;
    case ASTNode::Kind::ForStmt:
    case ASTNode::Kind::BreakStmt:
    case ASTNode::Kind::ContinueStmt:
        // Not implemented in MVP
        break;
    default:
        // Expression statement
        if (auto *expr = dynamic_cast<Expr*>(&stmt)) {
            genExpr(*expr);
        }
        break;
    }
}

void IRGen::genBlock(Block &block) {
    for (auto &s : block.getStatements()) {
        genStmt(*s);
    }
}

void IRGen::genIfStmt(IfStmt &stmt) {
    auto *condVal = genExpr(*stmt.getCond());

    auto *thenBB = CurrentFunc->createBasicBlock("then");
    auto *elseBB = stmt.hasElse() ? CurrentFunc->createBasicBlock("else") : nullptr;
    auto *mergeBB = CurrentFunc->createBasicBlock("if_end");

    // Convert condition to i1 if needed
    Builder->CreateCondBr(condVal, thenBB, elseBB ? elseBB : mergeBB);

    // Then block
    Builder->setInsertPoint(thenBB);
    genStmt(*stmt.getThen());
    if (!Builder->getInsertBlock()->getTerminator()) {
        Builder->CreateBr(mergeBB);
    }

    // Else block
    if (elseBB) {
        Builder->setInsertPoint(elseBB);
        genStmt(*stmt.getElse());
        if (!Builder->getInsertBlock()->getTerminator()) {
            Builder->CreateBr(mergeBB);
        }
    }

    Builder->setInsertPoint(mergeBB);
}

void IRGen::genWhileStmt(WhileStmt &stmt) {
    auto *condBB = CurrentFunc->createBasicBlock("while_cond");
    auto *bodyBB = CurrentFunc->createBasicBlock("while_body");
    auto *exitBB = CurrentFunc->createBasicBlock("while_exit");

    // Branch to condition
    Builder->CreateBr(condBB);

    // Condition block
    Builder->setInsertPoint(condBB);
    auto *condVal = genExpr(*stmt.getCond());
    Builder->CreateCondBr(condVal, bodyBB, exitBB);

    // Body block
    Builder->setInsertPoint(bodyBB);
    genStmt(*stmt.getBody());
    if (!Builder->getInsertBlock()->getTerminator()) {
        Builder->CreateBr(condBB);
    }

    Builder->setInsertPoint(exitBB);
}

void IRGen::genReturnStmt(ReturnStmt &stmt) {
    if (stmt.hasValue()) {
        auto *val = genExpr(*stmt.getValue());
        Builder->CreateRet(val);
    } else {
        Builder->CreateRetVoid();
    }
}

void IRGen::genVarDecl(VarDecl &decl) {
    Type *ty = mapType(decl.getType());

    if (decl.isArray()) {
        auto *alloca = Builder->CreateAlloca(ty, decl.getName());
        alloca->setArraySize(decl.getArraySize());
        NamedValues[decl.getName()] = alloca;
        return;
    }

    auto *alloca = Builder->CreateAlloca(ty, decl.getName());
    NamedValues[decl.getName()] = alloca;

    if (decl.hasInit()) {
        auto *initVal = genExpr(*decl.getInit());
        if (initVal->getType() != ty) {
            initVal = Builder->CreateSExt(initVal, ty, decl.getName() + "_ext");
        }
        Builder->CreateStore(initVal, alloca);
    }
}

void IRGen::genPrintStmt(PrintStmt &stmt) {
    auto *val = genExpr(*stmt.getValue());
    // Generate call to __print(val)
    // Look up or create __print function
    auto *printFn = M->getFunction("__print");
    if (!printFn) {
        printFn = M->createFunction(Ctx.getVoidTy(), "__print");
        printFn->addArg(std::make_unique<Argument>(Ctx.getInt16Ty(), "val"));
    }
    Builder->CreateCall(printFn, {val});
}

// ====== Expressions ======
Value *IRGen::genExpr(Expr &expr) {
    switch (expr.getKind()) {
    case ASTNode::Kind::BinaryExpr:     return genBinaryExpr(static_cast<BinaryExpr&>(expr));
    case ASTNode::Kind::UnaryExpr:      return genUnaryExpr(static_cast<UnaryExpr&>(expr));
    case ASTNode::Kind::VarExpr:        return genVarExpr(static_cast<VarExpr&>(expr));
    case ASTNode::Kind::ArraySubscriptExpr: return genArraySubscript(static_cast<ArraySubscriptExpr&>(expr));
    case ASTNode::Kind::ReadExpr:       return genReadExpr();
    case ASTNode::Kind::IntegerLiteral: return genIntegerLiteral(static_cast<IntegerLiteral&>(expr));
    case ASTNode::Kind::BoolLiteral:    return genBoolLiteral(static_cast<BoolLiteral&>(expr));
    default: return Builder->getInt16(0);
    }
}

Value *IRGen::genBinaryExpr(BinaryExpr &expr) {
    auto op = expr.getOp();

    // Assignment: store RHS to LHS
    if (op == BinaryExpr::Op::Assign) {
        // Scalar: x = val
        if (auto *var = dynamic_cast<VarExpr*>(expr.getLHS())) {
            auto *alloca = NamedValues[var->getName()];
            if (!alloca) return Builder->getInt16(0);
            auto *rhs = genExpr(*expr.getRHS());
            Builder->CreateStore(rhs, alloca);
            return rhs;
        }
        // Array: a[i] = val
        if (auto *arr = dynamic_cast<ArraySubscriptExpr*>(expr.getLHS())) {
            auto *base = NamedValues[arr->getName()];
            if (!base) return Builder->getInt16(0);
            auto *index = genExpr(*arr->getIndex());
            auto *rhs = genExpr(*expr.getRHS());
            Builder->CreateArrayStore(rhs, base, index);
            return rhs;
        }
        return Builder->getInt16(0);
    }

    // Logical operators: short-circuit evaluation (simplified for MVP)
    if (op == BinaryExpr::Op::And) {
        auto *lhs = genExpr(*expr.getLHS());
        auto *rhs = genExpr(*expr.getRHS());
        return Builder->CreateAnd(lhs, rhs, "and_tmp");
    }
    if (op == BinaryExpr::Op::Or) {
        auto *lhs = genExpr(*expr.getLHS());
        auto *rhs = genExpr(*expr.getRHS());
        return Builder->CreateOr(lhs, rhs, "or_tmp");
    }

    Value *lhs = genExpr(*expr.getLHS());
    Value *rhs = genExpr(*expr.getRHS());

    switch (op) {
    case BinaryExpr::Op::Add: return Builder->CreateAdd(lhs, rhs, "add_tmp");
    case BinaryExpr::Op::Sub: return Builder->CreateSub(lhs, rhs, "sub_tmp");
    case BinaryExpr::Op::Mul: return Builder->CreateMul(lhs, rhs, "mul_tmp");
    case BinaryExpr::Op::Div: return Builder->CreateSDiv(lhs, rhs, "div_tmp");
    case BinaryExpr::Op::Rem: return Builder->CreateSRem(lhs, rhs, "rem_tmp");
    case BinaryExpr::Op::Eq:  return Builder->CreateICmpEQ(lhs, rhs, "eq_tmp");
    case BinaryExpr::Op::Ne:  return Builder->CreateICmpNE(lhs, rhs, "ne_tmp");
    case BinaryExpr::Op::Lt:  return Builder->CreateICmpSLT(lhs, rhs, "lt_tmp");
    case BinaryExpr::Op::Gt:  return Builder->CreateICmpSGT(lhs, rhs, "gt_tmp");
    case BinaryExpr::Op::Le:  return Builder->CreateICmpSLE(lhs, rhs, "le_tmp");
    case BinaryExpr::Op::Ge:  return Builder->CreateICmpSGE(lhs, rhs, "ge_tmp");
    default: return Builder->getInt16(0);
    }
}

Value *IRGen::genUnaryExpr(UnaryExpr &expr) {
    auto *operand = genExpr(*expr.getOperand());
    if (expr.getOp() == UnaryExpr::Op::Neg) {
        return Builder->CreateNeg(operand, "neg_tmp");
    }
    if (expr.getOp() == UnaryExpr::Op::Not) {
        return Builder->CreateNot(operand, "not_tmp");
    }
    return operand;
}

Value *IRGen::genVarExpr(VarExpr &expr) {
    auto *alloca = NamedValues[expr.getName()];
    if (!alloca) return Builder->getInt16(0);
    return Builder->CreateLoad(alloca->getAllocatedType(), alloca, expr.getName());
}

Value *IRGen::genIntegerLiteral(IntegerLiteral &lit) {
    return Builder->getInt16(lit.getValue());
}

Value *IRGen::genBoolLiteral(BoolLiteral &lit) {
    return Builder->getInt16(lit.getValue() ? 1 : 0);
}

Value *IRGen::genArraySubscript(ArraySubscriptExpr &expr) {
    auto *baseAlloca = NamedValues[expr.getName()];
    if (!baseAlloca) return Builder->getInt16(0);

    auto *indexVal = genExpr(*expr.getIndex());
    // ArrayLoad: loads baseAlloca[indexVal]
    return Builder->CreateArrayLoad(Ctx.getInt16Ty(), baseAlloca, indexVal, "arr_val");
}

Value *IRGen::genReadExpr() {
    auto *readFn = M->getFunction("__read");
    if (!readFn) {
        readFn = M->createFunction(Ctx.getInt16Ty(), "__read");
    }
    return Builder->CreateCall(readFn, {}, "read_val");
}

} // namespace ll1
