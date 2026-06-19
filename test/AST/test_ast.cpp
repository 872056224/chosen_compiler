#include <ll1/AST/AST.h>
#include <cassert>
#include <iostream>

int main() {
    // Test IntegerLiteral
    auto lit = std::make_unique<ll1::IntegerLiteral>(42);
    assert(lit->getKind() == ll1::ASTNode::Kind::IntegerLiteral);
    assert(lit->getValue() == 42);

    // Test VarExpr
    auto var = std::make_unique<ll1::VarExpr>("x");
    assert(var->getKind() == ll1::ASTNode::Kind::VarExpr);
    assert(var->getName() == "x");

    // Test BinaryExpr: x + 10
    auto bin = std::make_unique<ll1::BinaryExpr>(
        ll1::BinaryExpr::Op::Add,
        std::make_unique<ll1::VarExpr>("x"),
        std::make_unique<ll1::IntegerLiteral>(10));
    assert(bin->getOp() == ll1::BinaryExpr::Op::Add);
    assert(dynamic_cast<ll1::VarExpr*>(bin->getLHS()));
    assert(dynamic_cast<ll1::IntegerLiteral*>(bin->getRHS())->getValue() == 10);

    // Test VarDecl: int a = 5;
    auto init = std::make_unique<ll1::IntegerLiteral>(5);
    auto varDecl = std::make_unique<ll1::VarDecl>(
        ll1::BuiltinType::Int, "a", std::move(init));
    assert(varDecl->getKind() == ll1::ASTNode::Kind::VarDecl);
    assert(varDecl->getName() == "a");
    assert(varDecl->getType() == ll1::BuiltinType::Int);
    assert(varDecl->hasInit());
    assert(varDecl->getInit()->getKind() == ll1::ASTNode::Kind::IntegerLiteral);

    // Test IfStmt
    auto cond = std::make_unique<ll1::BinaryExpr>(
        ll1::BinaryExpr::Op::Lt,
        std::make_unique<ll1::VarExpr>("a"),
        std::make_unique<ll1::IntegerLiteral>(10));
    auto thenBody = std::make_unique<ll1::Block>();
    auto ifStmt = std::make_unique<ll1::IfStmt>(std::move(cond), std::move(thenBody));
    assert(ifStmt->getKind() == ll1::ASTNode::Kind::IfStmt);
    assert(!ifStmt->hasElse());

    // Test full If-Else chain
    auto elseBody = std::make_unique<ll1::Block>();
    auto ifElse = std::make_unique<ll1::IfStmt>(
        std::make_unique<ll1::BinaryExpr>(ll1::BinaryExpr::Op::Gt,
            std::make_unique<ll1::VarExpr>("b"),
            std::make_unique<ll1::IntegerLiteral>(0)),
        std::make_unique<ll1::Block>(),
        std::move(elseBody));
    assert(ifElse->hasElse());

    // Test WhileStmt
    auto whileStmt = std::make_unique<ll1::WhileStmt>(
        std::make_unique<ll1::BoolLiteral>(true),
        std::make_unique<ll1::Block>());
    assert(whileStmt->getKind() == ll1::ASTNode::Kind::WhileStmt);

    // Test ReturnStmt
    auto ret = std::make_unique<ll1::ReturnStmt>(std::make_unique<ll1::IntegerLiteral>(0));
    assert(ret->getKind() == ll1::ASTNode::Kind::ReturnStmt);
    assert(ret->hasValue());

    // Test FuncDecl: fn int add(int a, int b) { return a + b; }
    ll1::FuncDecl::Param p1{ll1::BuiltinType::Int, "a"};
    ll1::FuncDecl::Param p2{ll1::BuiltinType::Int, "b"};
    auto body = std::make_unique<ll1::Block>();
    auto *bb = static_cast<ll1::Block*>(body.get());
    bb->addStmt(std::make_unique<ll1::ReturnStmt>(
        std::make_unique<ll1::BinaryExpr>(ll1::BinaryExpr::Op::Add,
            std::make_unique<ll1::VarExpr>("a"),
            std::make_unique<ll1::VarExpr>("b"))));
    auto func = std::make_unique<ll1::FuncDecl>(
        ll1::BuiltinType::Int, "add",
        std::vector<ll1::FuncDecl::Param>{p1, p2},
        std::move(body));
    assert(func->getKind() == ll1::ASTNode::Kind::FuncDecl);
    assert(func->getName() == "add");
    assert(func->getReturnType() == ll1::BuiltinType::Int);
    assert(func->getParams().size() == 2);
    assert(func->getBody() != nullptr);

    // Test operator names
    assert(std::string(ll1::BinaryExpr::opName(ll1::BinaryExpr::Op::Add)) == "+");
    assert(std::string(ll1::BinaryExpr::opName(ll1::BinaryExpr::Op::Eq)) == "==");

    std::cout << "All AST tests passed.\n";
    return 0;
}
