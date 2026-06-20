#include <ll1/Driver/Compiler.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

void testCompile(const std::string &source, const std::string &name,
                 const std::vector<std::string> &expectedOps) {
    ll1::Compiler compiler;
    std::string asmCode = compiler.compile(source);

    assert(!compiler.hasError());
    assert(!asmCode.empty());
    assert(asmCode.find("hlt") != std::string::npos);

    for (auto &op : expectedOps) {
        assert(asmCode.find(op) != std::string::npos);
    }

    std::cout << "  " << name << ": OK\n";
}

int main() {
    std::cout << "Integration tests:\n";

    // Test 1: Simple expression
    testCompile(
        "fn int main() { int a = 10; int b = 20; return a + b; }",
        "add vars",
        {"main:", "add", "ret"}
    );

    // Test 2: If/else
    testCompile(
        "fn int main() { int a = 5; if (a < 10) { return 1; } else { return 0; } }",
        "if else",
        {"main:", "cmp", "ret"}
    );

    // Test 3: While loop
    testCompile(
        "fn int main() { int i = 0; while (i < 10) { i = i + 1; } return i; }",
        "while",
        {"main:", "cmp", "jmp", "ret"}
    );

    // Test 4: Void main
    testCompile(
        "fn void main() { int x = 42; return; }",
        "void main",
        {"main:", "ret"}
    );

    // Test 5: Multiple returns
    testCompile(
        "fn int main() { int a = 10; int b = 20; if (a < b) { return b; } return a; }",
        "multiple returns",
        {"main:", "cmp", "ret"}
    );

    // Test 6: Compile from file
    {
        // Write a temp source file
        std::ofstream tmp("test_input.ll1");
        tmp << "fn int main() { return 99; }\n";
        tmp.close();

        ll1::Compiler compiler;
        std::string asmCode = compiler.compileFile("test_input.ll1");
        assert(!compiler.hasError());
        assert(asmCode.find("main:") != std::string::npos);
        assert(asmCode.find("ret") != std::string::npos);
        std::cout << "  file input: OK\n";

        // Cleanup
        std::remove("test_input.ll1");
    }

    // Test 7: Full pipeline -- write output to file
    {
        ll1::Compiler compiler;
        std::string asmCode = compiler.compile("fn int main() { return 42; }");
        assert(!compiler.hasError());

        std::ofstream out("test_output.asm");
        out << asmCode;
        out.close();

        std::ifstream in("test_output.asm");
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        assert(content.find("main:") != std::string::npos);
        assert(content.find("ret") != std::string::npos);
        std::cout << "  write output file: OK\n";

        std::remove("test_output.asm");
    }

    std::cout << "All integration tests passed.\n";
    return 0;
}
