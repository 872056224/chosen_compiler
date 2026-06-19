#include <ll1/IRGen/IRGen.h>
#include <ll1/Sema/Sema.h>
#include <ll1/Parse/Parser.h>
#include <ll1/Lex/Lexer.h>
#include <ll1/IR/Module.h>
#include <ll1/IR/Function.h>
#include <cassert>
#include <iostream>
#include <sstream>

void testIRGen(const std::string &source, const std::string &name) {
    std::istringstream in(source);
    ll1::Lexer lexer(in);
    ll1::Parser parser(lexer);
    auto prog = parser.parseProgram();
    assert(!parser.hasError());

    ll1::DiagnosticEngine diag;
    ll1::Sema sema(diag);
    bool ok = sema.analyze(*prog);
    assert(ok);
    assert(!diag.hasErrors());

    ll1::IRGen irgen;
    auto mod = irgen.generate(*prog);
    assert(mod != nullptr);

    std::cout << "  " << name << ": OK (" << mod->getFunctionList().size() << " functions)\n";
}

int main() {
    std::cout << "IRGen tests:\n";

    // Variable operations
    testIRGen("fn int main() { int a = 10; return a; }", "var decl + ret");
    testIRGen("fn int main() { int a = 1; int b = 2; return a + b; }", "add");
    testIRGen("fn int main() { int a = 10; int b = 3; return a - b; }", "sub");
    testIRGen("fn int main() { int a = 2; int b = 3; return a * b; }", "mul");
    testIRGen("fn int main() { int a = 6; int b = 2; return a / b; }", "div");

    // Comparison
    testIRGen("fn int main() { int a = 5; int b = 3; if (a < b) { return 1; } return 0; }", "if with lt");

    // While loop
    testIRGen("fn int main() { int i = 0; while (i < 10) { i = i + 1; } return i; }", "while");

    // Void function
    testIRGen("fn void main() { int x = 0; return; }", "void function");

    // Multiple functions
    testIRGen("fn int add(int a, int b) { return a + b; } fn int main() { return 0; }", "multi func");

    // Bool
    testIRGen("fn int main() { bool flag = true; return 0; }", "bool var");

    // Complex expression
    testIRGen("fn int main() { int a = 1; int b = 2; int c = a * b + 3; return c; }", "complex expr");

    // Assignment
    testIRGen("fn int main() { int x = 0; x = 42; return x; }", "assignment");

    std::cout << "All IRGen tests passed.\n";
    return 0;
}
