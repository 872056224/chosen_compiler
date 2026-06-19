#include <ll1/Parse/Parser.h>
#include <sstream>

namespace ll1 {

Parser::Parser(Lexer &lex) : Lex(lex) {
    advance(); // Prime first token
}

void Parser::advance() {
    CurTok = Lex.nextToken();
}

bool Parser::check(TokenKind k) const {
    return CurTok.is(k);
}

bool Parser::match(TokenKind k) {
    if (check(k)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenKind k, const std::string &msg) {
    if (check(k)) {
        Token t = CurTok;
        advance();
        return t;
    }
    error(msg + " (got '" + CurTok.Lexeme + "')");
    return Token{TokenKind::TOK_ERROR, "", {0, 0}, 0};
}

void Parser::error(const std::string &msg) {
    if (!HasError) {
        HasError = true;
        std::ostringstream ss;
        ss << "line " << CurTok.Loc.Line << ": " << msg;
        ErrorMsg = ss.str();
    }
}

BuiltinType Parser::tokenToType(TokenKind k) {
    switch (k) {
    case TokenKind::KW_INT:  return BuiltinType::Int;
    case TokenKind::KW_CHAR: return BuiltinType::Char;
    case TokenKind::KW_BOOL: return BuiltinType::Bool;
    case TokenKind::KW_VOID: return BuiltinType::Void;
    default: return BuiltinType::Void;
    }
}

BinaryExpr::Op Parser::tokenToBinaryOp(TokenKind k) {
    switch (k) {
    case TokenKind::OP_PLUS:   return BinaryExpr::Op::Add;
    case TokenKind::OP_MINUS:  return BinaryExpr::Op::Sub;
    case TokenKind::OP_STAR:   return BinaryExpr::Op::Mul;
    case TokenKind::OP_SLASH:  return BinaryExpr::Op::Div;
    case TokenKind::OP_PERCENT: return BinaryExpr::Op::Rem;
    case TokenKind::OP_LT:     return BinaryExpr::Op::Lt;
    case TokenKind::OP_GT:     return BinaryExpr::Op::Gt;
    case TokenKind::OP_LE:     return BinaryExpr::Op::Le;
    case TokenKind::OP_GE:     return BinaryExpr::Op::Ge;
    case TokenKind::OP_EQ:     return BinaryExpr::Op::Eq;
    case TokenKind::OP_NE:     return BinaryExpr::Op::Ne;
    case TokenKind::OP_AND:    return BinaryExpr::Op::And;
    case TokenKind::OP_OR:     return BinaryExpr::Op::Or;
    case TokenKind::OP_ASSIGN: return BinaryExpr::Op::Assign;
    default: return BinaryExpr::Op::Add; // unreachable
    }
}

UnaryExpr::Op Parser::tokenToUnaryOp(TokenKind k) {
    switch (k) {
    case TokenKind::OP_MINUS: return UnaryExpr::Op::Neg;
    case TokenKind::OP_NOT:   return UnaryExpr::Op::Not;
    default: return UnaryExpr::Op::Neg; // unreachable
    }
}

// ====== Type Parsing ======
BuiltinType Parser::parseType() {
    if (check(TokenKind::KW_INT) || check(TokenKind::KW_CHAR) ||
        check(TokenKind::KW_BOOL) || check(TokenKind::KW_VOID)) {
        BuiltinType t = tokenToType(CurTok.Kind);
        advance();
        return t;
    }
    error("expected type");
    return BuiltinType::Void;
}

// ====== Expression Parsing (precedence climbing) ======
std::unique_ptr<Expr> Parser::parseExpr() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto lhs = parseLogicalOr();
    if (match(TokenKind::OP_ASSIGN)) {
        auto rhs = parseAssignment();
        return std::make_unique<BinaryExpr>(BinaryExpr::Op::Assign,
            std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto lhs = parseLogicalAnd();
    while (check(TokenKind::OP_OR)) {
        BinaryExpr::Op op = tokenToBinaryOp(CurTok.Kind);
        advance();
        auto rhs = parseLogicalAnd();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto lhs = parseEquality();
    while (check(TokenKind::OP_AND)) {
        BinaryExpr::Op op = tokenToBinaryOp(CurTok.Kind);
        advance();
        auto rhs = parseEquality();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto lhs = parseRelational();
    while (check(TokenKind::OP_EQ) || check(TokenKind::OP_NE)) {
        BinaryExpr::Op op = tokenToBinaryOp(CurTok.Kind);
        advance();
        auto rhs = parseRelational();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto lhs = parseAdditive();
    while (check(TokenKind::OP_LT) || check(TokenKind::OP_GT) ||
           check(TokenKind::OP_LE) || check(TokenKind::OP_GE)) {
        BinaryExpr::Op op = tokenToBinaryOp(CurTok.Kind);
        advance();
        auto rhs = parseAdditive();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    auto lhs = parseMultiplicative();
    while (check(TokenKind::OP_PLUS) || check(TokenKind::OP_MINUS)) {
        BinaryExpr::Op op = tokenToBinaryOp(CurTok.Kind);
        advance();
        auto rhs = parseMultiplicative();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto lhs = parseUnary();
    while (check(TokenKind::OP_STAR) || check(TokenKind::OP_SLASH) ||
           check(TokenKind::OP_PERCENT)) {
        BinaryExpr::Op op = tokenToBinaryOp(CurTok.Kind);
        advance();
        auto rhs = parseUnary();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenKind::OP_MINUS) || check(TokenKind::OP_NOT)) {
        UnaryExpr::Op op = tokenToUnaryOp(CurTok.Kind);
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }
    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    // ident '(' Args? ')'  -> function call
    if (check(TokenKind::TOK_IDENT)) {
        std::string name = CurTok.Lexeme;
        SourceLocation loc = CurTok.Loc;
        advance();
        if (match(TokenKind::LPAREN)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenKind::RPAREN)) {
                args.push_back(parseExpr());
                while (match(TokenKind::COMMA)) {
                    args.push_back(parseExpr());
                }
            }
            expect(TokenKind::RPAREN, "expected ')' after arguments");
            auto call = std::make_unique<CallExpr>(name, std::move(args));
            call->setLoc(loc);
            return call;
        }
        auto var = std::make_unique<VarExpr>(name);
        var->setLoc(loc);
        return var;
    }

    // number
    if (check(TokenKind::TOK_NUMBER)) {
        int16_t val = CurTok.IntValue;
        auto loc = CurTok.Loc;
        advance();
        auto lit = std::make_unique<IntegerLiteral>(val);
        lit->setLoc(loc);
        return lit;
    }

    // '(' Expr ')'
    if (match(TokenKind::LPAREN)) {
        auto expr = parseExpr();
        expect(TokenKind::RPAREN, "expected ')' after expression");
        return expr;
    }

    // true / false
    if (check(TokenKind::KW_TRUE)) {
        advance();
        return std::make_unique<BoolLiteral>(true);
    }
    if (check(TokenKind::KW_FALSE)) {
        advance();
        return std::make_unique<BoolLiteral>(false);
    }

    error("expected expression");
    return std::make_unique<IntegerLiteral>(0);
}

// ====== Statement Parsing ======
std::unique_ptr<Stmt> Parser::parseStmt() {
    // VarDecl or ExprStmt
    if (check(TokenKind::KW_INT) || check(TokenKind::KW_CHAR) || check(TokenKind::KW_BOOL)) {
        return parseExprOrVarDecl(); // starts with type -> VarDecl
    }

    // Other statements
    switch (CurTok.Kind) {
    case TokenKind::TOK_IDENT:
    case TokenKind::TOK_NUMBER:
    case TokenKind::LPAREN:
    case TokenKind::OP_MINUS:
    case TokenKind::OP_NOT:
    case TokenKind::KW_TRUE:
    case TokenKind::KW_FALSE:
        return parseExprOrVarDecl(); // starts with expr -> ExprStmt

    case TokenKind::KW_IF:     return parseIfStmt();
    case TokenKind::KW_WHILE:  return parseWhileStmt();
    case TokenKind::KW_FOR:    return parseForStmt();
    case TokenKind::KW_BREAK:  return parseBreakStmt();
    case TokenKind::KW_CONTINUE: return parseContinueStmt();
    case TokenKind::KW_RETURN: return parseReturnStmt();
    case TokenKind::LBRACE:    return parseBlock();
    case TokenKind::SEMI:      advance(); return std::make_unique<Block>(); // empty stmt

    default:
        error("expected statement");
        advance();
        return std::make_unique<Block>();
    }
}

std::unique_ptr<Stmt> Parser::parseExprOrVarDecl() {
    if (check(TokenKind::KW_INT) || check(TokenKind::KW_CHAR) || check(TokenKind::KW_BOOL)) {
        // Variable declaration
        return parseVarDecl();
    }
    // Expression statement
    auto expr = (check(TokenKind::SEMI)) ? nullptr : parseExpr();
    expect(TokenKind::SEMI, "expected ';' after expression");
    // Wrap expr in a minimal way - return the expr directly (Expr : Stmt)
    return expr;
}

std::unique_ptr<Block> Parser::parseBlock() {
    SourceLocation loc = CurTok.Loc;
    expect(TokenKind::LBRACE, "expected '{'");
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!check(TokenKind::RBRACE) && !check(TokenKind::TOK_EOF)) {
        stmts.push_back(parseStmt());
    }
    expect(TokenKind::RBRACE, "expected '}'");
    auto block = std::make_unique<Block>(std::move(stmts));
    block->setLoc(loc);
    return block;
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    SourceLocation loc = CurTok.Loc;
    expect(TokenKind::KW_IF, "expected 'if'");
    expect(TokenKind::LPAREN, "expected '(' after 'if'");
    auto cond = parseExpr();
    expect(TokenKind::RPAREN, "expected ')' after condition");
    auto thenBlock = parseStmt();
    std::unique_ptr<Stmt> elseBlock = nullptr;
    if (match(TokenKind::KW_ELSE)) {
        // else Block or else IfStmt
        elseBlock = parseStmt();
    }
    auto stmt = std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    stmt->setLoc(loc);
    return stmt;
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    SourceLocation loc = CurTok.Loc;
    expect(TokenKind::KW_WHILE, "expected 'while'");
    expect(TokenKind::LPAREN, "expected '(' after 'while'");
    auto cond = parseExpr();
    expect(TokenKind::RPAREN, "expected ')' after condition");
    auto body = parseStmt();
    auto stmt = std::make_unique<WhileStmt>(std::move(cond), std::move(body));
    stmt->setLoc(loc);
    return stmt;
}

std::unique_ptr<ForStmt> Parser::parseForStmt() {
    SourceLocation loc = CurTok.Loc;
    expect(TokenKind::KW_FOR, "expected 'for'");
    expect(TokenKind::LPAREN, "expected '(' after 'for'");
    auto init = check(TokenKind::SEMI) ? nullptr : parseExpr();
    expect(TokenKind::SEMI, "expected ';' in for");
    auto cond = check(TokenKind::SEMI) ? nullptr : parseExpr();
    expect(TokenKind::SEMI, "expected ';' in for");
    auto incr = check(TokenKind::RPAREN) ? nullptr : parseExpr();
    expect(TokenKind::RPAREN, "expected ')' after for clauses");
    auto body = parseStmt();
    // Wrap init/incr as expression statements if non-null
    auto initStmt = init ? std::unique_ptr<Stmt>(std::move(init)) : nullptr;
    auto incrExpr = incr ? std::unique_ptr<Expr>(std::move(incr)) : nullptr;
    auto stmt = std::make_unique<ForStmt>(std::move(initStmt), std::move(cond),
                                          std::move(incrExpr), std::move(body));
    stmt->setLoc(loc);
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseBreakStmt() {
    auto loc = CurTok.Loc;
    expect(TokenKind::KW_BREAK, "expected 'break'");
    expect(TokenKind::SEMI, "expected ';' after 'break'");
    auto s = std::make_unique<BreakStmt>();
    s->setLoc(loc);
    return s;
}

std::unique_ptr<Stmt> Parser::parseContinueStmt() {
    auto loc = CurTok.Loc;
    expect(TokenKind::KW_CONTINUE, "expected 'continue'");
    expect(TokenKind::SEMI, "expected ';' after 'continue'");
    auto s = std::make_unique<ContinueStmt>();
    s->setLoc(loc);
    return s;
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    auto loc = CurTok.Loc;
    expect(TokenKind::KW_RETURN, "expected 'return'");
    std::unique_ptr<Expr> val = nullptr;
    if (!check(TokenKind::SEMI)) {
        val = parseExpr();
    }
    expect(TokenKind::SEMI, "expected ';' after return");
    auto s = std::make_unique<ReturnStmt>(std::move(val));
    s->setLoc(loc);
    return s;
}

// ====== Declaration Parsing ======
std::unique_ptr<VarDecl> Parser::parseVarDecl() {
    auto loc = CurTok.Loc;
    BuiltinType ty = parseType();
    Token nameTok = expect(TokenKind::TOK_IDENT, "expected identifier");
    std::unique_ptr<Expr> init = nullptr;
    if (match(TokenKind::OP_ASSIGN)) {
        init = parseExpr();
    }
    expect(TokenKind::SEMI, "expected ';' after variable declaration");
    auto decl = std::make_unique<VarDecl>(ty, nameTok.Lexeme, std::move(init));
    decl->setLoc(loc);
    return decl;
}

std::unique_ptr<FuncDecl> Parser::parseFuncDef() {
    auto loc = CurTok.Loc;
    expect(TokenKind::KW_FN, "expected 'fn'");
    BuiltinType returnTy = parseType();
    Token nameTok = expect(TokenKind::TOK_IDENT, "expected function name");
    expect(TokenKind::LPAREN, "expected '(' after function name");

    // Parameters
    std::vector<FuncDecl::Param> params;
    if (!check(TokenKind::RPAREN)) {
        do {
            BuiltinType pty = parseType();
            Token pname = expect(TokenKind::TOK_IDENT, "expected parameter name");
            params.push_back({pty, pname.Lexeme});
        } while (match(TokenKind::COMMA));
    }
    expect(TokenKind::RPAREN, "expected ')' after parameters");

    auto body = parseStmt(); // parseBlock calls parseStmt internally
    auto decl = std::make_unique<FuncDecl>(returnTy, nameTok.Lexeme, params, std::move(body));
    decl->setLoc(loc);
    return decl;
}

std::unique_ptr<ASTNode> Parser::parseTopLevel() {
    if (check(TokenKind::KW_FN)) {
        return parseFuncDef();
    }
    if (check(TokenKind::KW_INT) || check(TokenKind::KW_CHAR) ||
        check(TokenKind::KW_BOOL)) {
        return parseVarDecl();
    }
    error("expected function or variable declaration at top level");
    advance();
    return nullptr;
}

std::unique_ptr<Program> Parser::parseProgram() {
    auto prog = std::make_unique<Program>();
    while (!check(TokenKind::TOK_EOF) && !HasError) {
        auto decl = parseTopLevel();
        if (decl) {
            prog->Declarations.push_back(std::move(decl));
        }
        if (HasError) break;
    }
    return prog;
}

} // namespace ll1
