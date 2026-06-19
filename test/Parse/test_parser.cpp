#include <ll1/Parse/Parser.h>
#include <ll1/Lex/Lexer.h>
#include <cassert>
#include <iostream>
#include <sstream>

void testParse(const std::string &source, const std::string &name) {
    std::istringstream in(source);
    ll1::Lexer lexer(in);
    ll1::Parser parser(lexer);
    auto prog = parser.parseProgram();
    assert(prog != nullptr);
    assert(!parser.hasError());
    std::cout << "  " << name << ": OK (" << prog->Declarations.size() << " decls)\n";
}

void testParseError(const std::string &source, const std::string &name) {
    std::istringstream in(source);
    ll1::Lexer lexer(in);
    ll1::Parser parser(lexer);
    auto prog = parser.parseProgram();
    assert(parser.hasError());
    std::cout << "  " << name << ": OK (error: " << parser.getErrorMsg() << ")\n";
}

int main() {
    std::cout << "Parser tests:\n";

    // Variable declarations
    testParse("int a;", "simple var decl");
    testParse("int a = 10;", "var with init");
    testParse("char c = 120;", "char var");
    testParse("bool flag = true;", "bool var");

    // Expressions
    testParse("int a = 1 + 2;", "add");
    testParse("int a = 3 - 1;", "sub");
    testParse("int a = 2 * 3;", "mul");
    testParse("int a = 6 / 2;", "div");
    testParse("int a = 7 % 3;", "rem");
    testParse("int a = 1 + 2 * 3;", "precedence add/mul");
    testParse("int a = (1 + 2) * 3;", "parenthesized");
    testParse("int a = -5;", "unary neg");
    testParse("int a = !flag;", "unary not");
    testParse("int a = 1 < 2;", "lt");
    testParse("int a = 1 >= 2;", "ge");
    testParse("int a = 1 == 2;", "eq");
    testParse("int a = 1 != 2;", "ne");
    testParse("int a = true && false;", "and");
    testParse("int a = true || false;", "or");
    testParse("int a = x = 5;", "assignment chain");

    // Empty top-level
    testParse("", "empty program");

    // Multiple declarations
    testParse("int a;\nint b = 2;", "multiple var decls");

    // Source location
    std::istringstream in("int x;\nint y;");
    ll1::Lexer lexer(in);
    ll1::Parser parser(lexer);
    auto prog = parser.parseProgram();
    assert(prog->Declarations.size() == 2);
    assert(prog->Declarations[0]->getLoc().Line == 1);
    assert(prog->Declarations[1]->getLoc().Line == 2);
    std::cout << "  source locations: OK\n";

    // Error cases
    testParseError("int ;", "missing ident");
    testParseError("x = 5;", "bare expression at top-level");
    testParseError("int a = (1 + 2;", "unmatched paren");

    std::cout << "All parser tests passed.\n";
    return 0;
}
