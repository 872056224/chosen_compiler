#include <ll1/Lex/Lexer.h>
#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace ll1 {

// Keyword lookup table
static const std::unordered_map<std::string, TokenKind> Keywords = {
    {"int", TokenKind::KW_INT}, {"char", TokenKind::KW_CHAR},
    {"bool", TokenKind::KW_BOOL}, {"void", TokenKind::KW_VOID},
    {"if", TokenKind::KW_IF}, {"else", TokenKind::KW_ELSE},
    {"while", TokenKind::KW_WHILE}, {"for", TokenKind::KW_FOR},
    {"break", TokenKind::KW_BREAK}, {"continue", TokenKind::KW_CONTINUE},
    {"return", TokenKind::KW_RETURN}, {"fn", TokenKind::KW_FN},
    {"true", TokenKind::KW_TRUE}, {"false", TokenKind::KW_FALSE},
};

// ---- Token helpers ----
bool Token::isKeyword() const {
    return Kind >= TokenKind::KW_INT && Kind <= TokenKind::KW_FALSE;
}
bool Token::isOperator() const {
    return Kind >= TokenKind::OP_PLUS && Kind <= TokenKind::OP_ASSIGN;
}
bool Token::isLiteral() const {
    return Kind == TokenKind::TOK_NUMBER;
}
const char *Token::kindName() const { return tokenKindName(Kind); }

const char *tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::TOK_EOF: return "EOF";
    case TokenKind::TOK_ERROR: return "ERROR";
    case TokenKind::TOK_IDENT: return "IDENT";
    case TokenKind::TOK_NUMBER: return "NUMBER";
    case TokenKind::KW_INT: return "int";
    case TokenKind::KW_CHAR: return "char";
    case TokenKind::KW_BOOL: return "bool";
    case TokenKind::KW_VOID: return "void";
    case TokenKind::KW_IF: return "if";
    case TokenKind::KW_ELSE: return "else";
    case TokenKind::KW_WHILE: return "while";
    case TokenKind::KW_FOR: return "for";
    case TokenKind::KW_BREAK: return "break";
    case TokenKind::KW_CONTINUE: return "continue";
    case TokenKind::KW_RETURN: return "return";
    case TokenKind::KW_FN: return "fn";
    case TokenKind::KW_TRUE: return "true";
    case TokenKind::KW_FALSE: return "false";
    case TokenKind::OP_PLUS: return "+";
    case TokenKind::OP_MINUS: return "-";
    case TokenKind::OP_STAR: return "*";
    case TokenKind::OP_SLASH: return "/";
    case TokenKind::OP_PERCENT: return "%";
    case TokenKind::OP_LT: return "<";
    case TokenKind::OP_GT: return ">";
    case TokenKind::OP_LE: return "<=";
    case TokenKind::OP_GE: return ">=";
    case TokenKind::OP_EQ: return "==";
    case TokenKind::OP_NE: return "!=";
    case TokenKind::OP_AND: return "&&";
    case TokenKind::OP_OR: return "||";
    case TokenKind::OP_NOT: return "!";
    case TokenKind::OP_ASSIGN: return "=";
    case TokenKind::LPAREN: return "(";
    case TokenKind::RPAREN: return ")";
    case TokenKind::LBRACE: return "{";
    case TokenKind::RBRACE: return "}";
    case TokenKind::SEMI: return ";";
    case TokenKind::COMMA: return ",";
    }
    return "UNKNOWN";
}

// ---- Lexer ----
Lexer::Lexer(std::istream &input) : In(input) {
    CurLoc.Line = 1;
    CurLoc.Col = 0;
    advance(); // Prime first character
}

void Lexer::advance() {
    if (CurChar == '\n') {
        CurLoc.Line++;
        CurLoc.Col = 0;
    }
    CurLoc.Col++;
    CurChar = In.get();
}

char Lexer::peek() const {
    return In.peek();
}

void Lexer::skipWhitespace() {
    while (CurChar == ' ' || CurChar == '\t' || CurChar == '\n' || CurChar == '\r') {
        advance();
    }
}

void Lexer::skipLineComment() {
    while (CurChar != '\n' && CurChar != EOF) {
        advance();
    }
}

