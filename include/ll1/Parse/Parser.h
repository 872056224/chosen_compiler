#ifndef LL1_PARSE_PARSER_H
#define LL1_PARSE_PARSER_H

#include <ll1/Lex/Lexer.h>
#include <ll1/AST/AST.h>
#include <memory>
#include <vector>
#include <string>

namespace ll1 {

class Parser {
public:
    explicit Parser(Lexer &lex);

    // Entry point
    std::unique_ptr<class Program> parseProgram();

    bool hasError() const { return HasError; }
    const std::string &getErrorMsg() const { return ErrorMsg; }

private:
    Lexer &Lex;
    Token CurTok;
    bool HasError = false;
    std::string ErrorMsg;

    void advance();
    bool check(TokenKind k) const;
    bool match(TokenKind k);
    Token expect(TokenKind k, const std::string &msg);
    void error(const std::string &msg);

    // Decl parsing
    std::unique_ptr<ASTNode> parseTopLevel();
    std::unique_ptr<FuncDecl> parseFuncDef();
    std::unique_ptr<VarDecl> parseVarDecl();

    // Type parsing
    BuiltinType parseType();

    // Stmt parsing
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<IfStmt> parseIfStmt();
    std::unique_ptr<WhileStmt> parseWhileStmt();
    std::unique_ptr<ForStmt> parseForStmt();
    std::unique_ptr<Stmt> parseBreakStmt();
    std::unique_ptr<Stmt> parseContinueStmt();
    std::unique_ptr<ReturnStmt> parseReturnStmt();
    std::unique_ptr<PrintStmt> parsePrintStmt();
    std::unique_ptr<Stmt> parseExprOrVarDecl();

    // Expr parsing (precedence climbing via recursive descent)
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseMultiplicative();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();

    // Convert token to binary op
    static BinaryExpr::Op tokenToBinaryOp(TokenKind k);
    static UnaryExpr::Op tokenToUnaryOp(TokenKind k);
    static BuiltinType tokenToType(TokenKind k);
};

// Program node to hold top-level declarations
class Program {
public:
    std::vector<std::unique_ptr<ASTNode>> Declarations;
};

} // namespace ll1

#endif
