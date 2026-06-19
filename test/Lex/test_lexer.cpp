#include <ll1/Lex/Lexer.h>
#include <cassert>
#include <iostream>
#include <sstream>

void testKeywords() {
    std::istringstream in("int char bool void if else while for break continue return fn true false");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].is(ll1::TokenKind::KW_INT));
    assert(toks[1].is(ll1::TokenKind::KW_CHAR));
    assert(toks[2].is(ll1::TokenKind::KW_BOOL));
    assert(toks[3].is(ll1::TokenKind::KW_VOID));
    assert(toks[4].is(ll1::TokenKind::KW_IF));
    assert(toks[5].is(ll1::TokenKind::KW_ELSE));
    assert(toks[6].is(ll1::TokenKind::KW_WHILE));
    assert(toks[7].is(ll1::TokenKind::KW_FOR));
    assert(toks[8].is(ll1::TokenKind::KW_BREAK));
    assert(toks[9].is(ll1::TokenKind::KW_CONTINUE));
    assert(toks[10].is(ll1::TokenKind::KW_RETURN));
    assert(toks[11].is(ll1::TokenKind::KW_FN));
    assert(toks[12].is(ll1::TokenKind::KW_TRUE));
    assert(toks[13].is(ll1::TokenKind::KW_FALSE));
    assert(toks[14].is(ll1::TokenKind::TOK_EOF));
    std::cout << "  Keywords: OK\n";
}

void testIdentifiers() {
    std::istringstream in("x myVar _private foo123");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].is(ll1::TokenKind::TOK_IDENT));
    assert(toks[0].Lexeme == "x");
    assert(toks[1].is(ll1::TokenKind::TOK_IDENT));
    assert(toks[1].Lexeme == "myVar");
    assert(toks[2].Lexeme == "_private");
    assert(toks[3].Lexeme == "foo123");
    std::cout << "  Identifiers: OK\n";
}

void testNumbers() {
    std::istringstream in("0 42 100");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].is(ll1::TokenKind::TOK_NUMBER));
    assert(toks[0].IntValue == 0);
    assert(toks[1].IntValue == 42);
    assert(toks[2].IntValue == 100);
    std::cout << "  Numbers: OK\n";
}

void testOperators() {
    std::istringstream in("+ - * / % < > <= >= == != && || ! =");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].is(ll1::TokenKind::OP_PLUS));
    assert(toks[1].is(ll1::TokenKind::OP_MINUS));
    assert(toks[2].is(ll1::TokenKind::OP_STAR));
    assert(toks[3].is(ll1::TokenKind::OP_SLASH));
    assert(toks[4].is(ll1::TokenKind::OP_PERCENT));
    assert(toks[5].is(ll1::TokenKind::OP_LT));
    assert(toks[6].is(ll1::TokenKind::OP_GT));
    assert(toks[7].is(ll1::TokenKind::OP_LE));
    assert(toks[8].is(ll1::TokenKind::OP_GE));
    assert(toks[9].is(ll1::TokenKind::OP_EQ));
    assert(toks[10].is(ll1::TokenKind::OP_NE));
    assert(toks[11].is(ll1::TokenKind::OP_AND));
    assert(toks[12].is(ll1::TokenKind::OP_OR));
    assert(toks[13].is(ll1::TokenKind::OP_NOT));
    assert(toks[14].is(ll1::TokenKind::OP_ASSIGN));
    std::cout << "  Operators: OK\n";
}

void testDelimiters() {
    std::istringstream in("( ) { } ; ,");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].is(ll1::TokenKind::LPAREN));
    assert(toks[1].is(ll1::TokenKind::RPAREN));
    assert(toks[2].is(ll1::TokenKind::LBRACE));
    assert(toks[3].is(ll1::TokenKind::RBRACE));
    assert(toks[4].is(ll1::TokenKind::SEMI));
    assert(toks[5].is(ll1::TokenKind::COMMA));
    std::cout << "  Delimiters: OK\n";
}

void testComments() {
    std::istringstream in("int // this is a comment\nchar");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].is(ll1::TokenKind::KW_INT));
    assert(toks[1].is(ll1::TokenKind::KW_CHAR)); // comment skipped
    std::cout << "  Comments: OK\n";
}

void testSourceLocation() {
    std::istringstream in("int\nchar");
    ll1::Lexer lexer(in);
    auto toks = lexer.tokenize();

    assert(toks[0].Loc.Line == 1);
    assert(toks[1].Loc.Line == 2);
    std::cout << "  Source Location: OK\n";
}

int main() {
    std::cout << "Lexer tests:\n";
    testKeywords();
    testIdentifiers();
    testNumbers();
    testOperators();
    testDelimiters();
    testComments();
    testSourceLocation();
    std::cout << "All lexer tests passed.\n";
    return 0;
}
