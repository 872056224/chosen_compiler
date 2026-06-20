#ifndef LL1_LEX_TOKEN_H
#define LL1_LEX_TOKEN_H

#include <string>
#include <cstdint>

namespace ll1 {

enum class TokenKind {
    // Special
    TOK_EOF,
    TOK_ERROR,
    TOK_IDENT,
    TOK_NUMBER,

    // Keywords
    KW_INT, KW_CHAR, KW_BOOL, KW_VOID,
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR,
    KW_BREAK, KW_CONTINUE, KW_RETURN,
    KW_FN, KW_TRUE, KW_FALSE,
    KW_PRINT, KW_READ,

    // Operators
    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_PERCENT,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR, OP_NOT, OP_ASSIGN,

    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET, SEMI, COMMA,
};

struct SourceLocation {
    unsigned Line = 1;
    unsigned Col = 0;
};

struct Token {
    TokenKind Kind;
    std::string Lexeme;
    SourceLocation Loc;
    int16_t IntValue = 0;    // Filled for TOK_NUMBER

    // Helpers
    bool is(TokenKind k) const { return Kind == k; }
    bool isKeyword() const;
    bool isOperator() const;
    bool isLiteral() const;
    const char *kindName() const;
};

const char *tokenKindName(TokenKind kind);

} // namespace ll1

#endif
