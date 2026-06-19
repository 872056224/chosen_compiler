#ifndef LL1_LEX_LEXER_H
#define LL1_LEX_LEXER_H

#include <ll1/Lex/Token.h>
#include <string>
#include <vector>
#include <istream>

namespace ll1 {

class Lexer {
public:
    explicit Lexer(std::istream &input);

    // Get next token
    Token nextToken();

    // Tokenize entire input
    std::vector<Token> tokenize();

    bool hasError() const { return HasError; }

private:
    std::istream &In;
    SourceLocation CurLoc;
    char CurChar = ' ';
    bool HasError = false;

    void advance();
    char peek() const;

    void skipWhitespace();
    void skipLineComment();

    Token readIdentifier();
    Token readNumber();
    Token readOperator();

    Token makeToken(TokenKind kind, const std::string &lexeme);
    Token makeError(const std::string &msg);
};

} // namespace ll1

#endif
