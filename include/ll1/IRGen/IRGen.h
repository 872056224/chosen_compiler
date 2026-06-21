#ifndef LL1_IRGEN_IRGEN_H
#define LL1_IRGEN_IRGEN_H

#include <ll1/IR/Module.h>
#include <ll1/IR/IRBuilder.h>
#include <ll1/IR/LLVMContext.h>
#include <ll1/AST/AST.h>
#include <map>
#include <memory>

namespace ll1 {

class Program;
class Sema;

class IRGen {
public:
    IRGen();

    std::unique_ptr<Module> generate(Program &prog);

private:
    LLVMContext Ctx;
    std::unique_ptr<Module> M;
    std::unique_ptr<IRBuilder> Builder;
    Function *CurrentFunc = nullptr;

    // Map variable name to alloca instruction
    std::map<std::string, AllocaInst*> NamedValues;

    // Loop context for break/continue
    struct LoopContext { BasicBlock *CondBB; BasicBlock *ExitBB; };
    std::vector<LoopContext> LoopStack;

    Type *mapType(BuiltinType t);

    void genFuncDecl(FuncDecl &decl);
    void genStmt(Stmt &stmt);
    void genBlock(Block &block);
    void genIfStmt(IfStmt &stmt);
    void genWhileStmt(WhileStmt &stmt);
    void genForStmt(ForStmt &stmt);
    void genBreakStmt();
    void genContinueStmt();
    void genReturnStmt(ReturnStmt &stmt);
    void genVarDecl(VarDecl &decl);
    void genPrintStmt(PrintStmt &stmt);

    Value *genExpr(Expr &expr);
    Value *genBinaryExpr(BinaryExpr &expr);
    Value *genUnaryExpr(UnaryExpr &expr);
    Value *genVarExpr(VarExpr &expr);
    Value *genArraySubscript(ArraySubscriptExpr &expr);
    Value *genReadExpr();
    Value *genIntegerLiteral(IntegerLiteral &lit);
    Value *genBoolLiteral(BoolLiteral &lit);

    // Create alloca in entry block for local vars
    AllocaInst *createEntryBlockAlloca(Type *ty, const std::string &name);
};

} // namespace ll1
#endif
