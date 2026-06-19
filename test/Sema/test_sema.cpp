#include <ll1/Sema/Sema.h>
#include <ll1/Parse/Parser.h>
#include <ll1/Lex/Lexer.h>
#include <cassert>
#include <iostream>
#include <sstream>

void testPass(const std::string &source, const std::string &name) {
    std::istringstream in(source);
    ll1::Lexer lexer(in);
    ll1::Parser parser(lexer);
    auto prog = parser.parseProgram();
    assert(!parser.hasError());

    ll1::DiagnosticEngine diag;
    ll1::Sema sema(diag);
    bool ok = sema.analyze(*prog);
    if (!ok) {
        diag.printAll();
        assert(ok);
    }
    assert(!diag.hasErrors());
    std::cout << "  " << name << ": OK\n";
}

void testFail(const std::string &source, const std::string &name) {
    std::istringstream in(source);
    ll1::Lexer lexer(in);
    ll1::Parser parser(lexer);
    auto prog = parser.parseProgram();
    // Parser may or may not error—we test Sema specifically
    ll1::DiagnosticEngine diag;
    ll1::Sema sema(diag);
    sema.analyze(*prog);
    assert(diag.hasErrors());
    std::cout << "  " << name << ": OK (error detected)\n";
}

int main() {
    std::cout << "Sema tests:\n";

    // Valid programs
    testPass("int x;", "global var");
    testPass("int x = 42;", "global var with init");
    testPass("bool flag = true;", "bool var");
    testPass("fn int main() { return 0; }", "simple function");
    testPass("fn int main() { int x = 1; int y = 2; return x + y; }", "local vars + expr");
    testPass("fn int main() { if (true) { return 0; } return 1; }", "if stmt");
    testPass("fn int main() { int i = 0; while (i < 10) { i = i + 1; } return i; }", "while loop");

    // Error detection
    testFail("int x; int x;", "duplicate global");
    testFail("fn int f() { int x; int x; return 0; }", "duplicate local");
    testFail("fn int f() { return x; }", "undeclared var");
    testFail("fn char f() { return 42; }", "return type mismatch");
    testFail("fn int f() { int x; if (x) { return 0; } return 0; }", "non-bool condition");

    std::cout << "All sema tests passed.\n";
    return 0;
}