Token Lexer::makeToken(TokenKind kind, const std::string &lexeme) {
    Token tok;
    tok.Kind = kind;
    tok.Lexeme = lexeme;
    tok.Loc = CurLoc;
    return tok;
}

Token Lexer::makeError(const std::string &msg) {
    HasError = true;
    Token tok;
    tok.Kind = TokenKind::TOK_ERROR;
    tok.Lexeme = msg;
    tok.Loc = CurLoc;
    return tok;
}

Token Lexer::readIdentifier() {
    SourceLocation startLoc = CurLoc;
    std::string lexeme;
    while (std::isalnum(static_cast<unsigned char>(CurChar)) || CurChar == '_') {
        lexeme += CurChar;
        advance();
    }
    auto it = Keywords.find(lexeme);
    TokenKind kind = (it != Keywords.end()) ? it->second : TokenKind::TOK_IDENT;
    Token tok = makeToken(kind, lexeme);
    tok.Loc = startLoc;  // Use starting position for identifiers
    return tok;
}

Token Lexer::readNumber() {
    SourceLocation startLoc = CurLoc;
    std::string lexeme;
    while (std::isdigit(static_cast<unsigned char>(CurChar))) {
        lexeme += CurChar;
        advance();
    }
    Token tok = makeToken(TokenKind::TOK_NUMBER, lexeme);
    tok.IntValue = static_cast<int16_t>(std::atoi(lexeme.c_str()));
    tok.Loc = startLoc;
    return tok;
}

Token Lexer::readOperator() {
    char first = CurChar;
    advance();
    switch (first) {
    case '+': return makeToken(TokenKind::OP_PLUS, "+");
    case '-': return makeToken(TokenKind::OP_MINUS, "-");
    case '*': return makeToken(TokenKind::OP_STAR, "*");
    case '%': return makeToken(TokenKind::OP_PERCENT, "%");
    case '(': return makeToken(TokenKind::LPAREN, "(");
    case ')': return makeToken(TokenKind::RPAREN, ")");
    case '{': return makeToken(TokenKind::LBRACE, "{");
    case '}': return makeToken(TokenKind::RBRACE, "}");
    case ';': return makeToken(TokenKind::SEMI, ";");
    case ',': return makeToken(TokenKind::COMMA, ",");

    case '/':
        if (CurChar == '/') { skipLineComment(); return nextToken(); }
        return makeToken(TokenKind::OP_SLASH, "/");

    case '<':
        if (CurChar == '=') { advance(); return makeToken(TokenKind::OP_LE, "<="); }
        return makeToken(TokenKind::OP_LT, "<");

    case '>':
        if (CurChar == '=') { advance(); return makeToken(TokenKind::OP_GE, ">="); }
        return makeToken(TokenKind::OP_GT, ">");

    case '=':
        if (CurChar == '=') { advance(); return makeToken(TokenKind::OP_EQ, "=="); }
        return makeToken(TokenKind::OP_ASSIGN, "=");

    case '!':
        if (CurChar == '=') { advance(); return makeToken(TokenKind::OP_NE, "!="); }
        return makeToken(TokenKind::OP_NOT, "!");

    case '&':
        if (CurChar == '&') { advance(); return makeToken(TokenKind::OP_AND, "&&"); }
        return makeError("expected '&&' but got '&'");

    case '|':
        if (CurChar == '|') { advance(); return makeToken(TokenKind::OP_OR, "||"); }
        return makeError("expected '||' but got '|'");

    default:
        return makeError(std::string("unexpected character: '") + first + "'");
    }
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (CurChar == EOF) {
        return makeToken(TokenKind::TOK_EOF, "");
    }

    // Identifier or keyword
    if (std::isalpha(static_cast<unsigned char>(CurChar)) || CurChar == '_') {
        return readIdentifier();
    }

    // Number
    if (std::isdigit(static_cast<unsigned char>(CurChar))) {
        return readNumber();
    }

    // Operators and delimiters
    return readOperator();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.is(TokenKind::TOK_EOF) || tok.is(TokenKind::TOK_ERROR)) {
            break;
        }
    }
    return tokens;
}

} // namespace ll1
